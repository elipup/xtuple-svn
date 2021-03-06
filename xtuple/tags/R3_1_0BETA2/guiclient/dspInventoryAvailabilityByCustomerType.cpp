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

#include "dspInventoryAvailabilityByCustomerType.h"

#include <QVariant>
#include <QSqlError>
#include <QMessageBox>
#include "inputManager.h"
#include "dspAllocations.h"
#include "dspOrders.h"
#include "dspRunningAvailability.h"
#include "workOrder.h"
#include "purchaseOrder.h"
#include "createCountTagsByItem.h"
#include "dspSubstituteAvailabilityByItem.h"
#include "salesOrderList.h"
#include "reserveSalesOrderItem.h"
#include "storedProcErrorLookup.h"
#include "dspReservations.h"

#include <openreports.h>
#include <metasql.h>
#include <parameter.h>


/*
 *  Constructs a dspInventoryAvailabilityByCustomerType as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspInventoryAvailabilityByCustomerType::dspInventoryAvailabilityByCustomerType(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_onlyShowShortages, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_showWoSupply, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_custtype, SIGNAL(updated()), this, SLOT(sFillList()));
  connect(_avail, SIGNAL(populateMenu(QMenu*, QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*, QTreeWidgetItem*)));
  connect(_autoupdate, SIGNAL(toggled(bool)), this, SLOT(sAutoUpdateToggled(bool)));

  _custtype->setType(ParameterGroup::CustomerType);

  _avail->setRootIsDecorated(TRUE);
  _avail->addColumn(tr("Item Number"),  _itemColumn, Qt::AlignLeft   );
  _avail->addColumn(tr("Description"),  -1,          Qt::AlignLeft   );
  _avail->addColumn(tr("UOM"),          _uomColumn,  Qt::AlignCenter );
  _avail->addColumn(tr("QOH"),          _qtyColumn,  Qt::AlignRight  );
  _avail->addColumn(tr("This Alloc."),  _qtyColumn,  Qt::AlignRight  );
  _avail->addColumn(tr("Total Alloc."), _qtyColumn,  Qt::AlignRight  );
  _avail->addColumn(tr("Orders"),       _qtyColumn,  Qt::AlignRight  );
  _avail->addColumn(tr("This Avail."),  _qtyColumn,  Qt::AlignRight  );
  _avail->addColumn(tr("Total Avail."), _qtyColumn,  Qt::AlignRight  );
  _avail->addColumn(tr("At Shipping"),  _qtyColumn,  Qt::AlignRight  );
  _avail->addColumn(tr("Sched. Date"),  _dateColumn, Qt::AlignCenter );
  _avail->setIndentation(10);

  if(!_metrics->boolean("EnableSOReservations"))
  {
    _useReservationNetting->hide();
    _useReservationNetting->setEnabled(false);
  }
  else
  {
    connect(_useReservationNetting, SIGNAL(toggled(bool)), this, SLOT(sHandleReservationNetting(bool)));
    if(_useReservationNetting->isChecked())
      sHandleReservationNetting(true);
  }
  connect(omfgThis, SIGNAL(workOrdersUpdated(int, bool)), this, SLOT(sFillList()));
  if(_autoupdate->isChecked())
    sAutoUpdateToggled(true);

  sFillList();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspInventoryAvailabilityByCustomerType::~dspInventoryAvailabilityByCustomerType()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspInventoryAvailabilityByCustomerType::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspInventoryAvailabilityByCustomerType::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("onlyShowShortages", &valid);
  if (valid)
    _onlyShowShortages->setChecked(TRUE);

  return NoError;
}

void dspInventoryAvailabilityByCustomerType::sPrint()
{
  ParameterList params;

  _custtype->appendValue(params);

  if(_onlyShowShortages->isChecked())
    params.append("onlyShowShortages");

  orReport report("InventoryAvailabilityByCustomerType", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspInventoryAvailabilityByCustomerType::sPopulateMenu(QMenu *pMenu,  QTreeWidgetItem *selected)
{
  int menuItem;
  
  if (_avail->altId() == -2)
  {
    menuItem = pMenu->insertItem(tr("Add to Packing List Batch"), this, SLOT(sAddToPackingListBatch()));
  }
  else if (_avail->altId() != -1)
  {
    menuItem = pMenu->insertItem("View Allocations...", this, SLOT(sViewAllocations()), 0);
    if (selected->text(5).toDouble() == 0.0)
      pMenu->setItemEnabled(menuItem, FALSE);
    
    menuItem = pMenu->insertItem("View Orders...", this, SLOT(sViewOrders()), 0);
    if (selected->text(6).toDouble() == 0.0)
     pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem("Running Availability...", this, SLOT(sRunningAvailability()), 0);
    menuItem = pMenu->insertItem("Substitute Availability...", this, SLOT(sViewSubstituteAvailability()), 0);

    q.prepare ("SELECT item_type "
             "FROM itemsite,item "
             "WHERE ((itemsite_id=:itemsite_id)"
             "AND (itemsite_item_id=item_id)"
             "AND (itemsite_supply));");
    q.bindValue(":itemsite_id", _avail->id());
    q.exec();
    if (q.next())
    {
      if (q.value("item_type") == "P")
      {
        pMenu->insertSeparator();
        menuItem = pMenu->insertItem("Issue Purchase Order...", this, SLOT(sIssuePO()), 0);
        if (!_privileges->check("MaintainPurchaseOrders"))
          pMenu->setItemEnabled(menuItem, FALSE);
      }
      else if (q.value("item_type") == "M")
      {
        pMenu->insertSeparator();
        menuItem = pMenu->insertItem("Issue Work Order...", this, SLOT(sIssueWO()), 0);
        if (!_privileges->check("MaintainWorkOrders"))
          pMenu->setItemEnabled(menuItem, FALSE);
      }
    }

    if(_metrics->boolean("EnableSOReservations"))
    {
      pMenu->insertSeparator();

      pMenu->insertItem(tr("Show Reservations..."), this, SLOT(sShowReservations()));
      pMenu->insertSeparator();

      int menuid;
      menuid = pMenu->insertItem(tr("Unreserve Stock"), this, SLOT(sUnreserveStock()), 0);
      pMenu->setItemEnabled(menuid, _privileges->check("MaintainReservations"));
      menuid = pMenu->insertItem(tr("Reserve Stock..."), this, SLOT(sReserveStock()), 0);
      pMenu->setItemEnabled(menuid, _privileges->check("MaintainReservations"));
      menuid = pMenu->insertItem(tr("Reserve Line Balance"), this, SLOT(sReserveLineBalance()), 0);
      pMenu->setItemEnabled(menuid, _privileges->check("MaintainReservations"));
    }

    pMenu->insertSeparator();
    menuItem = pMenu->insertItem("Issue Count Tag...", this, SLOT(sIssueCountTag()), 0);
    if (!_privileges->check("IssueCountTags"))
      pMenu->setItemEnabled(menuItem, FALSE);
  }
}

void dspInventoryAvailabilityByCustomerType::sViewAllocations()
{
  q.prepare( "SELECT coitem_scheddate "
             "FROM coitem "
             "WHERE (coitem_id=:soitem_id);" );
  q.bindValue(":soitem_id", _avail->altId());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("itemsite_id", _avail->id());
    params.append("byDate", q.value("coitem_scheddate"));
    params.append("run");

    dspAllocations *newdlg = new dspAllocations();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
}

void dspInventoryAvailabilityByCustomerType::sViewOrders()
{
  q.prepare( "SELECT coitem_scheddate "
             "FROM coitem "
             "WHERE (coitem_id=:soitem_id);" );
  q.bindValue(":soitem_id", _avail->altId());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("itemsite_id", _avail->id());
    params.append("byDate", q.value("coitem_scheddate"));
    params.append("run");

    dspOrders *newdlg = new dspOrders();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
}

void dspInventoryAvailabilityByCustomerType::sRunningAvailability()
{
  ParameterList params;
  params.append("itemsite_id", _avail->id());
  params.append("run");

  dspRunningAvailability *newdlg = new dspRunningAvailability();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspInventoryAvailabilityByCustomerType::sViewSubstituteAvailability()
{
  q.prepare( "SELECT coitem_scheddate "
             "FROM coitem "
             "WHERE (coitem_id=:soitem_id);" );
  q.bindValue(":soitem_id", _avail->altId());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("itemsite_id", _avail->id());
    params.append("byDate", q.value("coitem_scheddate"));
    params.append("run");

    dspSubstituteAvailabilityByItem *newdlg = new dspSubstituteAvailabilityByItem();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
//  ToDo
}

void dspInventoryAvailabilityByCustomerType::sIssuePO()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _avail->id());

  purchaseOrder *newdlg = new purchaseOrder();
  if(newdlg->set(params) == NoError)
    omfgThis->handleNewWindow(newdlg);
}

void dspInventoryAvailabilityByCustomerType::sIssueWO()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _avail->id());

  workOrder *newdlg = new workOrder();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspInventoryAvailabilityByCustomerType::sIssueCountTag()
{
  ParameterList params;
  params.append("itemsite_id", _avail->id());

  createCountTagsByItem newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspInventoryAvailabilityByCustomerType::sFillList()
{
  _avail->clear();

  QString sql( "SELECT itemsite_id, coitem_id,"
               "       cohead_id, cohead_number, (cust_number||'-'||cust_name) AS custname,"
               "       item_number, item_description, uom_name, item_picklist,"
               "       qoh, formatQty(qoh) AS f_qoh,sobalance,"
               "       formatQty(sobalance) AS f_sobalance,"
               "       formatQty(allocated) AS f_allocated,"
               "       ordered, formatQty(ordered) AS f_ordered,"
               "       (qoh + ordered - sobalance) AS woavail,"
               "<? if exists(\"useReservationNetting\") ?>"
               "       formatQty(coitem_qtyreserved) AS f_soavail,"
               "<? else ?>"
               "       formatQty(qoh + ordered - sobalance) AS f_soavail,"
               "<? endif ?>"
               "       (qoh + ordered - allocated) AS totalavail,"
               "       formatQty(qoh + ordered - allocated) AS f_totalavail,"
               "       atshipping,formatQty(atshipping) AS f_atshipping,"
               "       formatDate(coitem_scheddate) AS f_scheddate,"
               "       (coitem_qtyreserved > 0 AND sobalance > coitem_qtyreserved) AS partialreservation,"
               "       ((sobalance <> 0) and ((sobalance - coitem_qtyreserved) = 0)) AS fullreservation,"
               "       onpacklist,"
               "       reorderlevel "
               "<? if exists(\"showWoSupply\") ?>, "        
               "       wo_id,"
               "       wo_status,"
               "       wo_number,"
               "       wo_ordered,"
               "       formatQty(wo_ordered) AS f_wo_ordered,"
               "       formatDate(wo_startdate) AS f_wo_startdate, "
               "       formatDate(wo_duedate) AS f_wo_duedate,"
               "       COALESCE(wo_latestart,false) AS wo_latestart,"
               "       COALESCE(wo_latedue,false) AS wo_latedue "
               "<? endif ?>"
               "FROM ( SELECT itemsite_id, coitem_id,"
               "              cohead_id, cohead_number, cust_number, cust_name,"
               "              item_number, (item_descrip1 || ' ' || item_descrip2) AS item_description,"
               "              uom_name, item_picklist,"
               "              noNeg(itemsite_qtyonhand) AS qoh,"
               "              noNeg(coitem_qtyord - coitem_qtyshipped + coitem_qtyreturned) AS sobalance,"
               "              qtyAllocated(itemsite_id, coitem_scheddate) AS allocated,"
               "              qtyOrdered(itemsite_id, coitem_scheddate) AS ordered,"
               "              qtyatshipping(coitem_id) AS atshipping,"
               "              coitem_qtyreserved,"
               "              coitem_scheddate,"
               "              (pack_id IS NOT NULL) AS onpacklist,"
               "              CASE WHEN(itemsite_useparams) THEN itemsite_reorderlevel ELSE 0.0 END AS reorderlevel "
               "<? if exists(\"showWoSupply\") ?>, " 
               "              COALESCE(wo_id,-1) AS wo_id,"
               "              formatwonumber(wo_id) AS wo_number,"
               "              noNeg((wo_qtyord-wo_qtyrcv)) AS wo_ordered,"
               "              wo_status, wo_startdate, wo_duedate,"
               "              ((wo_startdate <= CURRENT_DATE) AND (wo_status IN ('O','E','S','R'))) AS wo_latestart,"
               "              (wo_duedate<=CURRENT_DATE) AS wo_latedue " 
               "<? endif ?>" 
               "       FROM cohead "
               "            LEFT OUTER JOIN pack"
               "              ON (pack_head_type='SO' AND pack_head_id=cohead_id),"
               "            cust, itemsite, item, uom, coitem, site() "
               "<? if exists(\"showWoSupply\") ?> "
               "            LEFT OUTER JOIN wo"
               "             ON ((coitem_itemsite_id=wo_itemsite_id)"
               "             AND (wo_status IN ('E','R','I'))"
               "             AND (wo_qtyord-wo_qtyrcv > 0)"
               "             AND (noNeg(coitem_qtyord - coitem_qtyshipped + coitem_qtyreturned-qtyatshipping(coitem_id)) > "
               "              (SELECT itemsite_qtyonhand FROM itemsite WHERE (itemsite_id=coitem_itemsite_id))))"
               "<? endif ?>"
               "       WHERE ( (coitem_cohead_id=cohead_id)"
               "        AND (coitem_itemsite_id=itemsite_id)"
			   "        AND (itemsite_warehous_id=warehous_id)"
               "        AND (itemsite_item_id=item_id)"
               "        AND (item_inv_uom_id=uom_id)"
               "        AND (coitem_status NOT IN ('C', 'X'))"
               "        AND (cohead_cust_id=cust_id)"
               "<? if exists(\"custtype_id\") ?>"
               "        AND (cust_custtype_id=<? value(\"custtype_id\") ?>)"
               "<? elseif exists(\"custtype_pattern\") ?>"
               "        AND (cust_custtype_id IN (SELECT custtype_id "
               "                                    FROM custtype "
               "                                   WHERE(custtype_code ~ <? value(\"custtype_pattern\") ?>)))"
               "<? endif ?>"
               ")) AS data "
               " <? if exists(\"onlyShowShortages\") ?>"
               "WHERE ( ((qoh + ordered - allocated) < 0)"
               " OR ((qoh + ordered - sobalance) < 0) ) "
               "<? endif ?>"
               "ORDER BY cohead_id, cohead_number, item_number"
               "<? if exists(\"showWoSupply\") ?> ,"
               "wo_duedate"
               "<? endif ?>"
               ";");
    
  ParameterList params;             
  _custtype->appendValue(params);
  if (_onlyShowShortages->isChecked())
    params.append("onlyShowShortages");
  if (_showWoSupply->isChecked())
    params.append("showWoSupply");
  if (_useReservationNetting->isChecked())
    params.append("useReservationNetting");
  
  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  if (q.first())
  {
    XTreeWidgetItem *coitem = NULL, *cohead = NULL;
    XTreeWidgetItem *wo = NULL;
    int coitemid = -1;
    int coheadid = -1;
      
    do
    {
      if (coitemid != q.value("coitem_id").toInt())
      {
        if(coheadid != q.value("cohead_id").toInt())
        {
          coheadid = q.value("cohead_id").toInt();
          cohead = new XTreeWidgetItem(_avail, cohead, coheadid, -2,
                                       q.value("cohead_number"), q.value("custname"));
          if(q.value("onpacklist").toBool())
            cohead->setTextColor("green");
        }

        coitemid = q.value("coitem_id").toInt();
        coitem = new XTreeWidgetItem( cohead, coitem,
                                             q.value("itemsite_id").toInt(), q.value("coitem_id").toInt(),
                                             q.value("item_number"),
                                             q.value("item_description"), q.value("uom_name"),
                                             q.value("f_qoh"), q.value("f_sobalance"),
                                             q.value("f_allocated"), q.value("f_ordered"),
                                             q.value("f_soavail"), q.value("f_totalavail"),
                                             q.value("f_atshipping"), q.value("f_scheddate") );

        if (q.value("qoh").toDouble() < 0)
          coitem->setTextColor(3, "red");
        else if (q.value("qoh").toDouble() < q.value("reorderlevel").toDouble())
          coitem->setTextColor(3, "orange");

        if (q.value("woavail").toDouble() < 0.0)
          coitem->setTextColor(7, "red");
        else if (q.value("woavail").toDouble() <= q.value("reorderlevel").toDouble())
          coitem->setTextColor(7, "orange");

        if (q.value("totalavail").toDouble() < 0.0)
          coitem->setTextColor(8, "red");
        else if (q.value("totalavail").toDouble() <= q.value("reorderlevel").toDouble())
          coitem->setTextColor(8, "orange"); 

        if(_useReservationNetting->isChecked())
        {
          if(q.value("partialreservation").toBool())
          {
            coitem->setTextColor(0, "blue");
            coitem->setTextColor(1, "blue");
            coitem->setTextColor(7, "blue");
          }
          else if(q.value("fullreservation").toBool())
          {
            coitem->setTextColor(0, "green");
            coitem->setTextColor(1, "green");
            coitem->setTextColor(7, "green");
          }
        }
      }
      if ((coitem)
      && (_showWoSupply->isChecked())
      && (q.value("wo_id").toInt() != -1) )
      {
        wo = new XTreeWidgetItem( coitem, wo,
                                             q.value("itemsite_id").toInt(),-1,
                                             q.value("wo_number"),"",
                                              q.value("wo_status"),
                                             "", "",
                                             "", q.value("f_wo_ordered"),
                                            q.value("f_wo_startdate"), q.value("f_wo_duedate"),
                                             "" );
                                             
        if (q.value("wo_latestart").toBool())
          wo->setTextColor(7, "red");
        if (q.value("wo_latedue").toBool())
          wo->setTextColor(8, "red");
      }
    }
    while (q.next());
  }
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  _avail->expandAll();
}

void dspInventoryAvailabilityByCustomerType::sAutoUpdateToggled(bool pAutoUpdate)
{
  if (pAutoUpdate)
    connect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
  else
    disconnect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
}

void dspInventoryAvailabilityByCustomerType::sHandleReservationNetting(bool yn)
{
  if(yn)
    _avail->headerItem()->setText(7, tr("This Reserve"));
  else
    _avail->headerItem()->setText(7, tr("This Avail."));
  sFillList();
}

void dspInventoryAvailabilityByCustomerType::sReserveStock()
{
  ParameterList params;
  params.append("soitem_id", _avail->altId());

  reserveSalesOrderItem newdlg(this, "", true);
  newdlg.set(params);
  if(newdlg.exec() == XDialog::Accepted)
    sFillList();
}

void dspInventoryAvailabilityByCustomerType::sReserveLineBalance()
{
  q.prepare("SELECT reserveSoLineBalance(:soitem_id) AS result;");
  q.bindValue(":soitem_id", _avail->altId());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("reserveSoLineBalance", result),
                  __FILE__, __LINE__);
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, tr("Error\n") +
                      q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void dspInventoryAvailabilityByCustomerType::sUnreserveStock()
{
  q.prepare("UPDATE coitem SET coitem_qtyreserved=0 WHERE coitem_id=:soitem_id;");
  q.bindValue(":soitem_id", _avail->altId());
  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, tr("Error\n") +
                      q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void dspInventoryAvailabilityByCustomerType::sShowReservations()
{
  ParameterList params;
  params.append("soitem_id", _avail->altId());
  params.append("run");

  dspReservations * newdlg = new dspReservations();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspInventoryAvailabilityByCustomerType::sAddToPackingListBatch()
{
  q.prepare("SELECT addToPackingListBatch(:sohead_id) AS result;");
  q.bindValue(":sohead_id", _avail->id());
  q.exec();
  sFillList();
}

