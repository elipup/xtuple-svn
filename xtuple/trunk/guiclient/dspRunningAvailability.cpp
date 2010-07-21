/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspRunningAvailability.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include <openreports.h>

#include "dspWoScheduleByWorkOrder.h"
#include "firmPlannedOrder.h"
#include "mqlutil.h"
#include "purchaseRequest.h"
#include "salesOrder.h"
#include "transferOrder.h"
#include "workOrder.h"
#include "purchaseOrder.h"

dspRunningAvailability::dspRunningAvailability(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  _ready = true;

  connect(_availability, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));
  connect(_availability,SIGNAL(resorted()), this, SLOT(sHandleResort()));
  connect(_item,	SIGNAL(newId(int)), this, SLOT(sFillList()));
  connect(_print,	SIGNAL(clicked()),  this, SLOT(sPrint()));
  connect(_showPlanned,	SIGNAL(clicked()),  this, SLOT(sFillList()));
  connect(_warehouse,	SIGNAL(newID(int)), this, SLOT(sFillList()));

  _availability->addColumn(tr("Order Type"),    _itemColumn, Qt::AlignLeft,  true, "ordertype");
  _availability->addColumn(tr("Order #"),       _itemColumn, Qt::AlignLeft,  true, "ordernumber");
  _availability->addColumn(tr("Source/Destination"),     -1, Qt::AlignLeft,  true, "item_number");
  _availability->addColumn(tr("Due Date"),      _dateColumn, Qt::AlignLeft,  true, "duedate");
  _availability->addColumn(tr("Ordered"),        _qtyColumn, Qt::AlignRight, true, "qtyordered");
  _availability->addColumn(tr("Received"),       _qtyColumn, Qt::AlignRight, true, "qtyreceived");
  _availability->addColumn(tr("Balance"),        _qtyColumn, Qt::AlignRight, true, "balance");
  _availability->addColumn(tr("Running Avail."), _qtyColumn, Qt::AlignRight, true, "runningavail");

  _qoh->setValidator(omfgThis->qtyVal());
  _reorderLevel->setValidator(omfgThis->qtyVal());
  _orderMultiple->setValidator(omfgThis->qtyVal());
  _orderToQty->setValidator(omfgThis->qtyVal());

  connect(omfgThis, SIGNAL(workOrdersUpdated(int, bool)), this, SLOT(sFillList()));

  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
}

dspRunningAvailability::~dspRunningAvailability()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspRunningAvailability::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspRunningAvailability::set(const ParameterList &pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("itemsite_id", &valid);
  if (valid)
  {
    _ready = false;
    _item->setItemsiteid(param.toInt());
    _item->setReadOnly(TRUE);
    _showPlanned->setFocus();
    _showPlanned->init();
    _ready = true;
    sFillList();
  }

  return NoError;
}

void dspRunningAvailability::setParams(ParameterList & params)
{
  params.append("item_id",	_item->id());
  params.append("warehous_id",	_warehouse->id());

  params.append("firmPo",	tr("Planned P/O (firmed)"));
  params.append("plannedPo",	tr("Planned P/O"));
  params.append("firmWo",	tr("Planned W/O (firmed)"));
  params.append("plannedWo",	tr("Planned W/O"));
  params.append("firmTo",	tr("Planned T/O (firmed)"));
  params.append("plannedTo",	tr("Planned T/O"));
  params.append("firmWoReq",	tr("Planned W/O Req. (firmed)"));
  params.append("plannedWoReq",	tr("Planned W/O Req."));
  params.append("pr",		tr("Purchase Request"));

  if (_showPlanned->isChecked())
    params.append("showPlanned");

  if (_metrics->boolean("MultiWhs"))
    params.append("MultiWhs");

  if (_metrics->value("Application") == "Standard")
  {
    XSqlQuery xtmfg;
    xtmfg.exec("SELECT pkghead_name FROM pkghead WHERE pkghead_name='xtmfg'");
    if (xtmfg.first())
      params.append("Manufacturing");
    params.append("showMRPplan");
  }
}

