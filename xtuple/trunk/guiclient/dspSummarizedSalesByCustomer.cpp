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

#include "dspSummarizedSalesByCustomer.h"

#include <QVariant>
#include <QWorkspace>
#include <QStatusBar>
#include <parameter.h>
#include <QMessageBox>
#include "dspSalesHistoryByCustomer.h"
#include "rptSummarizedSalesByCustomer.h"

/*
 *  Constructs a dspSummarizedSalesByCustomer as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspSummarizedSalesByCustomer::dspSummarizedSalesByCustomer(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
    connect(_cohist, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspSummarizedSalesByCustomer::~dspSummarizedSalesByCustomer()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspSummarizedSalesByCustomer::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <QMenu>

void dspSummarizedSalesByCustomer::init()
{
  statusBar()->hide();

  _productCategory->setType(ProductCategory);
  _currency->setType(Currency);

  _cohist->addColumn(tr("Customer"),    -1,          Qt::AlignLeft   );
  _cohist->addColumn(tr("First Sale"),  _dateColumn, Qt::AlignCenter );
  _cohist->addColumn(tr("Last Sale"),   _dateColumn, Qt::AlignCenter );
  _cohist->addColumn(tr("Total Qty."),  _qtyColumn,  Qt::AlignRight  );
  _cohist->addColumn(tr("Total Sales"), _qtyColumn,  Qt::AlignRight  );
  _cohist->addColumn(tr("Currency"),    _currencyColumn, Qt::AlignLeft );

  if (omfgThis->singleCurrency())
      _cohist->hideColumn(5);
}

void dspSummarizedSalesByCustomer::sPopulateMenu(QMenu *menuThis)
{
  menuThis->insertItem(tr("View Sales Detail..."), this, SLOT(sViewDetail()), 0);
}

void dspSummarizedSalesByCustomer::sViewDetail()
{
  ParameterList params;
  _warehouse->appendValue(params);
  _productCategory->appendValue(params);
  _dates->appendValue(params);
  params.append("cust_id", _cohist->id());
  params.append("run");

  dspSalesHistoryByCustomer *newdlg = new dspSalesHistoryByCustomer();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspSummarizedSalesByCustomer::sPrint()
{
  ParameterList params;
  _warehouse->appendValue(params);
  _productCategory->appendValue(params);
  _dates->appendValue(params);
  params.append("print");

  rptSummarizedSalesByCustomer newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspSummarizedSalesByCustomer::sFillList()
{
  if (!checkParameters())
    return;

  QString sql( "SELECT cust_id, (cust_number || '-' || cust_name),"
               "       formatDate(MIN(cohist_invcdate)), formatDate(MAX(cohist_invcdate)),"
               "       formatQty(SUM(cohist_qtyshipped)),"
               "       formatMoney(SUM(cohist_qtyshipped * "
	       "		       currToCurr(cohist_curr_id, cust_curr_id,"
	       "				  cohist_unitprice, 'now'))),"
	       "       currConcat(cust_curr_id) "
               "FROM cohist, itemsite, cust, item "
               "WHERE ((cohist_itemsite_id=itemsite_id)"
               " AND (cohist_cust_id=cust_id)"
               " AND (itemsite_item_id=item_id)"
               " AND (cohist_invcdate BETWEEN :startDate AND :endDate)" );

  if (_productCategory->isSelected())
    sql += " AND (item_prodcat_id=:prodcat_id)";
  else if (_productCategory->isPattern())
    sql += " AND (item_prodcat_id IN (SELECT prodcat_id FROM prodcat WHERE (prodcat_code ~ :prodcat_pattern)))";
  
  if (_currency->isSelected())
      sql += " AND cust_curr_id = :curr_id ";
  else if (_currency->isPattern())
      sql += " AND cust_curr_id IN (SELECT curr_id FROM curr_symbol WHERE currConcat(curr_id) ~ :currConcat_pattern) ";

  if (_warehouse->isSelected())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  sql += ") "
         "GROUP BY cust_number, cust_id, cust_name, cust_curr_id";

  q.prepare(sql);
  _warehouse->bindValue(q);
  _currency->bindValue(q);
  _productCategory->bindValue(q);
  _dates->bindValue(q);
  q.exec();
  _cohist->populate(q);
}

bool dspSummarizedSalesByCustomer::checkParameters()
{
  if (!_dates->startDate().isValid())
  {
    if(isVisible()) {
      QMessageBox::warning( this, tr("Enter Start Date"),
                            tr("Please enter a valid Start Date.") );
      _dates->setFocus();
    }
    return FALSE;
  }

  if (!_dates->endDate().isValid())
  {
    if(isVisible()) {
      QMessageBox::warning( this, tr("Enter End Date"),
                            tr("Please enter a valid End Date.") );
      _dates->setFocus();
    }
    return FALSE;
  }

  return TRUE;
}
