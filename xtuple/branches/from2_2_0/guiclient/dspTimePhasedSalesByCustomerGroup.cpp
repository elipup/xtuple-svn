/*
 * Common Public Attribution License Version 1.0. 
 * 
 * The contents of this file are subject to the Common Public Attribution 
 * License Version 1.0 (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License 
 * at http://www.xTuple.com/CPAL.  The License is based on the Mozilla 
 * Public License Version 1.1 but Sections 14 and 15 have been added to 
 * cover use of software over a computer network and provide for limited 
 * attribution for the Original Developer. In addition, Exhibit A has 
 * been modified to be consistent with Exhibit B.
 * 
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License. 
 * 
 * The Original Code is PostBooks Accounting, ERP, and CRM Suite. 
 * 
 * The Original Developer is not the Initial Developer and is __________. 
 * If left blank, the Original Developer is the Initial Developer. 
 * The Initial Developer of the Original Code is OpenMFG, LLC, 
 * d/b/a xTuple. All portions of the code written by xTuple are Copyright 
 * (c) 1999-2007 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
 * 
 * Contributor(s): ______________________.
 * 
 * Alternatively, the contents of this file may be used under the terms 
 * of the xTuple End-User License Agreeement (the xTuple License), in which 
 * case the provisions of the xTuple License are applicable instead of 
 * those above.  If you wish to allow use of your version of this file only 
 * under the terms of the xTuple License and not to allow others to use 
 * your version of this file under the CPAL, indicate your decision by 
 * deleting the provisions above and replace them with the notice and other 
 * provisions required by the xTuple License. If you do not delete the 
 * provisions above, a recipient may use your version of this file under 
 * either the CPAL or the xTuple License.
 * 
 * EXHIBIT B.  Attribution Information
 * 
 * Attribution Copyright Notice: 
 * Copyright (c) 1999-2007 by OpenMFG, LLC, d/b/a xTuple
 * 
 * Attribution Phrase: 
 * Powered by PostBooks, an open source solution from xTuple
 * 
 * Attribution URL: www.xtuple.org 
 * (to be included in the "Community" menu of the application if possible)
 * 
 * Graphic Image as provided in the Covered Code, if any. 
 * (online at www.xtuple.com/poweredby)
 * 
 * Display of Attribution Information is required in Larger Works which 
 * are defined in the CPAL as a work which combines Covered Code or 
 * portions thereof with code not governed by the terms of the CPAL.
 */

#include "dspTimePhasedSalesByCustomerGroup.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include <qstatusbar.h>
#include <parameter.h>
#include <qworkspace.h>
#include <q3valuevector.h>
#include <dbtools.h>
#include <datecluster.h>
#include <openreports.h>
#include "dspSalesHistoryByCustomer.h"
#include "rptTimePhasedSalesByCustomerGroup.h"
#include "OpenMFGGUIClient.h"
#include "submitReport.h"