void dspRunningAvailability::sPrint()
{
  ParameterList params;
  setParams(params);
  params.append("includeFormatted");

  orReport report("RunningAvailability", params);
  if (report.isValid())
      report.print();
  else
    report.reportError(this);
}

void dspRunningAvailability::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pSelected)
{
  QAction *menuItem;
  
  QString ordertype = pSelected->text(_availability->column("ordertype"));

  if (ordertype == tr("Planned W/O (firmed)") ||
      ordertype == tr("Planned W/O") ||
      ordertype == tr("Planned P/O (firmed)") ||
      ordertype == tr("Planned P/O") )
  {
    if (ordertype == tr("Planned W/O (firmed)") ||
	ordertype == tr("Planned P/O (firmed)") )
      pMenu->addAction(tr("Soften Order..."), this, SLOT(sSoftenOrder()));
    else
      pMenu->addAction(tr("Firm Order..."), this, SLOT(sFirmOrder()));
 
    pMenu->addAction(tr("Release Order..."), this, SLOT(sReleaseOrder()));
    pMenu->addAction(tr("Delete Order..."), this, SLOT(sDeleteOrder()));
  }
  
  else if (ordertype.contains("W/O") &&
	  !(ordertype == tr("Planned W/O Req. (firmed)") || ordertype == tr("Planned W/O Req.")))
  {
    pMenu->addAction(tr("View Work Order Details..."), this, SLOT(sViewWo()));
    menuItem = pMenu->addAction(tr("Work Order Schedule..."), this, SLOT(sDspWoScheduleByWorkOrder()));
    menuItem->setEnabled( _privileges->check("MaintainWorkOrders") ||
				    _privileges->check("ViewWorkOrders"));
  }
  else if (ordertype == "S/O")
  {
    menuItem = pMenu->addAction(tr("View Sales Order..."), this, SLOT(sViewSo()));
    menuItem->setEnabled(_privileges->check("ViewSalesOrders"));
  }

  else if (ordertype == "T/O")
  {
    menuItem = pMenu->addAction(tr("View Transfer Order..."), this, SLOT(sViewTo()));
    menuItem->setEnabled(_privileges->check("ViewTransferOrders"));
  }

  else if (ordertype == "P/O")
  {
    menuItem = pMenu->addAction(tr("View Purchase Order..."), this, SLOT(sViewPo()));
    menuItem->setEnabled(_privileges->check("ViewPurchaseOrders") || _privileges->check("MaintainPurchaseOrders"));
  }

}

