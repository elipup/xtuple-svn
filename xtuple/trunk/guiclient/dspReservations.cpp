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

#include "dspReservations.h"

#include <QMenu>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>

#include "mqlutil.h"
#include "salesOrder.h"
#include "transferOrder.h"
#include "workOrder.h"

dspReservations::dspReservations(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  connect(_allocations, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));

  _item->setReadOnly(TRUE);
  _warehouse->setEnabled(FALSE);

  _allocations->setRootIsDecorated(TRUE);
  _allocations->addColumn(tr("Order/Location LotSerial"), -1,             Qt::AlignLeft   );
  _allocations->addColumn(tr("Total Qty."),               _qtyColumn,     Qt::AlignRight  );
  _allocations->addColumn(tr("Relieved"),                 _qtyColumn,     Qt::AlignRight  );
  _allocations->addColumn(tr("Reserved"),                 _qtyColumn,     Qt::AlignRight  );
  _allocations->addColumn(tr("Running Bal."),             _qtyColumn,     Qt::AlignRight  );
  _allocations->addColumn(tr("Required"),                 _dateColumn,    Qt::AlignCenter );

  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
}

dspReservations::~dspReservations()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspReservations::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspReservations::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("soitem_id", &valid);
  if (valid)
  {
    q.prepare("SELECT coitem_itemsite_id"
              "  FROM coitem"
              " WHERE(coitem_id=:soitem_id);");
    q.bindValue(":soitem_id", param.toInt());
    q.exec();
    if(q.first())
      _item->setItemsiteid(q.value("coitem_itemsite_id").toInt());
  }

  param = pParams.value("itemsite_id", &valid);
  if (valid)
    _item->setItemsiteid(param.toInt());

  if (pParams.inList("run"))
  {
    sFillList();
    return NoError_Run;
  }

  return NoError;
}

void dspReservations::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pSelected)
{
  int menuItem;

  if (QString(pSelected->text(0)) == "W/O")
  {
    menuItem = pMenu->insertItem(tr("View Work Order..."), this, SLOT(sViewWorkOrder()), 0);
    pMenu->setItemEnabled(menuItem, _privileges->check("ViewWorkOrders"));
  }
  else if (QString(pSelected->text(0)) == "S/O")
  {
    menuItem = pMenu->insertItem(tr("View Sales Order..."), this, SLOT(sViewCustomerOrder()), 0);
    pMenu->setItemEnabled(menuItem, _privileges->check("ViewSalesOrders"));

    pMenu->insertItem(tr("Edit Sales Order..."), this, SLOT(sEditCustomerOrder()), 0);
    pMenu->setItemEnabled(menuItem, _privileges->check("MaintainSalesOrders"));
  }
  else if (QString(pSelected->text(0)) == "T/O")
  {
    menuItem = pMenu->insertItem(tr("View Transfer Order..."), this, SLOT(sViewTransferOrder()), 0);
    pMenu->setItemEnabled(menuItem, _privileges->check("ViewTransferOrders"));

    pMenu->insertItem(tr("Edit Transfer Order..."), this, SLOT(sEditTransferOrder()), 0);
    pMenu->setItemEnabled(menuItem, _privileges->check("MaintainTransferOrders"));
  }
}

void dspReservations::sViewWorkOrder()
{
  q.prepare( "SELECT womatl_wo_id "
             "FROM womatl "
             "WHERE (womatl_id=:womatl_id);" );
  q.bindValue(":womatl_id", _allocations->id());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("mode", "view");
    params.append("wo_id", q.value("womatl_wo_id").toInt());
  
    workOrder *newdlg = new workOrder();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
}

void dspReservations::sViewCustomerOrder()
{
  q.prepare( "SELECT coitem_cohead_id "
             "FROM coitem "
             "WHERE (coitem_id=:coitem_id);" );
  q.bindValue(":coitem_id", _allocations->id());
  q.exec();
  if (q.first())
    salesOrder::viewSalesOrder(q.value("coitem_cohead_id").toInt());
}