/*
 *  Constructs a dspTimePhasedSalesByCustomerGroup as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspTimePhasedSalesByCustomerGroup::dspTimePhasedSalesByCustomerGroup(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_sohist, SIGNAL(populateMenu(Q3PopupMenu*,Q3ListViewItem*,int)), this, SLOT(sPopulateMenu(Q3PopupMenu*,Q3ListViewItem*,int)));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
    connect(_calendar, SIGNAL(newCalendarId(int)), _periods, SLOT(populate(int)));
    connect(_calendar, SIGNAL(select(ParameterList&)), _periods, SLOT(load(ParameterList&)));
    connect(_submit, SIGNAL(clicked()), this, SLOT(sSubmit()));
    
    if (!_metrics->boolean("EnableBatchManager"))
      _submit->hide();
    
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspTimePhasedSalesByCustomerGroup::~dspTimePhasedSalesByCustomerGroup()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspTimePhasedSalesByCustomerGroup::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <Q3PopupMenu>

void dspTimePhasedSalesByCustomerGroup::init()
{
  statusBar()->hide();
  
  _customerGroup->setType(CustomerGroup);
  _productCategory->setType(ProductCategory);
  
  _sohist->addColumn(tr("Cust. #"),  _orderColumn, Qt::AlignLeft );
  _sohist->addColumn(tr("Customer"), 180,          Qt::AlignLeft );
}

void dspTimePhasedSalesByCustomerGroup::sPrint()
{
  ParameterList params;
  params.append("print");
  _customerGroup->appendValue(params);
  _productCategory->appendValue(params);
  _periods->getSelected(params);

  if (_byCustomer->isChecked())
    params.append("orderByCustomer");
  else if (_bySales->isChecked())
    params.append("orderBySales");

  rptTimePhasedSalesByCustomerGroup newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspTimePhasedSalesByCustomerGroup::sViewShipments()
{
  ParameterList params;
  params.append("cust_id", _sohist->id());
  params.append("startDate", _columnDates[_column - 2].startDate);
  params.append("endDate", _columnDates[_column - 2].endDate);
  params.append("run");

  if (_productCategory->isSelected())
    params.append("prodcat_id", _productCategory->id());
  else if (_productCategory->isPattern())
    params.append("prodcat_pattern", _productCategory->pattern());

  dspSalesHistoryByCustomer *newdlg = new dspSalesHistoryByCustomer();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspTimePhasedSalesByCustomerGroup::sPopulateMenu(Q3PopupMenu *menuThis, Q3ListViewItem *, int pColumn)
{
  int intMenuItem;

  _column = pColumn;

  if (pColumn > 1)
  {
    intMenuItem = menuThis->insertItem(tr("View Sales Detail..."), this, SLOT(sViewShipments()), 0);
    if (!_privleges->check("ViewSalesHistory"))
      menuThis->setItemEnabled(intMenuItem, FALSE);
  }
}

void dspTimePhasedSalesByCustomerGroup::sFillList()
{
  _sohist->clear();

  if (!_periods->isPeriodSelected())
    return;

  _sohist->clear();
  while (_sohist->columns() > 2)
    _sohist->removeColumn(2);

  QString sql("SELECT cust_id, cust_number, cust_name");

  int columns = 1;
  XListViewItem *cursor = _periods->firstChild();
  if (cursor != 0)
  {
    do
    {
      if (_periods->isSelected(cursor))
      {
        if (_productCategory->isSelected())
          sql += QString(", shipmentsByCustomerValue(cust_id, %1, :prodcat_id) AS bucket%3")
                 .arg(cursor->id())
                 .arg(columns++);
        else if (_productCategory->isPattern())
          sql += QString(", shipmentsByCustomerValue(cust_id, %1, :prodcat_pattern) AS bucket%3")
                 .arg(cursor->id())
                 .arg(columns++);
        else
          sql += QString(", shipmentsByCustomerValue(cust_id, %1) AS bucket%2")
                 .arg(cursor->id())
                 .arg(columns++);
  
        _sohist->addColumn(formatDate(((PeriodListViewItem *)cursor)->startDate()), _qtyColumn, Qt::AlignRight);

        _columnDates.append(DatePair(((PeriodListViewItem *)cursor)->startDate(), ((PeriodListViewItem *)cursor)->endDate()));
      }
    }
    while ((cursor = cursor->nextSibling()) != 0);
  }

  sql += " FROM cust, custgrp, custgrpitem "
         "WHERE ( (custgrpitem_cust_id=cust_id)"
         " AND (custgrpitem_custgrp_id=custgrp_id)";

  if (_customerGroup->isSelected())
    sql += " AND (custgrp_id=:custgrp_id)";
  else if (_customerGroup->isPattern())
    sql += " AND (custgrp_name ~ :custgrp_pattern)";

  if (_byCustomer->isChecked())
    sql += ") "
           "ORDER BY cust_number;";
  else if (_bySales->isChecked())
    sql += ") "
           "ORDER BY bucket1 DESC;";

  q.prepare(sql);
  _customerGroup->bindValue(q);
  _productCategory->bindValue(q);
  q.exec();
  if (q.first())
  {
    Q3ValueVector<Numeric> totals(columns);;

    do
    {
      XListViewItem *item = new XListViewItem( _sohist, _sohist->lastItem(), q.value("cust_id").toInt(),
                                               q.value("cust_number"), q.value("cust_name") );

      for (int column = 1; column < columns; column++)
      {
        QString bucketName = QString("bucket%1").arg(column);
        item->setText((column + 1), formatMoney(q.value(bucketName).toDouble()));
        totals[column] += q.value(bucketName).toDouble();
      }
    }
    while (q.next());

//  Add the totals row
    XListViewItem *total = new XListViewItem(_sohist, _sohist->lastItem(), -1, QVariant(tr("Totals:")));
    for (int column = 1; column < columns; column++)
      total->setText((column + 1), formatMoney(totals[column].toDouble()));
  }
}

void dspTimePhasedSalesByCustomerGroup::sSubmit()
{
  if (_periods->isPeriodSelected())
  {
    ParameterList params(buildParameters());
    params.append("report_name", "TimePhasedSalesHistoryByCustomer");
    
    submitReport newdlg(this, "", TRUE);
    newdlg.set(params);

    if (newdlg.check() == cNoReportDefinition)
      QMessageBox::critical( this, tr("Report Definition Not Found"),
                             tr( "The report defintions for this report, \"TimePhasedSalesHistoryByCustomer\" cannot be found.\n"
                                 "Please contact your Systems Administrator and report this issue." ) );
    else
      newdlg.exec();
  }
  else
    QMessageBox::critical( this, tr("Incomplete criteria"),
                           tr( "The criteria you specified is not complete. Please make sure all\n"
                               "fields are correctly filled out before running the report." ) );
}

ParameterList dspTimePhasedSalesByCustomerGroup::buildParameters()
{
  ParameterList params;

  _customerGroup->appendValue(params);
  _productCategory->appendValue(params);

  XListViewItem *cursor = _periods->firstChild();
  QList<QVariant> periodList;
  while (cursor)
  {
    if (cursor->isSelected())
      periodList.append(cursor->id());

    cursor = cursor->nextSibling();
  }
  params.append("period_id_list", periodList);

  if (_bySales->isChecked())
    params.append("orderBySales");
  else /*if(_byCustomer->isChecked())*/
    params.append("orderByCustomer");

  return params;
}