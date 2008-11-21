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
 * The Original Code is xTuple ERP: PostBooks Edition 
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
 * Powered by xTuple ERP: PostBooks Edition
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

#include "dspBacklogByCustomer.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>

#include <metasql.h>
#include "mqlutil.h"

#include <openreports.h>

#include "salesOrder.h"
#include "salesOrderItem.h"
#include "printPackingList.h"

#define	AMOUNT_COL	8

dspBacklogByCustomer::dspBacklogByCustomer(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_soitem, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_showPrices, SIGNAL(toggled(bool)), this, SLOT(sHandlePrices(bool)));

  _cust->setFocus();

  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _soitem->setSelectionMode(QAbstractItemView::ExtendedSelection);
  _soitem->setRootIsDecorated(TRUE);
  _soitem->addColumn(tr("S/O #/Line #"),              _itemColumn,     Qt::AlignLeft   );
  _soitem->addColumn(tr("Cust. P/O #/Item Number"),   -1,              Qt::AlignLeft   );
  _soitem->addColumn(tr("Order"),                     _dateColumn,     Qt::AlignCenter );
  _soitem->addColumn(tr("Ship/Sched."),               _dateColumn,     Qt::AlignCenter );
  _soitem->addColumn(tr("Qty. UOM"),                  _qtyColumn,      Qt::AlignRight  );
  _soitem->addColumn(tr("Ordered"),                   _qtyColumn,      Qt::AlignRight  );
  _soitem->addColumn(tr("Shipped"),                   _qtyColumn,      Qt::AlignRight  );
  _soitem->addColumn(tr("Balance"),                   _qtyColumn,      Qt::AlignRight  );
  if (_privileges->check("ViewCustomerPrices") || _privileges->check("MaintainCustomerPrices"))
    _soitem->addColumn(tr("Ext. Price"),              _bigMoneyColumn, Qt::AlignRight  );

  _showPrices->setEnabled(_privileges->check("ViewCustomerPrices") || _privileges->check("MaintainCustomerPrices"));

  if (! _showPrices->isChecked())
    _soitem->hideColumn(AMOUNT_COL);

  _cust->setFocus();
}

dspBacklogByCustomer::~dspBacklogByCustomer()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspBacklogByCustomer::languageChange()
{
  retranslateUi(this);
}

void dspBacklogByCustomer::sHandlePrices(bool pShowPrices)
{
  if (pShowPrices)
    _soitem->showColumn(AMOUNT_COL);
  else
    _soitem->hideColumn(AMOUNT_COL);

  sFillList();
}

