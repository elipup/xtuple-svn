/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspSlowMovingInventoryByClassCode.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>

#include <metasql.h>
#include <openreports.h>
#include <parameter.h>

#include "guiclient.h"
#include "adjustmentTrans.h"
#include "enterMiscCount.h"
#include "transferTrans.h"
#include "createCountTagsByItem.h"

dspSlowMovingInventoryByClassCode::dspSlowMovingInventoryByClassCode(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  _costsGroupInt = new QButtonGroup(this);
  _costsGroupInt->addButton(_useStandardCosts);
  _costsGroupInt->addButton(_useActualCosts);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_itemsite, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));
  connect(_showValue, SIGNAL(toggled(bool)), this, SLOT(sHandleValue(bool)));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));

  _classCode->setType(ParameterGroup::ClassCode);

  _itemsite->addColumn(tr("Site"),          _whsColumn,  Qt::AlignCenter, true,  "warehous_code" );
  _itemsite->addColumn(tr("Item Number"),   _itemColumn, Qt::AlignLeft,   true,  "item_number"   );
  _itemsite->addColumn(tr("Description"),   -1,          Qt::AlignLeft,   true,  "itemdescrip"   );
  _itemsite->addColumn(tr("UOM"),           _uomColumn,  Qt::AlignCenter, true,  "uom_name" );
  _itemsite->addColumn(tr("Last Movement"), _itemColumn, Qt::AlignCenter, true,  "itemsite_datelastused" );
  _itemsite->addColumn(tr("QOH"),           _qtyColumn,  Qt::AlignRight,  true,  "itemsite_qtyonhand"  );
  _itemsite->addColumn(tr("Unit Cost"),     _costColumn, Qt::AlignRight,  true,  "cost"  );
  _itemsite->addColumn(tr("Value"),         _costColumn, Qt::AlignRight,  true,  "value"  );

  sHandleValue(_showValue->isChecked());

  _showValue->setEnabled(_privileges->check("ViewInventoryValue"));
}

dspSlowMovingInventoryByClassCode::~dspSlowMovingInventoryByClassCode()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspSlowMovingInventoryByClassCode::languageChange()
{
  retranslateUi(this);
}

bool dspSlowMovingInventoryByClassCode::setParams(ParameterList & params)
{
  if(!_cutoffDate->isValid())
  {
    QMessageBox::warning(this, tr("No Cutoff Date"),
        tr("You must specify a cutoff date."));
    _cutoffDate->setFocus();
    return false;
  }

  params.append("cutoffDate", _cutoffDate->date());
  _warehouse->appendValue(params);
  _classCode->appendValue(params);

  if(_showValue->isChecked())
    params.append("showValue");

  if (_useStandardCosts->isChecked())
    params.append("useStandardCosts");

  if (_useActualCosts->isChecked())
    params.append("useActualCosts");

  return true;
}

void dspSlowMovingInventoryByClassCode::sPrint()
{
  ParameterList params;
  if (! setParams(params))
    return;

  orReport report("SlowMovingInventoryByClassCode", params);

  if (report.isValid())
    report.print();
  else
  {
    report.reportError(this);
  }
}

void dspSlowMovingInventoryByClassCode::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pSelected)
{
  int menuItem;

  if (((XTreeWidgetItem *)pSelected)->id() != -1)
  {
    menuItem = pMenu->insertItem(tr("Transfer to another Site..."), this, SLOT(sTransfer()), 0);
    if (!_privileges->check("CreateInterWarehouseTrans"))
      pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem(tr("Adjust this QOH..."), this, SLOT(sAdjust()), 0);
    if (!_privileges->check("CreateAdjustmentTrans"))
      pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem(tr("Reset this QOH to 0..."), this, SLOT(sReset()), 0);
    if (!_privileges->check("CreateAdjustmentTrans"))
      pMenu->setItemEnabled(menuItem, FALSE);

    pMenu->insertSeparator();

    menuItem = pMenu->insertItem(tr("Enter Misc. Count..."), this, SLOT(sMiscCount()), 0);
    if (!_privileges->check("EnterMiscCounts"))
      pMenu->setItemEnabled(menuItem, FALSE);

    pMenu->insertSeparator();

    menuItem = pMenu->insertItem(tr("Issue Count Tag..."), this, SLOT(sIssueCountTag()), 0);
    if (!_privileges->check("IssueCountTags"))
      pMenu->setItemEnabled(menuItem, FALSE);
  } 
}

