/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QPixmap>
#include <QMenu>
#include <QToolBar>

#include <parameter.h>

#include "guiclient.h"
#include "inputManager.h"

#include "item.h"
#include "items.h"
#include "searchForItem.h"
#include "copyItem.h"
#include "itemGroups.h"
#include "itemImages.h"
#include "lotSerial.h"
#include "bom.h"
#include "bomList.h"
#include "copyBOM.h"

#include "boo.h"
#include "booList.h"
#include "copyBOO.h"
#include "massReplaceComponent.h"
#include "massExpireComponent.h"

#include "bbom.h"
#include "bboms.h"

#include "maintainItemCosts.h"
#include "updateActualCostsByItem.h"
#include "updateActualCostsByClassCode.h"
#include "postCostsByItem.h"
#include "postCostsByClassCode.h"

#include "dspCostedSingleLevelBOM.h"
#include "dspCostedIndentedBOM.h"
#include "dspCostedSummarizedBOM.h"
#include "dspItemCostsByClassCode.h"
#include "dspItemCostSummary.h"
#include "dspItemCostHistory.h"

#include "costingElements.h"

#include "dspItemsByClassCode.h"
#include "dspItemsByCharacteristic.h"
#include "dspItemsByProductCategory.h"
#include "dspSingleLevelBOM.h"
#include "dspIndentedBOM.h"
#include "dspSummarizedBOM.h"
#include "dspSequencedBOM.h"
#include "dspSingleLevelWhereUsed.h"
#include "dspIndentedWhereUsed.h"
#include "dspPendingBOMChanges.h"
#include "dspOperationsByWorkCenter.h"
#include "dspStandardOperationsByWorkCenter.h"
#include "dspCapacityUOMsByClassCode.h"
#include "dspCapacityUOMsByProductCategory.h"

#include "itemAvailabilityWorkbench.h"

#include "uoms.h"
#include "classCodes.h"
#include "productCategories.h"
#include "freightClasses.h"
#include "characteristics.h"
#include "laborRates.h"
#include "workCenters.h"
#include "standardOperations.h"

#include "dspUndefinedManufacturedItems.h"
#include "dspUnusedPurchasedItems.h"
#include "dspInvalidBillsOfMaterials.h"
#include "reassignClassCodeByClassCode.h"
#include "reassignProductCategoryByProductCategory.h"

#include "menuProducts.h"