void dspBacklogByCustomer::sPrint()
{
  if (!_cust->isValid())
  {
    QMessageBox::warning( this, tr("Enter a Valid Customer Number"),
                          tr("You must enter a valid Customer Number for this report.") );
    _cust->setFocus();
    return;
  }

  ParameterList params;
  _warehouse->appendValue(params);
  _dates->appendValue(params);
  params.append("cust_id", _cust->id());
  params.append("openOnly");
  params.append("orderByScheddate");

  if (_showPrices->isChecked())
    params.append("showPrices");

  orReport report("BacklogByCustomer", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspBacklogByCustomer::sEditOrder()
{
  salesOrder::editSalesOrder(_soitem->id(), false);
}

void dspBacklogByCustomer::sViewOrder()
{
  salesOrder::viewSalesOrder(_soitem->id());
}

void dspBacklogByCustomer::sEditItem()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("soitem_id", _soitem->altId());
      
  salesOrderItem newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspBacklogByCustomer::sViewItem()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("soitem_id", _soitem->altId());
      
  salesOrderItem newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspBacklogByCustomer::sPrintPackingList()
{
  QList<QTreeWidgetItem*> selected = _soitem->selectedItems();
  for (int i = 0; i < selected.size(); i++)
  {
    XTreeWidgetItem *cursor = (XTreeWidgetItem*)(selected[i]);
    if (cursor->altId() == -1)
    {
      ParameterList params;
      params.append("sohead_id", cursor->id());

      printPackingList newdlg(this, "", TRUE);
      newdlg.set(params);
      newdlg.exec();
    }
  }
}

void dspBacklogByCustomer::sAddToPackingListBatch()
{
  QList<QTreeWidgetItem*> selected = _soitem->selectedItems();
  for (int i = 0; i < selected.size(); i++)
  {
    XTreeWidgetItem *cursor = (XTreeWidgetItem*)(selected[i]);
    if (cursor->altId() == -1)
    {
      q.prepare("SELECT addToPackingListBatch(:sohead_id) AS result;");
      q.bindValue(":sohead_id", cursor->id());
      q.exec();
      if (q.lastError().type() != QSqlError::None)
      {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return;
      }
    }
  }
}

void dspBacklogByCustomer::sPopulateMenu(QMenu *pMenu)
{
  bool hasParents     = FALSE;
  bool hasChildren    = FALSE;
  QList<QTreeWidgetItem*> selected = _soitem->selectedItems();
  for (int i = 0; i < selected.size(); i++)
  {
    XTreeWidgetItem *cursor = (XTreeWidgetItem*)(selected[i]);
    if ( (cursor->altId() == -1) && (!hasParents) )
      hasParents = TRUE;

    if ( (cursor->altId() != -1) && (!hasChildren) )
      hasChildren = TRUE;
  }

  int menuItem;

  if (selected.size() == 1)
  {
    menuItem = pMenu->insertItem(tr("Edit Order..."), this, SLOT(sEditOrder()), 0);
    if (!_privileges->check("MaintainSalesOrders"))
      pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem(tr("View Order..."), this, SLOT(sViewOrder()), 0);
    if ((!_privileges->check("MaintainSalesOrders")) && (!_privileges->check("ViewSalesOrders")))
      pMenu->setItemEnabled(menuItem, FALSE);

    if (hasChildren)
    {
      pMenu->insertSeparator();

      menuItem = pMenu->insertItem(tr("Edit Item..."), this, SLOT(sEditItem()), 0);
      if (!_privileges->check("MaintainSalesOrders"))
        pMenu->setItemEnabled(menuItem, FALSE);

      menuItem = pMenu->insertItem(tr("View Item..."), this, SLOT(sViewItem()), 0);
      if ((!_privileges->check("MaintainSalesOrders")) && (!_privileges->check("ViewSalesOrders")))
        pMenu->setItemEnabled(menuItem, FALSE);
    }
  }

  if (hasParents)
  {
    pMenu->insertSeparator();

    menuItem = pMenu->insertItem(tr("Print Packing List..."), this, SLOT(sPrintPackingList()), 0);
    if (!_privileges->check("PrintPackingLists"))
      pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem(tr("Add to Packing List Batch..."), this, SLOT(sAddToPackingListBatch()), 0);
    if (!_privileges->check("MaintainPackingListBatch"))
      pMenu->setItemEnabled(menuItem, FALSE);
  }
}

void dspBacklogByCustomer::sFillList()
{
  _soitem->clear();

  MetaSQLQuery mql = mqlLoad(":/so/displays/SalesOrderItems.mql");
  ParameterList params;
  _dates->appendValue(params);
  _warehouse->appendValue(params);
  params.append("cust_id", _cust->id());
  params.append("openOnly");
  params.append("orderByScheddate");
  q = mql.toQuery(params);

  if (q.first())
  {
    XTreeWidgetItem *head = NULL;
    int soheadid        = -1;
    double totalBacklog = 0.0;

    do
    {
      if (soheadid != q.value("cohead_id").toInt())
      {
        soheadid = q.value("cohead_id").toInt();

        head = new XTreeWidgetItem( _soitem, head, soheadid, -1,
                                  q.value("cohead_number"), q.value("cohead_custponumber"),
                                  formatDate(q.value("cohead_orderdate").toDate()),
                                  formatDate(q.value("min_scheddate").toDate()) );
      }

      new XTreeWidgetItem( head, soheadid, q.value("coitem_id").toInt(),
                         q.value("coitem_linenumber"), q.value("item_number"),
                         formatDate(q.value("cohead_orderdate").toDate()),
                         formatDate(q.value("coitem_scheddate").toDate()),
                         q.value("uom_name"), formatQty(q.value("coitem_qtyord").toDouble()),
                         formatQty(q.value("coitem_qtyshipped").toDouble()),
                         formatQty(q.value("qtybalance").toDouble()),
                         formatMoney(q.value("baseextpricebalance").toDouble()) );
      totalBacklog += q.value("baseextpricebalance").toDouble();
    }
    while (q.next());


    if (_showPrices->isChecked())
    {
      XTreeWidgetItem *totals = new XTreeWidgetItem(_soitem, head, -1, -1, tr("Total Backlog"));
      totals->setText(AMOUNT_COL, formatMoney(totalBacklog));
    }
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
