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

//  menuSchedule.cpp
//  Create 08/22/2000 JSL
//  Copyright (c) 2000-2007, OpenMFG, LLC

#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QPixmap>
#include <QMenu>
#include <QToolBar>

#include <parameter.h>

#include "OpenMFGGUIClient.h"
#include "inputManager.h"

#include "plannedSchedules.h"
#include "plannedSchedule.h"

#include "plannedOrder.h"
#include "createPlannedOrdersByItem.h"
#include "createPlannedOrdersByPlannerCode.h"
#include "runMPSByPlannerCode.h"
#include "deletePlannedOrder.h"
#include "deletePlannedOrdersByPlannerCode.h"
#include "firmPlannedOrdersByPlannerCode.h"
#include "releasePlannedOrdersByPlannerCode.h"

#include "createBufferStatusByItem.h"
#include "createBufferStatusByPlannerCode.h"
#include "dspInventoryBufferStatusByParameterList.h"
#include "dspCapacityBufferStatusByWorkCenter.h"
#include "dspWoBufferStatusByParameterList.h"
#include "dspWoOperationBufrStsByWorkCenter.h"
#include "dspPoItemsByBufferStatus.h"

#include "dspTimePhasedCapacityByWorkCenter.h"
#include "dspTimePhasedLoadByWorkCenter.h"
#include "dspTimePhasedAvailableCapacityByWorkCenter.h"
#include "dspTimePhasedDemandByPlannerCode.h"
#include "dspTimePhasedProductionByItem.h"
#include "dspTimePhasedProductionByPlannerCode.h"

#include "dspPlannedOrdersByItem.h"
#include "dspPlannedOrdersByPlannerCode.h"
#include "dspMRPDetail.h"
#include "dspMPSDetail.h"
#include "dspRoughCutByWorkCenter.h"
#include "dspTimePhasedRoughCutByWorkCenter.h"
#include "dspPlannedRevenueExpensesByPlannerCode.h"
#include "dspTimePhasedPlannedREByPlannerCode.h"
#include "dspRunningAvailability.h"
#include "dspTimePhasedAvailability.h"
#include "dspExpediteExceptionsByPlannerCode.h"
#include "dspReorderExceptionsByPlannerCode.h"

#include "plannerCodes.h"
#include "whseWeek.h"
#include "whseCalendars.h"
#include "workCenters.h"

#include "menuSchedule.h"