void dspReservations::sEditCustomerOrder()
{
  q.prepare( "SELECT coitem_cohead_id "
             "FROM coitem "
             "WHERE (coitem_id=:coitem_id);" );
  q.bindValue(":coitem_id", _allocations->id());
  q.exec();
  if (q.first())
    salesOrder::editSalesOrder(q.value("coitem_cohead_id").toInt(), false);
}

void dspReservations::sViewTransferOrder()
{
  q.prepare( "SELECT toitem_tohead_id "
             "FROM toitem "
             "WHERE (toitem_id=:toitem_id);" );
  q.bindValue(":toitem_id", _allocations->id());
  q.exec();
  if (q.first())
    transferOrder::viewTransferOrder(q.value("toitem_tohead_id").toInt());
}

void dspReservations::sEditTransferOrder()
{
  q.prepare( "SELECT toitem_tohead_id "
             "FROM toitem "
             "WHERE (toitem_id=:toitem_id);" );
  q.bindValue(":toitem_id", _allocations->id());
  q.exec();
  if (q.first())
    transferOrder::editTransferOrder(q.value("toitem_tohead_id").toInt(), false);
}

void dspReservations::sFillList()
{
  _allocations->clear();

  if (_item->isValid())
  {
    XSqlQuery itemlocrsrv;
    itemlocrsrv.prepare( "SELECT (formatLocationName(itemloc_location_id) || ' ' || "
                         "       formatLotSerialNumber(itemloc_ls_id)) AS f_locationlot,"
						 "       formatQty(itemlocrsrv_qty) AS f_reserved "
                         "FROM itemlocrsrv JOIN itemloc ON (itemloc_id=itemlocrsrv_itemloc_id) "
                         "WHERE ( (itemlocrsrv_source='SO')"
					     "  AND   (itemlocrsrv_source_id=:source_id) ) "
						 "ORDER BY formatLocationName(itemloc_location_id),"
						 "         formatLotSerialNumber(itemloc_ls_id);" );

    MetaSQLQuery mql = mqlLoad(":so/displays/Reservations/FillListDetail.mql");

    ParameterList params;
    params.append("warehous_id", _warehouse->id());
    params.append("item_id",	   _item->id());

    q = mql.toQuery(params);

    double runningBal = 0;
    XTreeWidgetItem *last = 0;
    XTreeWidgetItem *document = 0;

    while (q.next())
    {
      runningBal += q.value("coitem_qtyreserved").toDouble();

      last = document = new XTreeWidgetItem(_allocations, last,
				            q.value("source_id").toInt(),
				            q.value("order_number"),
				            q.value("totalqty"),
				            q.value("relievedqty"), q.value("balanceqty"),
				            formatQty(runningBal), q.value("duedate") );
      last->setTextColor(5, "red");

      if(_metrics->boolean("EnableSOReservationsByLocation"))
	  {
        itemlocrsrv.bindValue(":source_id", q.value("source_id").toInt());
	    itemlocrsrv.exec();
	    if (itemlocrsrv.first())
	    {
	      do
		  {
            new XTreeWidgetItem( document,
                                 -1,
                                 itemlocrsrv.value("f_locationlot"),
                                 "",
                                 "", itemlocrsrv.value("f_reserved"),
                                 "", "" );
          }
		  while (itemlocrsrv.next());
        }
	  }
    }
    if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    QString avails("SELECT formatQty(itemsite_qtyonhand) AS f_qoh,"
		   "       formatQty(qtyunreserved(itemsite_id)) AS f_unreserved "
		   "FROM itemsite "
		   "WHERE ((itemsite_item_id=<? value(\"item_id\") ?>)"
		   "  AND  (itemsite_warehous_id=<? value(\"warehous_id\") ?>));");
    MetaSQLQuery availm(avails);
    q = availm.toQuery(params);
    if (q.first())
    {
      _qoh->setText(q.value("f_qoh").toString());
      _available->setText(q.value("f_unreserved").toString());
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}