void dspSlowMovingInventoryByClassCode::sTransfer()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _itemsite->id());

  transferTrans *newdlg = new transferTrans();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspSlowMovingInventoryByClassCode::sAdjust()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _itemsite->id());

  adjustmentTrans *newdlg = new adjustmentTrans();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspSlowMovingInventoryByClassCode::sReset()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _itemsite->id());
  params.append("qty", 0.0);

  adjustmentTrans *newdlg = new adjustmentTrans();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspSlowMovingInventoryByClassCode::sMiscCount()
{
  ParameterList params;
  params.append("itemsite_id", _itemsite->id());

  enterMiscCount newdlg(this, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec())
    sFillList();
}

void dspSlowMovingInventoryByClassCode::sIssueCountTag()
{
  ParameterList params;
  params.append("itemsite_id", _itemsite->id());
  
  createCountTagsByItem newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspSlowMovingInventoryByClassCode::sHandleValue(bool pShowValue)
{
  _itemsite->setColumnHidden(6, !pShowValue);
  _itemsite->setColumnHidden(7, !pShowValue);
}

void dspSlowMovingInventoryByClassCode::sFillList()
{
  ParameterList params;
  if (! setParams(params))
    return;

  _itemsite->clear();

  QString sql( "SELECT itemsite_id, warehous_code, item_number,"
               "       (item_descrip1 || ' ' || item_descrip2) AS itemdescrip, uom_name,"
               "       itemsite_qtyonhand, itemsite_datelastused, cost,"
               "       noNeg(cost * itemsite_qtyonhand) AS value,"
               "       CASE WHEN (COALESCE(itemsite_datelastused, startOfTime()) <= startOfTime()) THEN 'N/A'"
               "       END AS itemsite_datelastused_qtdisplayrole,"
               "       'qty' AS itemsite_qtyonhand_xtnumericrole,"
               "       'cost' AS cost_xtnumericrole,"
               "       'curr' AS value_xtnumericrole,"
               "       0 AS itemsite_qtyonhand_xttotalrole,"
               "       0 AS value_xttotalrole "
               "FROM ( SELECT itemsite_id, warehous_code, item_number,"
               "              item_descrip1, item_descrip2, uom_name,"
               "              itemsite_qtyonhand, itemsite_datelastused,"
	       "<? if exists(\"useActualCosts\") ?>"
	       "              actcost(itemsite_item_id) "
	       "<? else ?>"
	       "              stdcost(itemsite_item_id) "
	       "<? endif ?> AS cost "
	       "FROM itemsite, item, warehous, uom "
	       "WHERE ((itemsite_item_id=item_id)"
               " AND (item_inv_uom_id=uom_id)"
	       " AND (itemsite_warehous_id=warehous_id)"
	       " AND (itemsite_active)"
	       " AND (itemsite_datelastused < <? value(\"cutoffDate\") ?>)"
	       "<? if exists(\"warehous_id\") ?>"
	       " AND (itemsite_warehous_id=<? value(\"warehous_id\") ?>)"
	       "<? endif ?>"
	       "<? if exists(\"classcode_id\") ?>"
	       " AND (item_classcode_id=<? value(\"classcode_id\") ?>)"
	       "<? elseif exists(\"classcode_pattern\") ?>"
	       " AND (item_classcode_id IN (SELECT classcode_id FROM classcode WHERE classcode_code ~ <? value(\"classcode_pattern\") ?>))"
	       "<? endif ?>"
	       ") ) AS data "
	       "ORDER BY warehous_code, "
	       "<? if exists(\"sortByItem\") ?>"
	       "         item_number"
	       "<? elseif exists(\"sortByDate\") ?>"
	       "         itemsite_datelastused"
	       "<? else ?>"
	       "         noNeg(cost * itemsite_qtyonhand) DESC"
	       "<? endif ?>"
	       ";");

  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  _itemsite->populate(q);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