void dspRunningAvailability::sFirmOrder()
{
  ParameterList params;
  params.append("planord_id", _availability->id());

  firmPlannedOrder newdlg(this, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspRunningAvailability::sSoftenOrder()
{
  q.prepare( "UPDATE planord "
             "SET planord_firm=FALSE "
             "WHERE (planord_id=:planord_id);" );
  q.bindValue(":planord_id", _availability->id());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void dspRunningAvailability::sReleaseOrder()
{
  // TODO
  if (_availability->currentItem()->text(_availability->column("ordertype")) == tr("Planned W/O (firmed)") ||
      _availability->currentItem()->text(_availability->column("ordertype")) == tr("Planned W/O"))
  {
    ParameterList params;
    params.append("mode", "release");
    params.append("planord_id", _availability->id());

    workOrder *newdlg = new workOrder();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
#if 0
    if (newdlg.exec() != XDialog::Rejected)
    {
      sDeleteOrder();
      sFillList();
    }
#endif
  }
  else if (_availability->currentItem()->text(_availability->column("ordertype")) == tr("Planned P/O (firmed)") ||
	  _availability->currentItem()->text(_availability->column("ordertype")) == tr("Planned P/O"))
  {
    ParameterList params;
    params.append("mode", "release");
    params.append("planord_id", _availability->id());

    purchaseRequest newdlg(this, "", TRUE);
    newdlg.set(params);

    if (newdlg.exec() != XDialog::Rejected)
      sFillList();
  }
}

void dspRunningAvailability::sDeleteOrder()
{
  q.prepare( "SELECT deletePlannedOrder(:planord_id, FALSE) AS result;" );
  q.bindValue(":planord_id", _availability->id());
  q.exec();
  if (q.first())
  {
    /* TODO: uncomment when deletePlannedOrder returns INTEGER instead of BOOLEAN
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("deletePlannedOrder", result), __FILE__, __LINE__);
      return;
    }
    */
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void dspRunningAvailability::sViewSo()
{
  ParameterList params;
  salesOrder::viewSalesOrder(_availability->id());
}

void dspRunningAvailability::sViewTo()
{
  ParameterList params;
  transferOrder::viewTransferOrder(_availability->id());
}

void dspRunningAvailability::sViewWo()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("wo_id", _availability->id());

  workOrder *newdlg = new workOrder();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspRunningAvailability::sViewPo()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("pohead_id", _availability->id());

  purchaseOrder *newdlg = new purchaseOrder();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspRunningAvailability::sFillList()
{
  if (_item->isValid() && _ready)
  {
    q.prepare( "SELECT itemsite_qtyonhand,"
               "       CASE WHEN(itemsite_useparams) THEN itemsite_reorderlevel ELSE 0.0 END AS reorderlevel,"
               "       CASE WHEN(itemsite_useparams) THEN itemsite_ordertoqty   ELSE 0.0 END AS ordertoqty,"
               "       CASE WHEN(itemsite_useparams) THEN itemsite_multordqty   ELSE 0.0 END AS multorderqty "
               "FROM item, itemsite "
               "WHERE ( (itemsite_item_id=item_id)"
               " AND (itemsite_warehous_id=:warehous_id)"
               " AND (item_id=:item_id) );" );
    q.bindValue(":item_id", _item->id());
    q.bindValue(":warehous_id", _warehouse->id());
    q.exec();
    if (q.first())
    {
      _qoh->setDouble(q.value("itemsite_qtyonhand").toDouble());
      _reorderLevel->setDouble(q.value("reorderlevel").toDouble());
      _orderMultiple->setDouble(q.value("multorderqty").toDouble());
      _orderToQty->setDouble(q.value("ordertoqty").toDouble());

      MetaSQLQuery mql = mqlLoad("runningAvailability", "detail");
      ParameterList params;
      setParams(params);
      params.append("qoh",          q.value("itemsite_qtyonhand").toDouble());

      q = mql.toQuery(params);
      _availability->populate(q, true);
      if (q.lastError().type() != QSqlError::NoError)
      {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return;
      }
      sHandleResort();
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else
  {
    _qoh->setText("0.00");
    _reorderLevel->setText("0.00");
    _orderMultiple->setText("0.00");
    _orderToQty->setText("0.00");
  }
}

void dspRunningAvailability::sDspWoScheduleByWorkOrder()
{
  ParameterList params;
  params.append("wo_id", _availability->id());
  params.append("run");

  dspWoScheduleByWorkOrder *newdlg = new dspWoScheduleByWorkOrder();
  SetResponse setresp = newdlg->set(params);
  if (setresp == NoError || setresp == NoError_Run)
    omfgThis->handleNewWindow(newdlg);
}

void dspRunningAvailability::sHandleResort()
{
  for (int i = 0; i < _availability->topLevelItemCount(); i++)
  {
    XTreeWidgetItem *item = _availability->topLevelItem(i);
    if (item->data(_availability->column("runningavail"), Qt::DisplayRole).toDouble() < 0)
      item->setTextColor(_availability->column("runningavail"), namedColor("error"));
    else if (item->data(_availability->column("runningavail"), Qt::DisplayRole).toDouble() < _reorderLevel->toDouble())
      item->setTextColor(_availability->column("runningavail"), namedColor("warning"));
    else
      item->setTextColor(_availability->column("runningavail"), namedColor(""));
  }
}