menuSchedule::menuSchedule(OpenMFGGUIClient *Pparent) :
 QObject(Pparent, "msModule")
{
  parent = Pparent;
  
  toolBar = new QToolBar(tr("Schedule Tools"));
  toolBar->setObjectName("Schedule Tools");
  toolBar->setIconSize(QSize(32, 32));
  if (_preferences->boolean("ShowMSToolbar"))
    parent->addToolBar(toolBar);

  mainMenu = new QMenu();
  planningMenu = new QMenu();
  plannedOrdersMenu = new QMenu();
  plannedOrdersMrpMenu = new QMenu();
  capacityPlanMenu = new QMenu();
  capacityPlanTpPrdMenu = new QMenu();
  bufferMenu = new QMenu();
  bufferRunMenu = new QMenu();
  bufferInvMenu = new QMenu();
  bufferWoMenu = new QMenu();
  reportsMenu = new QMenu();
  reportsPlannedMenu = new QMenu();
  masterInfoMenu = new QMenu();

  actionProperties acts[] = {
  
    // Schedule | Planning
    { "menu",	tr("&Production Plan"), (char*)planningMenu,	mainMenu,	true,	NULL, NULL, true	, NULL },
    { "ms.newProductionPlan", tr("&New..."), SLOT(sNewProductionPlan()), planningMenu, _privleges->check("MaintainPlannedSchedules"), NULL, NULL, true , NULL },
    { "ms.listProductionPlans", tr("&List..."), SLOT(sListProductionPlans()), planningMenu, _privleges->check("MaintainPlannedSchedules") || _privleges->check("ViewPlannedSchedules"), NULL, NULL, true , NULL },

    // Schedule | Schedule  
    { "menu",	tr("&Scheduling"), (char*)plannedOrdersMenu,	mainMenu,	true,	NULL, NULL, true	, NULL },
    { "ms.createPlannedOrder", tr("&New Planned Order..."), SLOT(sCreatePlannedOrder()), plannedOrdersMenu, _privleges->check("CreatePlannedOrders"), NULL, NULL, true , NULL },
    { "separator", NULL, NULL, plannedOrdersMenu, true, NULL, NULL, true , NULL },
    { "ms.runMPSByPlannerCode", tr("Run M&PS..."), SLOT(sRunMPSByPlannerCode()),plannedOrdersMenu, _privleges->check("CreatePlannedOrders"), NULL, NULL, true , NULL },
 
    // Schedule | Schedule | MRP
    { "menu",	tr("Run &MRP"), (char*)plannedOrdersMrpMenu,	plannedOrdersMenu,	true,	NULL, NULL, true	, NULL },
    { "ms.runMRPByPlannerCode", tr("by &Planner Code..."), SLOT(sCreatePlannedReplenOrdersByPlannerCode()), plannedOrdersMrpMenu, _privleges->check("CreatePlannedOrders"), new QPixmap(":/images/runMrpByPlannerCode.png"), toolBar, true , "Run MRP by Planner Code" },
    { "ms.runMRPByItem", tr("by &Item..."), SLOT(sCreatePlannedReplenOrdersByItem()), plannedOrdersMrpMenu, _privleges->check("CreatePlannedOrders"), NULL, NULL, true , NULL },
    
    { "separator", NULL, NULL, plannedOrdersMenu, true, NULL, NULL, true , NULL },
    { "ms.firmPlannedOrdersByPlannerCode", tr("&Firm Planned Orders..."), SLOT(sFirmPlannedOrdersByPlannerCode()), plannedOrdersMenu, _privleges->check("FirmPlannedOrders"), NULL, NULL, true , NULL },
    { "ms.releasePlannedOrdersByPlannerCode", tr("&Release Planned Orders..."), SLOT(sReleasePlannedOrdersByPlannerCode()),  plannedOrdersMenu, _privleges->check("ReleasePlannedOrders"), NULL, NULL, true , NULL },
    { "separator", NULL, NULL, plannedOrdersMenu, true, NULL, NULL, true , NULL },
    { "ms.deletePlannedOrder", tr("&Delete Planned Order..."), SLOT(sDeletePlannedOrder()),plannedOrdersMenu, _privleges->check("DeletePlannedOrders"), NULL, NULL, true , NULL },
    { "ms.deletePlannedOrdersByPlannerCode", tr("Delete Planned Order&s..."), SLOT(sDeletePlannedOrdersByPlannerCode()), plannedOrdersMenu, _privleges->check("DeletePlannedOrders"), NULL, NULL, true , NULL },
    { "separator", NULL, NULL, mainMenu, true, NULL, NULL, true , NULL },

    // Schedule | Buffer Status
    { "menu",	tr("&Buffer Status"), (char*)bufferMenu,	mainMenu,	true,	NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
  
    // Schedule | Buffer Status | Run
    { "menu",	tr("&Update"), (char*)bufferRunMenu,	bufferMenu,	true,	NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.runBufferStatusByPlannerCode", tr("by &Planner Code..."), SLOT(sCreateBufferStatusByPlannerCode()), bufferRunMenu, _privleges->check("CreateBufferStatus"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.runBufferStatusByItem", tr("by &Item..."), SLOT(sCreateBufferStatusByItem()), bufferRunMenu, _privleges->check("CreateBufferStatus"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
 
    { "separator", NULL, NULL, bufferMenu, true, NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
  
    // Schedule | Buffer Management | Inventory Buffer Status
    { "menu",	tr("&Inventory"), (char*)bufferInvMenu,	bufferMenu,	true,	NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspInventoryBufferStatusByPlannerCode", tr("by &Planner Code..."), SLOT(sDspInventoryBufferStatusByPlannerCode()), bufferInvMenu, _privleges->check("ViewInventoryBufferStatus"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspInventoryBufferStatusByClassCode", tr("by &Class Code..."), SLOT(sDspInventoryBufferStatusByClassCode()), bufferInvMenu, _privleges->check("ViewInventoryBufferStatus"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspInventoryBufferStatusByItemGroup", tr("by &Item Group..."), SLOT(sDspInventoryBufferStatusByItemGroup()), bufferInvMenu, _privleges->check("ViewInventoryBufferStatus"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspPoLineItemsByBufferStatus", tr("&Purchase Order..."), SLOT(sDspPoItemsByBufferStatus()), bufferMenu, _privleges->check("ViewPurchaseOrders"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },

    // Schedule | Buffer Management | Work Order Buffer Status
    { "menu",	tr("&Work Order"), (char*)bufferWoMenu,	bufferMenu,	true,	NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspWoBufferStatusByPlannerCode", tr("by &Planner Code..."), SLOT(sDspWoBufferStatusByPlannerCode()), bufferWoMenu, _privleges->check("MaintainWorkOrders") || _privleges->check("ViewWorkOrders"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspWoBufferStatusByClassCode", tr("by &Class Code..."), SLOT(sDspWoBufferStatusByClassCode()), bufferWoMenu, _privleges->check("MaintainWorkOrders") || _privleges->check("ViewWorkOrders"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspWoBufferStatusByItemGroup", tr("by &Item Group..."), SLOT(sDspWoBufferStatusByItemGroup()), bufferWoMenu, _privleges->check("MaintainWorkOrders") || _privleges->check("ViewWorkOrders"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
  
    { "separator", NULL, NULL, bufferMenu, true, NULL, NULL,  _metrics->boolean("Routings") &&  _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspCapacityBufferStatusByWorkCenter", tr("&Capacity..."), SLOT(sDspCapacityBufferStatusByWorkCenter()), bufferMenu, _privleges->check("ViewWorkCenterBufferStatus"), NULL, NULL, _metrics->boolean("Routings") && _metrics->boolean("BufferMgt") , NULL },
    { "ms.dspWoOperationBufrStsByWorkCenter", tr("W/O &Operation..."), SLOT(sDspWoOperationBufrStsByWorkCenter()), bufferMenu, _privleges->check("MaintainWoOperations") || _privleges->check("ViewWoOperations"), NULL , NULL, _metrics->boolean("Routings") && _metrics->boolean("BufferMgt")  , NULL },  
  
    // Schedule | Capacity Plannning
    { "menu",	tr("&Capacity Planning"), (char*)capacityPlanMenu,	mainMenu,	true,	NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "cp.dspTimePhasedCapacityByWorkCenter", tr("Time-Phased &Capacity..."), SLOT(sDspTimePhasedCapacityByWorkCenter()), capacityPlanMenu, _privleges->check("ViewWorkCenterCapacity"), NULL, NULL, _metrics->boolean("Routings") , NULL },
    { "cp.dspTimePhasedLoadByWorkCenter", tr("Time-Phased &Load..."), SLOT(sDspTimePhasedLoadByWorkCenter()), capacityPlanMenu, _privleges->check("ViewWorkCenterLoad"), NULL, NULL, _metrics->boolean("Routings") , NULL },
    { "cp.dspTimePhasedAvailableCapacityByWorkCenter", tr("Time-Phased &Available Capacity..."), SLOT(sDspTimePhasedAvailableCapacityByWorkCenter()), capacityPlanMenu, _privleges->check("ViewWorkCenterCapacity"), NULL, NULL, _metrics->boolean("Routings") , NULL },
    { "separator", NULL, NULL, capacityPlanMenu, true, NULL, NULL,  _metrics->boolean("Routings") , NULL },

    // Schedule | Capacity Plannning | Time Phased Production
    { "menu",	tr("Time-Phased &Production"), (char*)capacityPlanTpPrdMenu,	capacityPlanMenu,	true,	NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "cp.dspTimePhasedProductionByPlannerCode", tr("by &Planner Code..."), SLOT(sDspTimePhasedProductionByPlannerCode()), capacityPlanTpPrdMenu, _privleges->check("ViewProduction"), NULL, NULL, true , NULL },
    { "cp.dspTimePhasedProductionByItem", tr("by &Item..."), SLOT(sDspTimePhasedProductionByItem()), capacityPlanTpPrdMenu, _privleges->check("ViewProduction"), NULL, NULL, true , NULL },

    { "cp.dspTimePhasedDemandByPlannerCode", tr("Time-Phased &Demand..."), SLOT(sDspTimePhasedDemandByPlannerCode()), capacityPlanMenu, _privleges->check("ViewProductionDemand"), NULL, NULL, true , NULL },
    { "separator", NULL, NULL, capacityPlanMenu, true, NULL, NULL, _metrics->boolean("Routings") && _metrics->boolean("BufferMgt") , NULL },
    { "cp.dspCapacityBufferStatusByWorkCenter", tr("Capacity &Buffer Status..."), SLOT(sDspCapacityBufferStatusByWorkCenter()), capacityPlanMenu, _privleges->check("ViewWorkCenterBufferStatus"), NULL, NULL, _metrics->boolean("Routings") && _metrics->boolean("BufferMgt") , NULL },
  
    // Schedule | Report
    { "menu",	tr("&Reports"), (char*)reportsMenu, mainMenu, true, NULL, NULL, true , NULL },
  
    // Schedule | Report | Planned Orders
    { "menu",	tr("Planned &Orders"), (char*)reportsPlannedMenu, reportsMenu, true, NULL, NULL, true , NULL },
    { "ms.dspPlannedOrdersByPlannerCode", tr("by &Planner Code..."), SLOT(sDspPlannedOrdersByPlannerCode()), reportsPlannedMenu, _privleges->check("ViewPlannedOrders"), new QPixmap(":/images/dspPlannedOrdersByPlannerCode.png"), toolBar, true , "Planned Orders by Planner Code" },
    { "ms.dspPlannedOrdersByItem", tr("by &Item..."), SLOT(sDspPlannedOrdersByItem()), reportsPlannedMenu, _privleges->check("ViewPlannedOrders"), NULL, NULL, true , NULL },
    
    { "separator", NULL, NULL, reportsMenu, true, NULL, NULL, true , NULL },
    { "ms.dspRunningAvailability", tr("&Running Availability..."), SLOT(sDspRunningAvailability()), reportsMenu, _privleges->check("ViewInventoryAvailability"), NULL, NULL, true , NULL },
    { "ms.dspTimePhasedAvailabiltiy", tr("&Time-Phased Availability..."), SLOT(sDspTimePhasedAvailability()), reportsMenu, _privleges->check("ViewInventoryAvailability"), NULL, NULL, true , NULL },
    { "ms.dspMPSDetail", tr("MP&S Detail..."), SLOT(sDspMPSDetail()), reportsMenu, _privleges->check("ViewMPS"), NULL, NULL, true , NULL },
    { "ms.dspMRPDetail", tr("&MRP Detail..."), SLOT(sDspMRPDetail()), reportsMenu, _privleges->check("ViewInventoryAvailability"), NULL, NULL, true , NULL },
    { "separator", NULL, NULL, reportsMenu, true, NULL, NULL, true , NULL },
    { "ms.dspExpediteExceptionsByPlannerCode", tr("E&xpedite Exceptions..."), SLOT(sDspExpediteExceptionsByPlannerCode()), reportsMenu, _privleges->check("ViewInventoryAvailability"), NULL, NULL, true , NULL },
    { "ms.dspReorderExceptionsByPlannerCode", tr("Reorder &Exceptions..."), SLOT(sDspReorderExceptionsByPlannerCode()),reportsMenu, _privleges->check("ViewInventoryAvailability"), NULL, NULL, true , NULL },
    { "separator", NULL, NULL, reportsMenu, true, NULL, NULL, _metrics->boolean("Routings") , NULL },
    { "ms.dspRoughCutByWorkCenter", tr("Rough &Cut Capacity Plan..."), SLOT(sDspRoughCutByWorkCenter()), reportsMenu, _privleges->check("ViewRoughCut"), NULL, NULL,  _metrics->boolean("Routings") , NULL },
    { "ms.dspTimePhasedRoughCutByWorkCenter", tr("Time-P&hased Rough Cut Capacity Plan..."), SLOT(sDspTimePhasedRoughCutByWorkCenter()), reportsMenu, _privleges->check("ViewRoughCut"), NULL, NULL,  _metrics->boolean("Routings") , NULL },
    { "separator", NULL, NULL, reportsMenu, true, NULL, NULL, _metrics->boolean("Routings") , NULL },
    { "ms.dspPlannedRevenue/ExpensesByPlannerCode", tr("P&lanned Revenue/Expenses..."), SLOT(sDspPlannedRevenueExpensesByPlannerCode()), reportsMenu, _privleges->check("ViewPlannedOrders") && _privleges->check("ViewCosts") && _privleges->check("ViewListPrices"), NULL, NULL, _metrics->boolean("Routings") , NULL },
    { "ms.dspTimePhasedPlannedRevenue/ExpensesByPlannerCode", tr("Time-Ph&ased Planned Revenue/Expenses..."), SLOT(sDspTimePhasedPlannedREByPlannerCode()),  reportsMenu, _privleges->check("ViewPlannedOrders") && _privleges->check("ViewCosts") && _privleges->check("ViewListPrices"), NULL, NULL, _metrics->boolean("Routings") , NULL },
    { "separator", NULL, NULL, mainMenu, true, NULL, NULL, true , NULL },
    
    //  Master Information
    { "menu",	tr("&Master Information"), (char*)masterInfoMenu, mainMenu, true, NULL, NULL, true , NULL },
    { "ms.plannerCodes", tr("&Planner Codes..."), SLOT(sPlannerCodes()), masterInfoMenu, _privleges->check("MaintainPlannerCodes") && _privleges->check("ViewPlannerCodes"), NULL, NULL, true , NULL },
    { "ms.warehouseWeek", tr("Warehouse &Week..."), SLOT(sWarehouseWeek()),masterInfoMenu, _privleges->check("MaintainWarehouseWorkWeek"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "ms.warehouseCalendarExceptions", tr("Warehouse Calendar &Exceptions..."), SLOT(sWarehouseCalendarExceptions()), masterInfoMenu, _privleges->check("MaintainWarehouseCalendarExceptions") || _privleges->check("ViewWarehouseCalendarExceptions"), NULL, NULL, _metrics->boolean("BufferMgt") , NULL },
    { "cp.workCenters", tr("Work &Centers..."), SLOT(sWorkCenters()), masterInfoMenu, _privleges->check("MaintainWorkCenters") && _privleges->check("ViewWorkCenters"), NULL, NULL, _metrics->boolean("Routings") , NULL },
  };
  addActionsToMenu(acts, sizeof(acts) / sizeof(acts[0]));

  parent->populateCustomMenu(mainMenu, "Schedule");
  parent->menuBar()->insertItem(tr("Sche&dule"), mainMenu);
}

void menuSchedule::addActionsToMenu(actionProperties acts[], unsigned int numElems)
{
  for (unsigned int i = 0; i < numElems; i++)
  {
    if (! acts[i].visible)
    {
      continue;
    }
    else if (acts[i].actionName == "menu")
    {
      acts[i].menu->insertItem(acts[i].actionTitle, (QMenu*)(acts[i].slot));
    }
    else if (acts[i].actionName == "separator")
    {
      acts[i].menu->addSeparator();
    }
    else if ((acts[i].toolBar != NULL) && (acts[i].toolBar != NULL))
    {
      parent->actions.append( new Action( parent,
					  acts[i].actionName,
					  acts[i].actionTitle,
					  this,
					  acts[i].slot,
					  acts[i].menu,
					  acts[i].priv,
					  *(acts[i].pixmap),
					  acts[i].toolBar,
                      acts[i].toolTip) );
    }
    else if (acts[i].toolBar != NULL)
    {
      parent->actions.append( new Action( parent,
					  acts[i].actionName,
					  acts[i].actionTitle,
					  this,
					  acts[i].slot,
					  acts[i].menu,
					  acts[i].priv,
					  *(acts[i].pixmap),
					  acts[i].toolBar,
                      acts[i].actionTitle) );
    }
    else
    {
      parent->actions.append( new Action( parent,
					  acts[i].actionName,
					  acts[i].actionTitle,
					  this,
					  acts[i].slot,
					  acts[i].menu,
					  acts[i].priv ) );
    }
  }
}

void menuSchedule::sCreatePlannedOrder()
{
  ParameterList params;
  params.append("mode", "new");

  plannedOrder newdlg(parent, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void menuSchedule::sCreatePlannedReplenOrdersByItem()
{
  createPlannedOrdersByItem(parent, "", TRUE).exec();
}

void menuSchedule::sCreatePlannedReplenOrdersByPlannerCode()
{
  createPlannedOrdersByPlannerCode(parent, "", TRUE).exec();
}

void menuSchedule::sRunMPSByPlannerCode()
{
  runMPSByPlannerCode(parent, "", TRUE).exec();
}

void menuSchedule::sDeletePlannedOrder()
{
  deletePlannedOrder(parent, "", TRUE).exec();
}

void menuSchedule::sDeletePlannedOrdersByPlannerCode()
{
  deletePlannedOrdersByPlannerCode(parent, "", TRUE).exec();
}

void menuSchedule::sFirmPlannedOrdersByPlannerCode()
{
  firmPlannedOrdersByPlannerCode(parent, "", TRUE).exec();
}

void menuSchedule::sReleasePlannedOrdersByPlannerCode()
{
  releasePlannedOrdersByPlannerCode(parent, "", TRUE).exec();
}

void menuSchedule::sCreateBufferStatusByItem()
{
  createBufferStatusByItem(parent, "", TRUE).exec();
}

void menuSchedule::sCreateBufferStatusByPlannerCode()
{
  createBufferStatusByPlannerCode(parent, "", TRUE).exec();
}

void menuSchedule::sDspInventoryBufferStatusByItemGroup()
{
  ParameterList params;
  params.append("itemgrp");

  dspInventoryBufferStatusByParameterList *newdlg = new dspInventoryBufferStatusByParameterList();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuSchedule::sDspInventoryBufferStatusByClassCode()
{
  ParameterList params;
  params.append("classcode");

  dspInventoryBufferStatusByParameterList *newdlg = new dspInventoryBufferStatusByParameterList();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuSchedule::sDspInventoryBufferStatusByPlannerCode()
{
  ParameterList params;
  params.append("plancode");

  dspInventoryBufferStatusByParameterList *newdlg = new dspInventoryBufferStatusByParameterList();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuSchedule::sDspCapacityBufferStatusByWorkCenter()
{
  omfgThis->handleNewWindow(new dspCapacityBufferStatusByWorkCenter());
}

void menuSchedule::sDspWoBufferStatusByItemGroup()
{
  ParameterList params;
  params.append("itemgrp");

  dspWoBufferStatusByParameterList *newdlg = new dspWoBufferStatusByParameterList();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuSchedule::sDspWoBufferStatusByClassCode()
{
  ParameterList params;
  params.append("classcode");

  dspWoBufferStatusByParameterList *newdlg = new dspWoBufferStatusByParameterList();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuSchedule::sDspWoBufferStatusByPlannerCode()
{
  ParameterList params;
  params.append("plancode");

  dspWoBufferStatusByParameterList *newdlg = new dspWoBufferStatusByParameterList();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuSchedule::sDspWoOperationBufrStsByWorkCenter()
{
  omfgThis->handleNewWindow(new dspWoOperationBufrStsByWorkCenter());
}

void menuSchedule::sDspPoItemsByBufferStatus()
{
  omfgThis->handleNewWindow(new dspPoItemsByBufferStatus());
}

void menuSchedule::sDspPlannedOrdersByItem()
{
  omfgThis->handleNewWindow(new dspPlannedOrdersByItem());
}

void menuSchedule::sDspPlannedOrdersByPlannerCode()
{
  omfgThis->handleNewWindow(new dspPlannedOrdersByPlannerCode());
}


void menuSchedule::sDspTimePhasedCapacityByWorkCenter()
{
  omfgThis->handleNewWindow(new dspTimePhasedCapacityByWorkCenter());
}

void menuSchedule::sDspTimePhasedAvailableCapacityByWorkCenter()
{
  omfgThis->handleNewWindow(new dspTimePhasedAvailableCapacityByWorkCenter());
}

void menuSchedule::sDspTimePhasedLoadByWorkCenter()
{
  omfgThis->handleNewWindow(new dspTimePhasedLoadByWorkCenter());
}

void menuSchedule::sDspTimePhasedProductionByItem()
{
  omfgThis->handleNewWindow(new dspTimePhasedProductionByItem());
}

void menuSchedule::sDspTimePhasedDemandByPlannerCode()
{
  omfgThis->handleNewWindow(new dspTimePhasedDemandByPlannerCode());
}

void menuSchedule::sDspTimePhasedProductionByPlannerCode()
{
  omfgThis->handleNewWindow(new dspTimePhasedProductionByPlannerCode());
}


void menuSchedule::sDspMPSDetail()
{
  omfgThis->handleNewWindow(new dspMPSDetail());
}

void menuSchedule::sDspRoughCutByWorkCenter()
{
  omfgThis->handleNewWindow(new dspRoughCutByWorkCenter());
}

void menuSchedule::sDspTimePhasedRoughCutByWorkCenter()
{
  omfgThis->handleNewWindow(new dspTimePhasedRoughCutByWorkCenter());
}

void menuSchedule::sDspPlannedRevenueExpensesByPlannerCode()
{
  omfgThis->handleNewWindow(new dspPlannedRevenueExpensesByPlannerCode());
}

void menuSchedule::sDspTimePhasedPlannedREByPlannerCode()
{
  omfgThis->handleNewWindow(new dspTimePhasedPlannedREByPlannerCode());
}

void menuSchedule::sDspTimePhasedAvailability()
{
  omfgThis->handleNewWindow(new dspTimePhasedAvailability());
}

void menuSchedule::sDspRunningAvailability()
{
  omfgThis->handleNewWindow(new dspRunningAvailability());
}

void menuSchedule::sDspMRPDetail()
{
  omfgThis->handleNewWindow(new dspMRPDetail());
}

void menuSchedule::sDspExpediteExceptionsByPlannerCode()
{
  omfgThis->handleNewWindow(new dspExpediteExceptionsByPlannerCode());
}

void menuSchedule::sDspReorderExceptionsByPlannerCode()
{
  omfgThis->handleNewWindow(new dspReorderExceptionsByPlannerCode());
}

void menuSchedule::sPlannerCodes()
{
  omfgThis->handleNewWindow(new plannerCodes());
}

void menuSchedule::sListProductionPlans()
{
  omfgThis->handleNewWindow(new plannedSchedules());
}

void menuSchedule::sNewProductionPlan()
{
  ParameterList params;
  params.append("mode", "new");

  plannedSchedule newdlg(parent, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void menuSchedule::sWarehouseWeek()
{
  omfgThis->handleNewWindow(new whseWeek());
}

void menuSchedule::sWarehouseCalendarExceptions()
{
  omfgThis->handleNewWindow(new whseCalendars());
}

void menuSchedule::sWorkCenters()
{
  omfgThis->handleNewWindow(new workCenters());
}

