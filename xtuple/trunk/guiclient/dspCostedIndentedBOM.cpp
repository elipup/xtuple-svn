/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspCostedIndentedBOM.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include <openreports.h>

#include "dspItemCostSummary.h"
#include "maintainItemCosts.h"

dspCostedIndentedBOM::dspCostedIndentedBOM(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  _revision->setType("BOM");
  _revision->setMode(RevisionLineEdit::View);

  QButtonGroup* _costsGroupInt = new QButtonGroup(this);
  _costsGroupInt->addButton(_useStandardCosts);
  _costsGroupInt->addButton(_useActualCosts);

  connect(_bomitem, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));
  connect(_costsGroupInt, SIGNAL(buttonClicked(int)), this, SLOT(sFillList()));
  connect(_item,                  SIGNAL(newId(int)), this, SLOT(sFillList()));
  connect(omfgThis,   SIGNAL(bomsUpdated(int, bool)), this, SLOT(sFillList(int, bool)));
  connect(_print,                  SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_revision,              SIGNAL(newId(int)), this, SLOT(sFillList()));

  _item->setType(ItemLineEdit::cGeneralManufactured);

  _bomitem->setRootIsDecorated(TRUE);
  _bomitem->addColumn(tr("Seq #"),       _itemColumn, Qt::AlignLeft,  true, "bomdata_bomwork_seqnumber");
  _bomitem->addColumn(tr("Item Number"), _itemColumn, Qt::AlignLeft,  true, "bomdata_item_number");
  _bomitem->addColumn(tr("Description"),          -1, Qt::AlignLeft,  true, "bomdata_itemdescription");
  _bomitem->addColumn(tr("UOM"),          _uomColumn, Qt::AlignCenter,true, "bomdata_uom_name");
  _bomitem->addColumn(tr("Ext. Qty. Per"),_qtyColumn, Qt::AlignRight, true, "bomdata_qtyper");
  _bomitem->addColumn(tr("Scrap %"),    _prcntColumn, Qt::AlignRight, true, "bomdata_scrap");
  _bomitem->addColumn(tr("Effective"),   _dateColumn, Qt::AlignCenter,true, "bomdata_effective");
  _bomitem->addColumn(tr("Expires"),     _dateColumn, Qt::AlignCenter,true, "bomdata_expires");
  _bomitem->addColumn(tr("Unit Cost"),   _costColumn, Qt::AlignRight, true, "unitcost");
  _bomitem->addColumn(tr("Ext. Cost"), _priceColumn, Qt::AlignRight, true,  "extendedcost");

  _bomitem->setIndentation(10);

  //If not Revision Control, hide control
  _revision->setVisible(_metrics->boolean("RevControl"));
}

dspCostedIndentedBOM::~dspCostedIndentedBOM()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspCostedIndentedBOM::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspCostedIndentedBOM::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
  {
    _item->setId(param.toInt());
    param = pParams.value("revision_id", &valid);
    if (valid)
      _revision->setId(param.toInt());
  }

  if (pParams.inList("run"))
  {
    sFillList();
    return NoError_Run;
  }

  return NoError;
}

bool dspCostedIndentedBOM::setParams(ParameterList &params)
{
  if (! _item->isValid())
  {
    QMessageBox::critical(this, tr("Must Supply Item"),
                          tr("<p>You must supply an Item to see its BOM."));
    _item->setFocus();
    return false;
  }

  params.append("item_id", _item->id());
  params.append("revision_id", _revision->id());

  if(_useStandardCosts->isChecked())
    params.append("useStandardCosts");

  if(_useActualCosts->isChecked())
    params.append("useActualCosts");

  params.append("always", tr("Always"));
  params.append("never",  tr("Never"));

  return true;
}

void dspCostedIndentedBOM::sPrint()
{
  ParameterList params;
  if (! setParams(params))
    return;

  orReport report("CostedIndentedBOM", params);

  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspCostedIndentedBOM::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pSelected)
{
  if (((XTreeWidgetItem *)pSelected)->id() != -1)
    pMenu->insertItem(tr("Maintain Item Costs..."), this, SLOT(sMaintainItemCosts()), 0);

  if (((XTreeWidgetItem *)pSelected)->id() != -1)
    pMenu->insertItem(tr("View Item Costing..."), this, SLOT(sViewItemCosting()), 0);
}

