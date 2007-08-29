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

#include "dspPartiallyShippedOrders.h"

#include <QVariant>
#include <QStatusBar>
#include <parameter.h>
#include <QWorkspace>
#include "salesOrder.h"
#include "salesOrderItem.h"
#include "printPackingList.h"
#include "rptPartiallyShippedOrders.h"
#include "OpenMFGGUIClient.h"

/*
 *  Constructs a dspPartiallyShippedOrders as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspPartiallyShippedOrders::dspPartiallyShippedOrders(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_so, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_showPrices, SIGNAL(toggled(bool)), this, SLOT(sHandlePrices(bool)));
    connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspPartiallyShippedOrders::~dspPartiallyShippedOrders()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspPartiallyShippedOrders::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <QMenu>
#include <QSqlError>

void dspPartiallyShippedOrders::init()
{
  statusBar()->hide();

  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _so->addColumn(tr("Hold"),        0,            Qt::AlignCenter );
  _so->addColumn(tr("S/O #"),       _orderColumn, Qt::AlignRight  );
  _so->addColumn(tr("Customer"),    -1,           Qt::AlignLeft   );
  _so->addColumn(tr("Hold Type"),   _dateColumn,  Qt::AlignCenter );
  _so->addColumn(tr("Ordered"),     _dateColumn,  Qt::AlignRight  );
  _so->addColumn(tr("Scheduled"),   _dateColumn,  Qt::AlignRight  );
  _so->addColumn(tr("Pack Date"),   _dateColumn,  Qt::AlignRight  );
  _so->addColumn(tr("Amount"),      _moneyColumn, Qt::AlignRight  );
  _so->addColumn(tr("Currency"),    _currencyColumn, Qt::AlignLeft);

  _so->hideColumn(7);
  _so->hideColumn(8);

  if ( (!_privleges->check("ViewCustomerPrices")) && (!_privleges->check("MaintainCustomerPrices")) )
    _showPrices->setEnabled(FALSE);

  sFillList();
}

void dspPartiallyShippedOrders::sHandlePrices(bool pShowPrices)
{
  if (pShowPrices)
  {
    _so->showColumn(7);
    if (!omfgThis->singleCurrency())
      _so->showColumn(8);
  }
  else
  {
    _so->hideColumn(7);
    _so->hideColumn(8);
  }

  sFillList();
}

void dspPartiallyShippedOrders::sPrint()
{
  ParameterList params;
  _warehouse->appendValue(params);
  _dates->appendValue(params);
  params.append("print");

  if (_showPrices->isChecked())
    params.append("showPrices");

  rptPartiallyShippedOrders newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspPartiallyShippedOrders::sEditOrder()
{
  salesOrder::editSalesOrder(_so->altId(), false);
}

void dspPartiallyShippedOrders::sViewOrder()
{
  salesOrder::viewSalesOrder(_so->altId());
}

void dspPartiallyShippedOrders::sPrintPackingList()
{
  ParameterList params;
  params.append("sohead_id", _so->altId());

  printPackingList newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspPartiallyShippedOrders::sPopulateMenu(QMenu *pMenu)
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("Edit Order..."), this, SLOT(sEditOrder()), 0);
  if (!_privleges->check("MaintainSalesOrders"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("View Order..."), this, SLOT(sViewOrder()), 0);
  if ((!_privleges->check("MaintainSalesOrders")) && (!_privleges->check("ViewSalesOrders")))
    pMenu->setItemEnabled(menuItem, FALSE);

  pMenu->insertSeparator();

  if ( (_so->currentItem()->text(0) != "P") && (_so->currentItem()->text(0) != "C") )
  {
    menuItem = pMenu->insertItem(tr("Print Packing List..."), this, SLOT(sPrintPackingList()), 0);
    if (!_privleges->check("PrintPackingLists"))
      pMenu->setItemEnabled(menuItem, FALSE);
  }
}

void dspPartiallyShippedOrders::sFillList()
{
  _so->clear();

  if (_dates->allValid())
  {
    QString sql( "SELECT CASE WHEN (cohead_holdtype IN ('P', 'C')) THEN -1"
                 "            ELSE cohead_id"
                 "       END AS _coheadid, cohead_id,"
                 "       cohead_holdtype, cohead_number, cust_name,"
                 "       CASE WHEN (cohead_holdtype='N') THEN :none"
                 "            WHEN (cohead_holdtype='C') THEN :credit"
                 "            WHEN (cohead_holdtype='S') THEN :ship"
                 "            WHEN (cohead_holdtype='P') THEN :pack"
                 "            ELSE :other"
                 "       END AS f_holdtype,"
                 "       formatDate(cohead_orderdate) AS f_orderdate,"
                 "       formatDate(MIN(coitem_scheddate)) AS f_scheddate,"
                 "       formatDate(cohead_packdate) AS f_packdate,"
                 "       formatMoney( SUM( noNeg(coitem_qtyord - coitem_qtyshipped + coitem_qtyreturned) *"
                 "                         (coitem_price / item_invpricerat) ) ) AS f_extprice,"
		 "       currConcat(cohead_curr_id) AS currAbbr,"
                 "       SUM( noNeg(coitem_qtyord - coitem_qtyshipped + coitem_qtyreturned) *"
                 "            (coitem_price / item_invpricerat) ) AS backlog,"
                 "       MIN(coitem_scheddate) AS scheddate "
                 "FROM cohead, itemsite, item, cust, coitem "
                 "WHERE ( (coitem_cohead_id=cohead_id)"
                 " AND (cohead_cust_id=cust_id)"
                 " AND (coitem_itemsite_id=itemsite_id)"
                 " AND (itemsite_item_id=item_id)"
                 " AND (coitem_status='O')"
                 " AND (cohead_id IN ( SELECT DISTINCT coitem_cohead_id"
                 "                     FROM coitem"
                 "                     WHERE (coitem_qtyshipped > 0) ))"
                 " AND (coitem_qtyshipped < coitem_qtyord)"
                 " AND (coitem_scheddate BETWEEN :startDate AND :endDate)" );

    if (_warehouse->isSelected())
      sql += " AND (itemsite_warehous_id=:warehous_id)";

    sql += ") "
           "GROUP BY cohead_id, cohead_number, cust_name, cohead_holdtype,"
           "         cohead_orderdate, cohead_packdate, cohead_curr_id "
           "ORDER BY scheddate, cohead_number;";

    q.prepare(sql);
    _warehouse->bindValue(q);
    _dates->bindValue(q);
    q.bindValue(":none", tr("None"));
    q.bindValue(":credit", tr("Credit"));
    q.bindValue(":ship", tr("Ship"));
    q.bindValue(":pack", tr("Pack"));
    q.bindValue(":other", tr("Other"));
    q.exec();
    if (q.first())
    {
      XTreeWidgetItem *last = NULL;

      do
      {
        last = new XTreeWidgetItem( _so, last, q.value("_coheadid").toInt(), q.value("cohead_id").toInt(),
                                  q.value("cohead_holdtype").toString(), q.value("cohead_number").toString(),
                                  q.value("cust_name").toString(), q.value("f_holdtype").toString(),
                                  q.value("f_orderdate").toString(), q.value("f_scheddate").toString(),
                                  q.value("f_packdate").toString(), q.value("f_extprice").toString(),
				  q.value("currAbbr"));
      }
      while (q.next());

      _so->setDragString("soheadid=");

      if (_showPrices->isChecked())
      {
	sql = "SELECT SUM(currToBase(cohead_curr_id,"
	      "         noNeg(coitem_qtyord - coitem_qtyshipped + coitem_qtyreturned) *"
	      "         (coitem_price / item_invpricerat), CURRENT_DATE)) AS backlog,"
	      "     currConcat(baseCurrId()) AS currAbbr "
	      "FROM cohead, itemsite, item, coitem "
	      "WHERE ( (coitem_cohead_id=cohead_id)"
	      " AND (coitem_itemsite_id=itemsite_id)"
	      " AND (itemsite_item_id=item_id)"
	      " AND (coitem_status='O')"
	      " AND (cohead_id IN ( SELECT DISTINCT coitem_cohead_id"
	      "                     FROM coitem"
	      "                     WHERE (coitem_qtyshipped > 0) ))"
	      " AND (coitem_qtyshipped < coitem_qtyord)"
	      " AND (coitem_scheddate BETWEEN :startDate AND :endDate)";

	if (_warehouse->isSelected())
	  sql += " AND (itemsite_warehous_id=:warehous_id)";
	sql += ") GROUP BY currAbbr";

	q.prepare(sql);
	_warehouse->bindValue(q);
	_dates->bindValue(q);
	q.exec();
	if (q.first())
	    new XTreeWidgetItem( _so, last, -1, -1,
			       "", "", tr("Total Backlog"), "", "", "", "",
			       formatMoney(q.value("backlog").toDouble()),
			       q.value("currAbbr"));
	else if (q.lastError().type() != QSqlError::NoError)
	    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      }
    }
  }
}

