/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspQOHByParameterList.h"

#include <QVariant>
#include <QMenu>

#include <openreports.h>

#include "adjustmentTrans.h"
#include "enterMiscCount.h"
#include "transferTrans.h"
#include "createCountTagsByItem.h"
#include "dspInventoryLocator.h"

dspQOHByParameterList::dspQOHByParameterList(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  _costsGroupInt = new QButtonGroup(this);
  _costsGroupInt->addButton(_useStandardCosts);
  _costsGroupInt->addButton(_useActualCosts);
  _costsGroupInt->addButton(_usePostedCosts);

  _showGroupInt = new QButtonGroup(this);
  _showGroupInt->addButton(_showAll);
  _showGroupInt->addButton(_showPositive);
  _showGroupInt->addButton(_showNegative);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_qoh, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_showValue, SIGNAL(toggled(bool)), this, SLOT(sHandleValue(bool)));

  _qoh->addColumn(tr("Site"),             _whsColumn,  Qt::AlignCenter, true,  "warehous_code" );
  _qoh->addColumn(tr("Class Code"),       _itemColumn, Qt::AlignLeft,   true,  "classcode_code"   );
  _qoh->addColumn(tr("Item Number"),      _itemColumn, Qt::AlignLeft,   true,  "item_number"   );
  _qoh->addColumn(tr("Description"),      -1,          Qt::AlignLeft,   true,  "itemdescrip"   );
  _qoh->addColumn(tr("UOM"),              _uomColumn,  Qt::AlignCenter, true,  "uom_name" );
  _qoh->addColumn(tr("Default Location"), _itemColumn, Qt::AlignLeft,   true,  "defaultlocation"   );
  _qoh->addColumn(tr("Reorder Lvl."),     _qtyColumn,  Qt::AlignRight,  true,  "reorderlevel"  );
  _qoh->addColumn(tr("QOH"),              _qtyColumn,  Qt::AlignRight,  true,  "qoh"  );
  _qoh->addColumn(tr("Non-Netable"),      _qtyColumn,  Qt::AlignRight,  true,  "f_nnqoh"  );
  _qoh->addColumn(tr("Unit Cost"),        _costColumn, Qt::AlignRight,  true,  "cost"  );
  _qoh->addColumn(tr("Value"),            _costColumn, Qt::AlignRight,  true,  "value"  );
  _qoh->addColumn(tr("NN Value"),         _costColumn, Qt::AlignRight,  true,  "f_nnvalue"  );
  _qoh->addColumn(tr("Cost Method"),      _costColumn, Qt::AlignCenter, true,  "f_costmethod" );

  sHandleValue(_showValue->isChecked());

  _showValue->setEnabled(_privileges->check("ViewInventoryValue"));
}

dspQOHByParameterList::~dspQOHByParameterList()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspQOHByParameterList::languageChange()
{
  retranslateUi(this);
}