void dspCostedIndentedBOM::sMaintainItemCosts()
{
  ParameterList params;
  params.append("item_id", _bomitem->altId());

  maintainItemCosts *newdlg  = new maintainItemCosts();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspCostedIndentedBOM::sViewItemCosting()
{
  ParameterList params;
  params.append( "item_id", _bomitem->altId() );
  params.append( "run",     TRUE              );

  dspItemCostSummary *newdlg = new dspItemCostSummary();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspCostedIndentedBOM::sFillList()
{
  if (! _item->isValid())
    return;

  MetaSQLQuery mql( "SELECT bomdata_bomwork_id AS id,"
                    "       CASE WHEN bomdata_bomwork_parent_id = -1 AND "
                    "                 bomdata_bomwork_id = -1 THEN"
                    "                     -1"
                    "            ELSE bomdata_item_id"
                    "       END AS altid,"
                    "       *,"
                    "<? if exists(\"useStandardCosts\") ?>"
                    "       bomdata_stdunitcost AS unitcost,"
                    "       bomdata_stdextendedcost AS extendedcost, "
                    "<? elseif exists(\"useActualCosts\") ?>"
                    "       bomdata_actunitcost AS unitcost,"
                    "       bomdata_actextendedcost AS extendedcost, "
                    "<? endif ?>"
                    "       'qtyper' AS bomdata_qtyper_xtnumericrole,"
                    "       'percent' AS bomdata_scrap_xtnumericrole,"
                    "       'cost' AS unitcost_xtnumericrole,"
                    "       'cost' AS extendedcost_xtnumericrole,"
                    "       CASE WHEN COALESCE(bomdata_effective, startOfTime()) <= startOfTime() THEN <? value(\"always\") ?> END AS bomdata_effective_qtdisplayrole,"
                    "       CASE WHEN COALESCE(bomdata_expires, endOfTime()) <= endOfTime() THEN <? value(\"never\") ?> END AS bomdata_expires_qtdisplayrole,"
                    "       CASE WHEN bomdata_expired THEN 'expired'"
                    "            WHEN bomdata_future  THEN 'future'"
                    "       END AS qtforegroundrole,"
                    "       bomdata_bomwork_level - 1 AS xtindentrole "
                    "FROM indentedbom(<? value(\"item_id\") ?>,"
                    "                 <? value(\"revision_id\") ?>,0,0)");
  ParameterList params;
  if (! setParams(params))
    return;
  q = mql.toQuery(params);
  _bomitem->populate(q, true);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  q.prepare( "SELECT formatCost(SUM(bomdata_actextendedcost)) AS actextendedcost,"
             "       formatCost(SUM(bomdata_stdextendedcost)) AS stdextendedcost,"
             "       formatCost(actcost(:item_id)) AS actual,"
             "       formatCost(stdcost(:item_id)) AS standard "
             "FROM indentedbom(:item_id,"
             "                 :revision_id,0,0)"
             "WHERE (bomdata_bomwork_level=1) "
             "GROUP BY actual, standard;" );
  q.bindValue(":item_id", _item->id());
  q.bindValue(":revision_id",_revision->id());
  q.exec();
  if (q.first())
  {
    XTreeWidgetItem *last = new XTreeWidgetItem(_bomitem, -1, -1);
    last->setText(0, tr("Total Cost"));
    if(_useStandardCosts->isChecked())
      last->setText(9, q.value("stdextendedcost").toString());
    else
      last->setText(9, q.value("actextendedcost").toString());

    last = new XTreeWidgetItem( _bomitem, -1, -1);
    last->setText(0, tr("Actual Cost"));
    last->setText(9, q.value("actual").toString());

    last = new XTreeWidgetItem( _bomitem, -1, -1);
    last->setText(0, tr("Standard Cost"));
    last->setText(9, q.value("standard").toString());
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  _bomitem->expandAll();
}

void dspCostedIndentedBOM::sFillList(int p, bool)
{
  if (p == _item->id())
    sFillList();
}
