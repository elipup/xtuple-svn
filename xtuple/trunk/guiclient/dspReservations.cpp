/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspReservations.h"

#include <QAction>
#include <QMenu>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>

#include "mqlutil.h"
#include "salesOrder.h"
#include "transferOrder.h"
#include "workOrder.h"

dspReservations::dspReservations(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_allocations, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));

  _qoh->setPrecision(omfgThis->qtyVal());
  _available->setPrecision(omfgThis->qtyVal());

  _allocations->setRootIsDecorated(TRUE);
  _allocations->addColumn(tr("Order/Location LotSerial"), -1,             Qt::AlignLeft,   true,  "order_number"   );
  _allocations->addColumn(tr("Total Qty."),               _qtyColumn,     Qt::AlignRight,  true,  "totalqty"  );
  _allocations->addColumn(tr("Relieved"),                 _qtyColumn,     Qt::AlignRight,  true,  "relievedqty"  );
  _allocations->addColumn(tr("Reserved"),                 _qtyColumn,     Qt::AlignRight,  true,  "reservedqty"  );
  _allocations->addColumn(tr("Running Bal."),             _qtyColumn,     Qt::AlignRight,  true,  "balanceqty"  );
  _allocations->addColumn(tr("Required"),                 _dateColumn,    Qt::AlignCenter, true,  "scheddate" );

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
  XWidget::set(pParams);
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
  QAction *menuItem;

  if (QString(pSelected->text(0)) == "W/O")
  {
    menuItem = pMenu->addAction(tr("View Work Order..."), this, SLOT(sViewWorkOrder()));
    menuItem->setEnabled(_privileges->check("ViewWorkOrders"));
  }
  else if (QString(pSelected->text(0)) == "S/O")
  {
    menuItem = pMenu->addAction(tr("View Sales Order..."), this, SLOT(sViewCustomerOrder()));
    menuItem->setEnabled(_privileges->check("ViewSalesOrders"));

    menuItem = pMenu->addAction(tr("Edit Sales Order..."), this, SLOT(sEditCustomerOrder()));
    menuItem->setEnabled(_privileges->check("MaintainSalesOrders"));
  }
  else if (QString(pSelected->text(0)) == "T/O")
  {
    menuItem = pMenu->addAction(tr("View Transfer Order..."), this, SLOT(sViewTransferOrder()));
    menuItem->setEnabled(_privileges->check("ViewTransferOrders"));

    menuItem = pMenu->addAction(tr("Edit Transfer Order..."), this, SLOT(sEditTransferOrder()));
    menuItem->setEnabled(_privileges->check("MaintainTransferOrders"));
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
    MetaSQLQuery mql = mqlLoad("reservations", "detail");

    ParameterList params;
    params.append("warehous_id", _warehouse->id());
    params.append("item_id",	   _item->id());

    q = mql.toQuery(params);
    _allocations->populate(q);

    QString avails("SELECT itemsite_qtyonhand,"
                   "       qtyunreserved(itemsite_id) AS unreserved "
                   "FROM itemsite "
                   "WHERE ((itemsite_item_id=<? value(\"item_id\") ?>)"
                   "  AND  (itemsite_warehous_id=<? value(\"warehous_id\") ?>));");
    MetaSQLQuery availm(avails);
    q = availm.toQuery(params);
    if (q.first())
    {
      _qoh->setDouble(q.value("itemsite_qtyonhand").toDouble());
      _available->setDouble(q.value("unreserved").toDouble());
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}