void dspQOHByParameterList::sPrint()
{
  ParameterList params;
  _warehouse->appendValue(params);
  _parameter->appendValue(params);

  if (_parameter->type() == ParameterGroup::ItemGroup)
    params.append("itemgrp");
  else if(_parameter->type() == ParameterGroup::ClassCode)
    params.append("classcode");

  if (_showPositive->isChecked())
    params.append("onlyShowPositive");

  if (_showNegative->isChecked())
    params.append("onlyShowNegative");

  if (_showValue->isChecked())
    params.append("showValue");

  if (_useStandardCosts->isChecked())
    params.append("useStandardCosts");

  if (_useActualCosts->isChecked())
    params.append("useActualCosts");

  if (_usePostedCosts->isChecked())
    params.append("usePostedCosts");

  orReport report("QOHByParameterList", params);

  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

SetResponse dspQOHByParameterList::set(const ParameterList &pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("classcode", &valid);
  if(valid)
    _parameter->setType(ParameterGroup::ClassCode);

  param = pParams.value("classcode_id", &valid);
  if (valid)
  {
    _parameter->setType(ParameterGroup::ClassCode);
    _parameter->setId(param.toInt());
  }

  param = pParams.value("classcode_pattern", &valid);
  if (valid)
  {
    _parameter->setType(ParameterGroup::ClassCode);
    _parameter->setPattern(param.toString());
  }

  param = pParams.value("itemgrp", &valid);
  if(valid)
    _parameter->setType(ParameterGroup::ItemGroup);

  param = pParams.value("itemgrp_id", &valid);
  if (valid)
  {
    _parameter->setType(ParameterGroup::ItemGroup);
    _parameter->setId(param.toInt());
  }

  param = pParams.value("itemgrp_pattern", &valid);
  if (valid)
  {
    _parameter->setType(ParameterGroup::ItemGroup);
    _parameter->setPattern(param.toString());
  }

  param = pParams.value("warehous_id", &valid);
  if (valid)
    _warehouse->setId(param.toInt());

  if (pParams.inList("run"))
    sFillList();

  switch (_parameter->type())
  {
    case ParameterGroup::ClassCode:
      setWindowTitle(tr("Quantities on Hand by Class Code"));
      break;

    case ParameterGroup::ItemGroup:
      setWindowTitle(tr("Quantities on Hand by Item Group"));
      break;

    default:
      break;
  }

  return NoError;
}

void dspQOHByParameterList::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pSelected)
{
  if (((XTreeWidgetItem *)pSelected)->id() != -1)
  {
    int menuItem;
  
    if (((XTreeWidgetItem *)pSelected)->altId())
    {
      pMenu->insertItem(tr("View Location/Lot/Serial # Detail..."), this, SLOT(sViewDetail()), 0);
      pMenu->insertSeparator();
    }

    if (_metrics->boolean("MultiWhs"))
    {
      menuItem = pMenu->insertItem(tr("Transfer to another Site..."), this, SLOT(sTransfer()), 0);
      if (!_privileges->check("CreateInterWarehouseTrans"))
        pMenu->setItemEnabled(menuItem, FALSE);
    }

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

void dspQOHByParameterList::sViewDetail()
{
  ParameterList params;
  params.append("itemsite_id", _qoh->id());
  params.append("run");

  dspInventoryLocator *newdlg = new dspInventoryLocator();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspQOHByParameterList::sTransfer()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _qoh->id());

  transferTrans *newdlg = new transferTrans();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspQOHByParameterList::sAdjust()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _qoh->id());

  adjustmentTrans *newdlg = new adjustmentTrans();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspQOHByParameterList::sReset()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("itemsite_id", _qoh->id());
  params.append("qty", 0.0);

  adjustmentTrans *newdlg = new adjustmentTrans();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspQOHByParameterList::sMiscCount()
{
  ParameterList params;
  params.append("itemsite_id", _qoh->id());

  enterMiscCount newdlg(this, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec())
    sFillList();
}

void dspQOHByParameterList::sIssueCountTag()
{
  ParameterList params;
  params.append("itemsite_id", _qoh->id());
  
  createCountTagsByItem newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspQOHByParameterList::sHandleValue(bool pShowValue)
{
  _qoh->setColumnHidden(_qoh->column("cost"),         !pShowValue);
  _qoh->setColumnHidden(_qoh->column("value"),        !pShowValue);
  _qoh->setColumnHidden(_qoh->column("f_nnvalue"),    !pShowValue);
  _qoh->setColumnHidden(_qoh->column("f_costmethod"), !pShowValue);
}

void dspQOHByParameterList::sFillList()
{
  _qoh->clear();
  _qoh->setColumnVisible(_qoh->column("f_costmethod"),
                         _showValue->isChecked() && _usePostedCosts->isChecked());
  
  QString sql( "SELECT itemsite_id, detail,"
               "       warehous_code, classcode_code, item_number, uom_name,"
               "       (item_descrip1 || ' ' || item_descrip2) AS itemdescrip,"
               "       defaultlocation,"
               "       reorderlevel, qoh, nnqoh,"
               "       CASE WHEN (itemsite_loccntrl) THEN nnqoh END AS f_nnqoh,"
               "       cost, (cost * qoh) AS value,"
               "       CASE WHEN (itemsite_loccntrl) THEN (cost * nnqoh) END AS f_nnvalue,"
               "       CASE WHEN(itemsite_costmethod='A') THEN 'Average'"
               "            WHEN(itemsite_costmethod='S') THEN 'Standard'"
               "            WHEN(itemsite_costmethod='J') THEN 'Job'"
               "            WHEN(itemsite_costmethod='N') THEN 'None'"
               "            ELSE 'UNKNOWN'"
               "       END AS f_costmethod,"
               "       'qty' AS reorderlevel_xtnumericrole,"
               "       'qty' AS qoh_xtnumericrole,"
               "       'qty' AS f_nnqoh_xtnumericrole,"
               "       0 AS qoh_xttotalrole,"
               "       0 AS f_nnqoh_xttotalrole,"
               "       'cost' AS cost_xtnumericrole,"
               "       'curr' AS value_xtnumericrole,"
               "       'curr' AS f_nnvalue_xtnumericrole,"
               "       0 AS value_xttotalrole,"
               "       0 AS f_nnvalue_xttotalrole,"
               "       :na AS f_nnqoh_xtnullrole,"
               "       :na AS f_nnvalue_xtnullrole,"
               "       CASE WHEN (qoh < 0) THEN 'error' END AS qoh_qtforegroundrole,"
               "       CASE WHEN (reorderlevel > qoh) THEN 'warning' END AS qoh_qtforegroundrole "
               "FROM ( SELECT itemsite_id, itemsite_loccntrl, itemsite_costmethod,"
               "              ( (itemsite_loccntrl) OR (itemsite_controlmethod IN ('L', 'S')) ) AS detail,"
               "              warehous_code, classcode_code, item_number, uom_name, item_descrip1, item_descrip2,"
               "              CASE WHEN (NOT useDefaultLocation(itemsite_id)) THEN :none"
               "                   ELSE defaultLocationName(itemsite_id)"
               "              END AS defaultlocation,"
               "              CASE WHEN (itemsite_useparams) THEN itemsite_reorderlevel ELSE 0.0 END AS reorderlevel,"
               "              itemsite_qtyonhand AS qoh,"
               "              itemsite_nnqoh AS nnqoh," );

  if (_useStandardCosts->isChecked())
    sql += " stdcost(item_id) AS cost ";
  else if (_useActualCosts->isChecked())
    sql += " actcost(item_id) AS cost ";
  else
    sql += " (itemsite_value / CASE WHEN(itemsite_qtyonhand=0) THEN 1 ELSE itemsite_qtyonhand END) AS cost ";

  sql += "FROM itemsite, item, classcode, warehous, uom "
         "WHERE ( (itemsite_item_id=item_id)"
         " AND (item_inv_uom_id=uom_id)"
         " AND (item_classcode_id=classcode_id)"
         " AND (itemsite_warehous_id=warehous_id)"
         " AND (itemsite_active)";

  if (_warehouse->isSelected())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  if (_parameter->isSelected())
  {
    if (_parameter->type() == ParameterGroup::ClassCode)
      sql += " AND (classcode_id=:classcode_id)";
    else if (_parameter->type() == ParameterGroup::ItemGroup)
      sql += " AND (item_id IN (SELECT itemgrpitem_item_id FROM itemgrpitem WHERE (itemgrpitem_itemgrp_id=:itemgrp_id)))";
  }
  else if (_parameter->isPattern())
  {
    if (_parameter->type() == ParameterGroup::ClassCode)
      sql += " AND (classcode_id IN (SELECT classcode_id FROM classcode WHERE classcode_code ~ :classcode_pattern))";
    else if (_parameter->type() == ParameterGroup::ItemGroup)
      sql += " AND (item_id IN (SELECT itemgrpitem_item_id FROM itemgrpitem, itemgrp WHERE ( (itemgrpitem_itemgrp_id=itemgrp_id) AND (itemgrp_name ~ :itemgrp_pattern) ) ))";
  }
  else if(_parameter->type() == ParameterGroup::ItemGroup)
    sql += " AND (item_id IN (SELECT DISTINCT itemgrpitem_item_id FROM itemgrpitem))";

  if (_showPositive->isChecked())
    sql += " AND (itemsite_qtyonhand>0)";
  else if (_showNegative->isChecked())
    sql += " AND (itemsite_qtyonhand<0)";

  sql += ") ) AS data "
         "ORDER BY warehous_code";

  sql += ", item_number;";

  q.prepare(sql);
  q.bindValue(":none", tr("None"));
  q.bindValue(":na", tr("N/A"));
  _warehouse->bindValue(q);
  _parameter->bindValue(q);
  q.exec();
  if (q.first())
  {
    _qoh->populate(q, true);
  }
}
