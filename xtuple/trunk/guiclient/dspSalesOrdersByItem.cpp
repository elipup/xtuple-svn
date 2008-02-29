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

#include "dspSalesOrdersByItem.h"

#include <QMenu>
#include <QMessageBox>
#include <QVariant>

#include "dspSalesOrderStatus.h"
#include "dspShipmentsBySalesOrder.h"
#include "returnAuthorization.h"
#include "salesOrder.h"

dspSalesOrdersByItem::dspSalesOrdersByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  connect(_item, SIGNAL(newId(int)), this, SLOT(sFillList()));
  connect(_so, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_dates, SIGNAL(updated()), this, SLOT(sFillList()));

  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setStartCaption(tr("Starting Order Date:"));
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);
  _dates->setEndCaption(tr("Ending Order Date:"));

  _item->setType(ItemLineEdit::cSold);

  _so->addColumn(tr("Order #"),    _orderColumn, Qt::AlignLeft   );
  _so->addColumn(tr("Order Date"), _dateColumn,  Qt::AlignCenter );
  _so->addColumn(tr("Customer"),   -1,           Qt::AlignLeft   );
  _so->addColumn(tr("Ordered"),    _qtyColumn,   Qt::AlignRight  );
  _so->addColumn(tr("Shipped"),    _qtyColumn,   Qt::AlignRight  );
  _so->addColumn(tr("Returned"),   _qtyColumn,   Qt::AlignRight  );
  _so->addColumn(tr("Balance"),    _qtyColumn,   Qt::AlignRight  );

  connect(omfgThis, SIGNAL(salesOrdersUpdated(int, bool)), this, SLOT(sFillList()));
}

dspSalesOrdersByItem::~dspSalesOrdersByItem()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspSalesOrdersByItem::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspSalesOrdersByItem::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
    _item->setId(param.toInt());

  param = pParams.value("startDate", &valid);
  if (valid)
    _dates->setStartDate(param.toDate());

  param = pParams.value("endDate", &valid);
  if (valid)
    _dates->setEndDate(param.toDate());

  return NoError;
}

void dspSalesOrdersByItem::sPopulateMenu(QMenu *menuThis)
{
  menuThis->insertItem(tr("Edit..."), this, SLOT(sEditOrder()), 0);
  menuThis->insertItem(tr("View..."), this, SLOT(sViewOrder()), 0);
  menuThis->insertSeparator();
  menuThis->insertItem(tr("Shipment Status..."), this, SLOT(sDspShipmentStatus()), 0);
  menuThis->insertItem(tr("Shipments.."), this, SLOT(sDspShipments()), 0);

  if (_privileges->check("MaintainReturns"))
  {
    menuThis->insertSeparator();
    menuThis->insertItem(tr("Create Return Authorization..."), this, SLOT(sCreateRA()));
  }
}

void dspSalesOrdersByItem::sEditOrder()
{
  salesOrder::editSalesOrder(_so->id(), false);
}

void dspSalesOrdersByItem::sViewOrder()
{
  salesOrder::viewSalesOrder(_so->id());
}

void dspSalesOrdersByItem::sCreateRA()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("sohead_id", _so->id());

  returnAuthorization *newdlg = new returnAuthorization();
  if (newdlg->set(params) == NoError)
    omfgThis->handleNewWindow(newdlg);
  else
    QMessageBox::critical(this, tr("Could Not Open Window"),
			  tr("The new Return Authorization could not be created"));
}

void dspSalesOrdersByItem::sDspShipmentStatus()
{
  ParameterList params;
  params.append("sohead_id", _so->id());
  params.append("run");

  dspSalesOrderStatus *newdlg = new dspSalesOrderStatus();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspSalesOrdersByItem::sDspShipments()
{
  ParameterList params;
  params.append("sohead_id", _so->id());
  params.append("run");

  dspShipmentsBySalesOrder *newdlg = new dspShipmentsBySalesOrder();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}


void dspSalesOrdersByItem::sFillList()
{
  if ((_item->isValid()) && (_dates->allValid()))
  {
    q.prepare( "SELECT cohead_id, cohead_number,"
               "       formatDate(cohead_orderdate), "
               "       cust_name,"
               "       formatQty(coitem_qtyord),"
               "       formatQty(coitem_qtyshipped),"
               "       formatQty(coitem_qtyreturned),"
               "       CASE WHEN (coitem_status='C') THEN :closed"
               "            ELSE formatQty(noNeg(coitem_qtyord - coitem_qtyshipped + coitem_qtyreturned))"
               "       END "
               "FROM cohead, coitem, itemsite, cust "
               "WHERE ( (coitem_cohead_id=cohead_id)"
               " AND (cohead_cust_id=cust_id)"
               " AND (coitem_itemsite_id=itemsite_id)"
               " AND (coitem_status<>'X')"
               " AND (cohead_orderdate BETWEEN :startDate AND :endDate)"
               " AND (itemsite_item_id=:item_id) ) "
               "ORDER BY cohead_number;" );
   _dates->bindValue(q);
    q.bindValue(":closed", tr("Closed"));
    q.bindValue(":item_id", _item->id());
    q.exec();
    _so->populate(q);
  }
  else
    _so->clear();
}