menuProducts::menuProducts(GUIClient *Pparent) :
 QObject(Pparent, "productMenu")
{
  parent = Pparent;

  toolBar = new QToolBar(tr("Products Tools"));
  toolBar->setObjectName("Products Tools");
  toolBar->setIconSize(QSize(32, 32));
  if (_preferences->boolean("ShowPDToolbar"))
    parent->addToolBar(toolBar);
    
  mainMenu	= new QMenu(parent);
  itemsMenu	= new QMenu(parent);
  bomMenu	= new QMenu(parent);
  booMenu = new QMenu(parent);
  breederBOMMenu = new QMenu(parent);
  costingMenu = new QMenu(parent);
  costingUpdActMenu = new QMenu(parent);
  costingPostActMenu = new QMenu(parent);
  costingUpdStdMenu = new QMenu(parent);
  costingReportsMenu = new QMenu(parent);
  costingReportsCostedMenu = new QMenu(parent);
  costingReportsItemCostsMenu = new QMenu(parent);
  reportsMenu = new QMenu(parent);
  reportsItemsMenu = new QMenu(parent);
  reportsBomsMenu = new QMenu(parent);
  reportsWhereUsdMenu = new QMenu(parent);
  reportsCapUomMenu = new QMenu(parent);
  masterInfoMenu = new QMenu(parent);
  utilitiesMenu = new QMenu(parent);

  mainMenu->setObjectName("menu.prod");
  itemsMenu->setObjectName("menu.prod.items");
  bomMenu->setObjectName("menu.prod.bom");
  booMenu->setObjectName("menu.prod.boo");
  breederBOMMenu->setObjectName("menu.prod.breeder");
  costingMenu->setObjectName("menu.prod.costing");
  costingUpdActMenu->setObjectName("menu.prod.costingupdact");
  costingPostActMenu->setObjectName("menu.prod.costingpostact");
  costingUpdStdMenu->setObjectName("menu.prod.costingupdstd");
  costingReportsMenu->setObjectName("menu.prod.costingreports");
  costingReportsCostedMenu->setObjectName("menu.prod.costingreportscosted");
  costingReportsItemCostsMenu->setObjectName("menu.prod.costingreportsitemcosts");
  reportsMenu->setObjectName("menu.prod.reports");
  reportsItemsMenu->setObjectName("menu.prod.reportsitems");
  reportsBomsMenu->setObjectName("menu.prod.reportsboms");
  reportsWhereUsdMenu->setObjectName("menu.prod.reportswhereusd");
  reportsCapUomMenu->setObjectName("menu.prod.reportscapuom");
  masterInfoMenu->setObjectName("menu.prod.masterinfo");
  utilitiesMenu->setObjectName("menu.prod.utilities");

  actionProperties acts[] = {
  
  // Product | Reports
  { "menu",	tr("&Reports"), (char*)reportsMenu,	mainMenu, true, NULL, NULL, true , NULL },
  
  // Product | Reports | Items
  { "menu",	tr("&Items"), (char*)reportsItemsMenu,	reportsMenu, true, NULL, NULL, true , NULL },
  { "pd.dspItemsByProductCategory", tr("by &Product Category..."), SLOT(sDspItemsByProductCategory()), reportsItemsMenu, _privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters"), NULL, NULL, true , NULL },
  { "pd.dspItemsByClassCode", tr("by &Class Code..."), SLOT(sDspItemsByClassCode()), reportsItemsMenu, _privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters"), NULL, NULL, true , NULL },
  { "pd.dspItemsByCharacteristic", tr("by C&haracteristic..."), SLOT(sDspItemsByCharacteristic()),  reportsItemsMenu, _privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters"), NULL, NULL, true , NULL },
  { "separator", NULL, NULL, reportsMenu,	true, NULL, NULL, true , NULL },
  
  // Product | Reports | BOMs
  { "menu",	tr("&Bills of Materials"), (char*)reportsBomsMenu,	reportsMenu, true, NULL, NULL, true , NULL },
  { "pd.dspSingleLevelBOM", tr("&Single Level..."), SLOT(sDspSingleLevelBOM()), reportsBomsMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },
  { "pd.dspIndentedBOM", tr("&Indented..."), SLOT(sDspIndentedBOM()), reportsBomsMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },
  { "pd.dspSummarizedBOM", tr("Summari&zed..."), SLOT(sDspSummarizedBOM()), reportsBomsMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },
  { "pd.dspSequencedBOM", tr("Se&quenced..."), SLOT(sDspSequencedBOM()), reportsBomsMenu, _privileges->check("ViewBOMs"), NULL, NULL,  _metrics->boolean("Routings") , NULL },
  
  // Product | Reports | Where Used
  { "menu",	tr("&Where Used"), (char*)reportsWhereUsdMenu,	reportsMenu, true, NULL, NULL, true , NULL },
  { "pd.dspSingleLevelWhereUsed", tr("&Single Level..."), SLOT(sDspSingleLevelWhereUsed()), reportsWhereUsdMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },
  { "pd.dspIndentedWhereUsed", tr("&Indented..."), SLOT(sDspIndentedWhereUsed()), reportsWhereUsdMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },

  { "pd.dspPendingBOMChanges", tr("&Pending BOM Changes..."), SLOT(sDspPendingBOMChanges()), reportsMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },
  { "separator", NULL, NULL, reportsMenu,	true, NULL, NULL, true , NULL },
  { "pd.dspOperationsByWorkCenter", tr("&Operations..."), SLOT(sDspOperationsByWorkCenter()), reportsMenu, _privileges->check("ViewBOOs"), NULL, NULL, _metrics->boolean("Routings") , NULL },
  { "pd.dspStandardOperationsByWorkCenter", tr("&Standard Operations..."), SLOT(sDspStandardOperByWorkCenter()), reportsMenu, _privileges->check("ViewStandardOperations"), NULL, NULL, _metrics->boolean("Routings") , NULL },
  { "separator", NULL, NULL, reportsMenu,	true, NULL, NULL,  _metrics->boolean("Routings") , NULL },

  // Product | Reports | Capacity UOMs
  { "menu",	tr("Capacity &UOMs"), (char*)reportsCapUomMenu,	reportsMenu, true, NULL, NULL, true , NULL },
  { "pd.dspCapacityUOMsByProductCategory", tr("by &Product Category..."), SLOT(sDspCapacityUOMsByProductCategory()), reportsCapUomMenu, _privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters"), NULL, NULL, true , NULL },
  { "pd.dspCapacityUOMsByClassCode", tr("by &Class Code..."), SLOT(sDspCapacityUOMsByClassCode()), reportsCapUomMenu, _privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters"), NULL, NULL, true , NULL },
  
  {  "separator", NULL, NULL, mainMenu,	true, NULL, NULL, true , NULL },

  // Product | Items
  { "menu",	tr("&Item"), (char*)itemsMenu,	mainMenu, true, NULL, NULL, true , NULL },
  { "pd.enterNewItem", tr("&New..."), SLOT(sNewItem()), itemsMenu, _privileges->check("MaintainItemMasters"), NULL, NULL, true , NULL },
  { "pd.listItems", tr("&List..."), SLOT(sItems()), itemsMenu, _privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters"), new QPixmap(":/images/items.png"), toolBar, true , tr("List Items") },
  { "pd.searchForItems", tr("&Search..."),SLOT(sSearchForItems()), itemsMenu, (_privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters")), NULL, NULL, true , NULL },
  { "pd.copyItem", tr("&Copy..."), SLOT(sCopyItem()), itemsMenu, _privileges->check("MaintainItemMasters") , NULL, NULL, true, NULL },
  { "separator", NULL, NULL, itemsMenu,	true, NULL, NULL, true , NULL },
  { "pd.itemAvailabilityWorkbench", tr("&Workbench..."), SLOT(sDspItemAvailabilityWorkbench()), itemsMenu, _privileges->check("ViewItemAvailabilityWorkbench"), NULL, NULL, true , NULL },
  { "separator", NULL, NULL, itemsMenu,	true, NULL, NULL, true , NULL },
  { "pd.itemGroups", tr("&Groups..."), SLOT(sItemGroups()), itemsMenu, (_privileges->check("MaintainItemGroups") || _privileges->check("ViewItemGroups")), NULL, NULL, true , NULL },
  { "pd.itemImages", tr("&Images..."), SLOT(sItemImages()), itemsMenu, (_privileges->check("MaintainItemMasters") || _privileges->check("ViewItemMasters")), NULL, NULL, true , NULL },

  // Product | Bill of Materials
  { "menu",	tr("Bill Of Ma&terials"), (char*)bomMenu,	mainMenu, true, NULL, NULL, true , NULL },
  { "pd.enterNewBOM", tr("&New..."), SLOT(sNewBOM()), bomMenu, _privileges->check("MaintainBOMs"), NULL, NULL, true , NULL },
  { "pd.listBOMs", tr("&List..."), SLOT(sBOMs()), bomMenu, _privileges->check("MaintainBOMs") || _privileges->check("ViewBOMs"), new QPixmap(":/images/boms.png"), toolBar, true , tr("List Bill of Materials") },
  { "pd.copyBOM", tr("&Copy..."), SLOT(sCopyBOM()), bomMenu, _privileges->check("MaintainBOMs"), NULL, NULL, true , NULL },
  { "separator", NULL, NULL, bomMenu,	true, NULL, NULL, true , NULL },
  { "pd.massReplaceComponentItem", tr("Mass &Replace..."), SLOT(sMassReplaceComponent()), bomMenu, _privileges->check("MaintainBOMs"), NULL, NULL, true , NULL },
  { "pd.massExpireComponentItem", tr("Mass E&xpire..."), SLOT(sMassExpireComponent()),  bomMenu, _privileges->check("MaintainBOMs"), NULL, NULL, true , NULL },

  // Product | Bill of Operations...
  { "menu",	tr("Bill Of &Operations"), (char*)booMenu,	mainMenu, true, NULL, NULL,  _metrics->boolean("Routings") , NULL },
  { "pd.enterNewBOO", tr("&New..."), SLOT(sNewBOO()), booMenu, (_privileges->check("MaintainBOOs") && _metrics->boolean("Routings")), NULL, NULL,  _metrics->boolean("Routings") , NULL },
  { "pd.listBOOs", tr("&List..."), SLOT(sBOOs()), booMenu, _privileges->check("MaintainBOOs") || _privileges->check("ViewBOOs"), new QPixmap(":/images/boos.png"), toolBar, _metrics->boolean("Routings") , tr("List Bill of Operations") },
  { "pd.copyBOO", tr("&Copy..."), SLOT(sCopyBOO()), booMenu, _privileges->check("MaintainBOOs"), NULL, NULL, _metrics->boolean("Routings") , NULL },

  // Product | Breeder Bill of Materials
  { "menu",	tr("&Breeder Bill Of Materials"), (char*)breederBOMMenu,	mainMenu, true, NULL, NULL,  _metrics->boolean("BBOM") , NULL },
  { "pd.enterNewBreederBOM", tr("&New..."), SLOT(sNewBreederBOM()), breederBOMMenu, _privileges->check("MaintainBBOMs"), NULL, NULL, _metrics->boolean("BBOM") , NULL },
  { "pd.listBreederBOMs", tr("&List..."), SLOT(sBreederBOMs()), breederBOMMenu, _privileges->check("MaintainBBOMs") || _privileges->check("ViewBBOMs"), NULL, NULL, _metrics->boolean("BBOM") , NULL },
  
  // Product | Costing
  { "menu",	tr("&Costing"), (char*)costingMenu,	mainMenu, true, NULL, NULL, true , NULL },
  { "pd.maintainItemCosts", tr("&Maintain Item Costs..."), SLOT(sMaintainItemCosts()), costingMenu, _privileges->check("ViewCosts"), NULL, NULL, true , NULL },
  
  // Product | Lot Serial
  { "pd.lotSerial",tr("&Lot/Serial..."),SLOT(sLotSerial()),mainMenu, TRUE,NULL, NULL, _metrics->boolean("LotSerialControl"), NULL }, 

  { "separator", NULL, NULL, costingMenu,	true, NULL, NULL, true , NULL },
  
  // Product | Costing | Update Actual Costs
  { "menu",	tr("Update &Actual Costs"), (char*)costingUpdActMenu,	costingMenu, true, NULL, NULL, true , NULL },
  { "pd.updateActualCostsByItem", tr("by &Item..."), SLOT(sUpdateActualCostsByItem()), costingUpdActMenu, _privileges->check("UpdateActualCosts"), NULL, NULL, true , NULL },
  { "pd.updateActualCostsByClassCode", tr("by &Class Code..."), SLOT(sUpdateActualCostsByClassCode()), costingUpdActMenu, _privileges->check("UpdateActualCosts"), NULL, NULL, true , NULL },

  // Product | Costing | Post Actual Costs
  { "menu",	tr("&Post Actual Costs"), (char*)costingPostActMenu,	costingMenu, true, NULL, NULL, true , NULL },
  { "pd.postActualCostsByItem", tr("by &Item..."), SLOT(sPostActualCostsByItem()), costingPostActMenu, _privileges->check("PostActualCosts"), NULL, NULL, true , NULL },
  { "pd.postActualCostsByClassCode", tr("by &Class Code..."), SLOT(sPostActualCostsByClassCode()), costingPostActMenu, _privileges->check("PostActualCosts"), NULL, NULL, true , NULL },

  // Product | Costing | Post Standard Costs
  { "menu",	tr("Post &Standard Costs"), (char*)costingUpdStdMenu,	costingMenu, true, NULL, NULL, true , NULL },
  { "pd.postStandardCostsByItem", tr("by &Item..."), SLOT(sUpdateStandardCostsByItem()), costingUpdStdMenu, _privileges->check("PostStandardCosts"), NULL, NULL, true , NULL },
  { "pd.postStandardCostsByClassCode", tr("by &Class Code..."), SLOT(sUpdateStandardCostsByClassCode()), costingUpdStdMenu, _privileges->check("PostStandardCosts"), NULL, NULL, true , NULL },

  { "separator", NULL, NULL, costingMenu,	true, NULL, NULL, true , NULL },

  //  Product | Costing | Reports
  { "menu",	tr("&Reports"), (char*)costingReportsMenu,	costingMenu, true, NULL, NULL, true , NULL },
  
  //  Product | Costing | Reports | Costed BOM
  { "menu",	tr("&Costed BOM"), (char*)costingReportsCostedMenu,	costingReportsMenu, true, NULL, NULL, true , NULL },
  { "pd.dspCostedSingleLevelBOM", tr("&Single Level..."), SLOT(sDspCostedSingleLevelBOM()),costingReportsCostedMenu, _privileges->check("ViewCosts"), NULL, NULL, true , NULL },
  { "pd.dspCostedIndentedBOM", tr("&Indented..."), SLOT(sDspCostedIndentedBOM()), costingReportsCostedMenu, _privileges->check("ViewCosts"), NULL, NULL, true , NULL },
  { "pd.dspCostedSummarizedBOM", tr("Summari&zed..."), SLOT(sDspCostedSummarizedBOM()), costingReportsCostedMenu, _privileges->check("ViewCosts"), NULL, NULL, true , NULL },
  
  //  Product | Costing | Reports | Item Costs
  { "menu",	tr("&Item Costs"), (char*)costingReportsItemCostsMenu,	costingReportsMenu, true, NULL, NULL, true , NULL },
  { "pd.dspItemCostsByClassCode", tr("by &Class Code..."), SLOT(sDspItemCostsByClassCode()), costingReportsItemCostsMenu, _privileges->check("ViewCosts"), NULL, NULL, true , NULL },
  { "pd.dspItemCostsSummary", tr("&Summary..."), SLOT(sDspItemCostSummary()), costingReportsItemCostsMenu, _privileges->check("ViewCosts"), NULL, NULL, true , NULL },
  { "pd.dspItemCostsHistory", tr("&History..."), SLOT(sDspItemCostHistory()), costingReportsItemCostsMenu, _privileges->check("ViewCosts"), NULL, NULL, true , NULL },

  { "separator", NULL, NULL, costingMenu,	true, NULL, NULL, true , NULL },
  { "pd.userDefinedCostingElements", tr("&User-Defined Costing Elements..."), SLOT(sUserCostingElements()), costingMenu, _privileges->check("MaintainUserCostingElements"), NULL, NULL, true , NULL },

  { "separator", NULL, NULL, mainMenu,	true, NULL, NULL, true , NULL },

  //  Produt | Master Information
  { "menu",	tr("&Master Information"), (char*)masterInfoMenu, mainMenu, true, NULL, NULL, true , NULL },
  { "pd.unitsOfMeasure", tr("&Units of Measure..."), SLOT(sUnitsOfMeasure()), masterInfoMenu, _privileges->check("MaintainUOMs") || _privileges->check("ViewUOMs"), NULL, NULL, true , NULL },
  { "pd.classCodes", tr("&Class Codes..."), SLOT(sClassCodes()), masterInfoMenu, _privileges->check("MaintainClassCodes") || _privileges->check("ViewClassCodes"), NULL, NULL, true , NULL },
  { "pd.productCategories", tr("&Product Categories..."), SLOT(sProductCategories()), masterInfoMenu, _privileges->check("MaintainProductCategories") || _privileges->check("ViewProductCategories"), NULL, NULL, true , NULL },
  { "pd.freightClasses", tr("&Freight Classes..."), SLOT(sFreightClasses()), masterInfoMenu, _privileges->check("MaintainFreightClasses") || _privileges->check("ViewFreightClasses"), NULL, NULL, true , NULL },
  { "pd.characteristics", tr("C&haracteristics..."), SLOT(sCharacteristics()), masterInfoMenu, _privileges->check("MaintainCharacteristics") || _privileges->check("ViewCharacteristics"), NULL, NULL, true , NULL },
  { "separator", NULL, NULL, masterInfoMenu,	true, NULL, NULL, _metrics->boolean("Routings")  , NULL },
  { "pd.standardLaborRates", tr("Standard &Labor Rates..."), SLOT(sStandardLaborRates()), masterInfoMenu, _privileges->check("MaintainLaborRates") || _privileges->check("ViewLaborRates"), NULL, NULL, _metrics->boolean("Routings") , NULL },
  { "pd.workCenters", tr("&Work Centers..."), SLOT(sWorkCenters()), masterInfoMenu, _privileges->check("MaintainWorkCenters") || _privileges->check("ViewWorkCenters"), NULL, NULL,  _metrics->boolean("Routings") , NULL },
  { "pd.standardOperations", tr("Standard &Operations..."), SLOT(sStandardOperations()), masterInfoMenu, _privileges->check("MaintainStandardOperations") || _privileges->check("ViewStandardOperations"), NULL, NULL, _metrics->boolean("Routings") , NULL },

  //  Produt | Utilies
  { "menu",	tr("&Utilities"), (char*)utilitiesMenu, mainMenu, true, NULL, NULL, true , NULL },
  { "pd.dspUnusedPurchasedItems", tr("Unused &Purchased Items..."), SLOT(sDspUnusedPurchasedItems()), utilitiesMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },
  { "pd.dspUndefinedManufacturedItems", tr("Undefined &Manufactured Items..."), SLOT(sDspUndefinedManufacturedItems()), utilitiesMenu, _privileges->check("ViewBOMs") || _privileges->check("ViewBOOs"), NULL, NULL, true , NULL },
  { "pd.dspBillsOfMaterialsWithoutComponentItemSites", tr("Bills of Ma&terials without Component Item Sites..."), SLOT(sDspInvalidBillsOfMaterials()), utilitiesMenu, _privileges->check("ViewBOMs"), NULL, NULL, true , NULL },
  { "separator", NULL, NULL, utilitiesMenu,	true, NULL, NULL, true , NULL },
  { "pd.reassignClassCodeByClassCode", tr("Reassign &Class Codes..."), SLOT(sReassignClassCodeByClassCode()), utilitiesMenu, _privileges->check("MaintainItemMasters"), NULL, NULL, true , NULL },
  { "pd.reassignProductCategoryByProductCategory", tr("&Reassign Product Categories..."), SLOT(sReassignProductCategoryByProductCategory()), utilitiesMenu, _privileges->check("MaintainItemMasters"), NULL, NULL, true , NULL },
  };

  addActionsToMenu(acts, sizeof(acts) / sizeof(acts[0]));

  parent->populateCustomMenu(mainMenu, "Products");
  parent->menuBar()->insertItem(tr("&Products"), mainMenu);
}

void menuProducts::addActionsToMenu(actionProperties acts[], unsigned int numElems)
{
  for (unsigned int i = 0; i < numElems; i++)
  {
    if (! acts[i].visible)
    {
      continue;
    }
    else if (acts[i].actionName == QString("menu"))
    {
      acts[i].menu->insertItem(acts[i].actionTitle, (QMenu*)(acts[i].slot));
    }
    else if (acts[i].actionName == QString("separator"))
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

void menuProducts::sNewItem()
{
  item::newItem();
}

void menuProducts::sItems()
{
  omfgThis->handleNewWindow(new items());
}

void menuProducts::sSearchForItems()
{
  omfgThis->handleNewWindow(new searchForItem());
}

void menuProducts::sCopyItem()
{
  copyItem(parent, "", TRUE).exec();
}

void menuProducts::sItemGroups()
{
  omfgThis->handleNewWindow(new itemGroups());
}

void menuProducts::sItemImages()
{
  omfgThis->handleNewWindow(new itemImages());
}

void menuProducts::sNewBOM()
{
  ParameterList params;
  params.append("mode", "new");

  BOM *newdlg = new BOM();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuProducts::sBOMs()
{
  omfgThis->handleNewWindow(new bomList());
}

void menuProducts::sCopyBOM()
{
  copyBOM(parent, "", TRUE).exec();
}

void menuProducts::sMassReplaceComponent()
{
  omfgThis->handleNewWindow(new massReplaceComponent());
}

void menuProducts::sMassExpireComponent()
{
  omfgThis->handleNewWindow(new massExpireComponent());
}

void menuProducts::sNewBOO()
{
  ParameterList params;
  params.append("mode", "new");

  boo *newdlg = new boo();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuProducts::sBOOs()
{
  omfgThis->handleNewWindow(new booList());
}

void menuProducts::sCopyBOO()
{
  copyBOO(parent, "", TRUE).exec();
}

void menuProducts::sNewBreederBOM()
{
  ParameterList params;
  params.append("mode", "new");

  bbom *newdlg = new bbom();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void menuProducts::sBreederBOMs()
{
  omfgThis->handleNewWindow(new bboms());
}

//  Costing
void menuProducts::sMaintainItemCosts()
{
  omfgThis->handleNewWindow(new maintainItemCosts());
}

void menuProducts::sUpdateActualCostsByItem()
{
  ParameterList params;
  params.append("costtype", "actual");

  updateActualCostsByItem *newdlg = new updateActualCostsByItem(parent, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
}

void menuProducts::sUpdateActualCostsByClassCode()
{
  ParameterList params;
  params.append("costtype", "actual");

  updateActualCostsByClassCode *newdlg = new updateActualCostsByClassCode(parent, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
}

void menuProducts::sPostActualCostsByItem()
{
  postCostsByItem(parent, "", TRUE).exec();
}

void menuProducts::sPostActualCostsByClassCode()
{
  postCostsByClassCode(parent, "", TRUE).exec();
}

void menuProducts::sUpdateStandardCostsByItem()
{
  ParameterList params;
  params.append("costtype", "standard");

  updateActualCostsByItem *newdlg = new updateActualCostsByItem(parent, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
}

void menuProducts::sUpdateStandardCostsByClassCode()
{
  ParameterList params;
  params.append("costtype", "standard");

  updateActualCostsByClassCode *newdlg = new updateActualCostsByClassCode(parent, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
}

void menuProducts::sDspCostedSingleLevelBOM()
{
  omfgThis->handleNewWindow(new dspCostedSingleLevelBOM());
}

void menuProducts::sDspCostedIndentedBOM()
{
  omfgThis->handleNewWindow(new dspCostedIndentedBOM());
}

void menuProducts::sDspCostedSummarizedBOM()
{
  omfgThis->handleNewWindow(new dspCostedSummarizedBOM());
}

void menuProducts::sDspItemCostsByClassCode()
{
  omfgThis->handleNewWindow(new dspItemCostsByClassCode());
}

void menuProducts::sDspItemCostSummary()
{
  omfgThis->handleNewWindow(new dspItemCostSummary());
}

void menuProducts::sDspItemCostHistory()
{
  omfgThis->handleNewWindow(new dspItemCostHistory());
}

void menuProducts::sUserCostingElements()
{
  omfgThis->handleNewWindow(new costingElements());
}

void menuProducts::sLotSerial()
{
  lotSerial newdlg(parent, "", TRUE);
  newdlg.exec();
}

//  Displays
void menuProducts::sDspItemsByClassCode()
{
  omfgThis->handleNewWindow(new dspItemsByClassCode());
}

void menuProducts::sDspItemsByProductCategory()
{
  omfgThis->handleNewWindow(new dspItemsByProductCategory());
}

void menuProducts::sDspItemsByCharacteristic()
{
  omfgThis->handleNewWindow(new dspItemsByCharacteristic());
}

void menuProducts::sDspSingleLevelBOM()
{
  omfgThis->handleNewWindow(new dspSingleLevelBOM());
}

void menuProducts::sDspIndentedBOM()
{
  omfgThis->handleNewWindow(new dspIndentedBOM());
}

void menuProducts::sDspSummarizedBOM()
{
  omfgThis->handleNewWindow(new dspSummarizedBOM());
}

void menuProducts::sDspSequencedBOM()
{
  omfgThis->handleNewWindow(new dspSequencedBOM());
}

void menuProducts::sDspSingleLevelWhereUsed()
{
  omfgThis->handleNewWindow(new dspSingleLevelWhereUsed());
}

void menuProducts::sDspIndentedWhereUsed()
{
  omfgThis->handleNewWindow(new dspIndentedWhereUsed());
}

void menuProducts::sDspPendingBOMChanges()
{
  omfgThis->handleNewWindow(new dspPendingBOMChanges());
}

void menuProducts::sDspOperationsByWorkCenter()
{
  omfgThis->handleNewWindow(new dspOperationsByWorkCenter());
}

void menuProducts::sDspStandardOperByWorkCenter()
{
  omfgThis->handleNewWindow(new dspStandardOperationsByWorkCenter());
}

void menuProducts::sDspCapacityUOMsByClassCode()
{
  omfgThis->handleNewWindow(new dspCapacityUOMsByClassCode());
}

void menuProducts::sDspCapacityUOMsByProductCategory()
{
  omfgThis->handleNewWindow(new dspCapacityUOMsByProductCategory());
}

void menuProducts::sDspItemAvailabilityWorkbench()
{
  omfgThis->handleNewWindow(new itemAvailabilityWorkbench());
}

//  Master Information
void menuProducts::sUnitsOfMeasure()
{
  omfgThis->handleNewWindow(new uoms());
}

void menuProducts::sClassCodes()
{
  omfgThis->handleNewWindow(new classCodes());
}

void menuProducts::sProductCategories()
{
  omfgThis->handleNewWindow(new productCategories());
}

void menuProducts::sFreightClasses()
{
  omfgThis->handleNewWindow(new freightClasses());
}

void menuProducts::sCharacteristics()
{
  omfgThis->handleNewWindow(new characteristics());
}

void menuProducts::sStandardLaborRates()
{
  omfgThis->handleNewWindow(new laborRates());
}

void menuProducts::sWorkCenters()
{
  omfgThis->handleNewWindow(new workCenters());
}

void menuProducts::sStandardOperations()
{
  omfgThis->handleNewWindow(new standardOperations());
}

//  Utilities
void menuProducts::sDspUnusedPurchasedItems()
{
  omfgThis->handleNewWindow(new dspUnusedPurchasedItems());
}

void menuProducts::sDspUndefinedManufacturedItems()
{
  omfgThis->handleNewWindow(new dspUndefinedManufacturedItems());
}

void menuProducts::sDspInvalidBillsOfMaterials()
{
  omfgThis->handleNewWindow(new dspInvalidBillsOfMaterials());
}

void menuProducts::sReassignClassCodeByClassCode()
{
  reassignClassCodeByClassCode(parent, "", TRUE).exec();
}

void menuProducts::sReassignProductCategoryByProductCategory()
{
  reassignProductCategoryByProductCategory(parent, "", TRUE).exec();
}
