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
 * (c) 1999-2008 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
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
 * Copyright (c) 1999-2008 by OpenMFG, LLC, d/b/a xTuple
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

#include "dspTimePhasedSalesByCustomerByItem.h"

#include <QVariant>
#include <QMessageBox>
#include <QStatusBar>
#include <QWorkspace>
#include <QMenu>
#include <q3valuevector.h>
#include <parameter.h>
#include <dbtools.h>
#include <datecluster.h>
#include <openreports.h>
#include "dspSalesHistoryByCustomer.h"
#include "guiclient.h"
#include "submitReport.h"

/*
 *  Constructs a dspTimePhasedSalesByCustomerByItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspTimePhasedSalesByCustomerByItem::dspTimePhasedSalesByCustomerByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_sohist, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*,int)));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_calendar, SIGNAL(newCalendarId(int)), _periods, SLOT(populate(int)));
  connect(_calendar, SIGNAL(select(ParameterList&)), _periods, SLOT(load(ParameterList&)));
  connect(_submit, SIGNAL(clicked()), this, SLOT(sSubmit()));
  
  if (!_metrics->boolean("EnableBatchManager"))
    _submit->hide();
  
  _customerType->setType(CustomerType);
  _productCategory->setType(ProductCategory);
  
  _sohist->addColumn(tr("Cust. #"),  _orderColumn, Qt::AlignLeft );
  _sohist->addColumn(tr("Customer"), 180,          Qt::AlignLeft );
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspTimePhasedSalesByCustomerByItem::~dspTimePhasedSalesByCustomerByItem()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspTimePhasedSalesByCustomerByItem::languageChange()
{
  retranslateUi(this);
}

void dspTimePhasedSalesByCustomerByItem::sPrint()
{
  if (_periods->isPeriodSelected())
  {
    orReport report("TimePhasedSalesHistoryByCustomerByItem", buildParameters());
    if (report.isValid())
      report.print();
    else
    {
      report.reportError(this);
      return;
    }
  }
  else
    QMessageBox::critical( this, tr("Incomplete criteria"),
                           tr( "The criteria you specified is not complete. Please make sure all\n"
                               "fields are correctly filled out before running the report." ) );
}

void dspTimePhasedSalesByCustomerByItem::sViewShipments()
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

void dspTimePhasedSalesByCustomerByItem::sPopulateMenu(QMenu *menuThis, QTreeWidgetItem *, int pColumn)
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

void dspTimePhasedSalesByCustomerByItem::sFillList()
{
  _sohist->clear();

  if (!_periods->isPeriodSelected())
    return;

  _sohist->clear();
  _sohist->setColumnCount(2);

  QString sql("SELECT cust_id, cust_number, cust_name");

  int columns = 1;
  QList<QTreeWidgetItem*> selected = _periods->selectedItems();
  for (int i = 0; i < selected.size(); i++)
  {
    PeriodListViewItem *cursor = (PeriodListViewItem*)selected[i];
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

    _sohist->addColumn(formatDate(cursor->startDate()), _qtyColumn, Qt::AlignRight);
    _columnDates.append(DatePair(cursor->startDate(), cursor->endDate()));
  }

  sql += " FROM cust ";

  if (_customerType->isSelected())
    sql += "WHERE (cust_custtype_id=:custtype_id)";
  else if (_customerType->isPattern())
    sql += "WHERE (cust_custtype_id IN (SELECT custtype_id FROM custtype WHERE (custtype_code ~ :custtype_pattern))) ";

  if (_byCustomer->isChecked())
    sql += "ORDER BY cust_number;";
  else if (_bySales->isChecked())
    sql += "ORDER BY bucket1 DESC;";

  q.prepare(sql);
  _customerType->bindValue(q);
  _productCategory->bindValue(q);
  q.exec();
  if (q.first())
  {
    Q3ValueVector<Numeric> totals(columns);;
    XTreeWidgetItem *last = 0;

    do
    {
      last = new XTreeWidgetItem( _sohist, last, q.value("cust_id").toInt(),
				 q.value("cust_number"), q.value("cust_name") );

      for (int column = 1; column < columns; column++)
      {
        QString bucketName = QString("bucket%1").arg(column);
        last->setText((column + 1), formatMoney(q.value(bucketName).toDouble()));
        totals[column] += q.value(bucketName).toDouble();
      }
    }
    while (q.next());

//  Add the totals row
    XTreeWidgetItem *total = new XTreeWidgetItem(_sohist, last, -1, QVariant(tr("Totals:")));
    for (int column = 1; column < columns; column++)
      total->setText((column + 1), formatMoney(totals[column].toDouble()));
  }
}

void dspTimePhasedSalesByCustomerByItem::sSubmit()
{
  if (_periods->isPeriodSelected())
  {
    ParameterList params(buildParameters());
    params.append("report_name", "TimePhasedSalesHistoryByCustomerByItem");
    
    submitReport newdlg(this, "", TRUE);
    newdlg.set(params);

    if (newdlg.check() == cNoReportDefinition)
      QMessageBox::critical( this, tr("Report Definition Not Found"),
                             tr( "The report defintions for this report, \"TimePhasedSalesHistoryByCustomerByItem\" cannot be found.\n"
                                 "Please contact your Systems Administrator and report this issue." ) );
    else
      newdlg.exec();

  }
  else
    QMessageBox::critical( this, tr("Incomplete criteria"),
                           tr( "The criteria you specified is not complete. Please make sure all\n"
                               "fields are correctly filled out before running the report." ) );
}

ParameterList dspTimePhasedSalesByCustomerByItem::buildParameters()
{
  ParameterList params;

  _customerType->appendValue(params);
  _productCategory->appendValue(params);

  QList<QTreeWidgetItem*> selected = _periods->selectedItems();
  QList<QVariant> periodList;
  for (int i = 0; i < selected.size(); i++)
    periodList.append(((XTreeWidgetItem*)selected[i])->id());

  params.append("period_id_list", periodList);

  if(_bySales->isChecked())
    params.append("orderBySales");
  else if(_byCustomer->isChecked())
    params.append("orderByCustomer");

  return params;
}
