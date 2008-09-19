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

#include "dspCostedSummarizedBOM.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include <openreports.h>
#include <parameter.h>

dspCostedSummarizedBOM::dspCostedSummarizedBOM(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  QButtonGroup* _costsGroupInt = new QButtonGroup(this);
  _costsGroupInt->addButton(_useStandardCosts);
  _costsGroupInt->addButton(_useActualCosts);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));

  _item->setType(ItemLineEdit::cGeneralManufactured);

  _bomitem->setRootIsDecorated(TRUE);
  _bomitem->addColumn(tr("Item Number"),  _itemColumn, Qt::AlignLeft,  true, "bomdata_item_number");
  _bomitem->addColumn(tr("Description"),           -1, Qt::AlignLeft,  true, "bomdata_itemdescription");
  _bomitem->addColumn(tr("UOM"),           _uomColumn, Qt::AlignCenter,true, "bomdata_uom_name");
  _bomitem->addColumn(tr("Ext. Qty. Per"),_qtyColumn, Qt::AlignRight, true, "bomdata_qtyper");
  _bomitem->addColumn(tr("Unit Cost"),    _costColumn, Qt::AlignRight, true, "unitcost");
  _bomitem->addColumn(tr("Ext. Cost"),    _priceColumn, Qt::AlignRight, true, "extendedcost");
  _bomitem->setIndentation(10);

  _expiredDaysLit->setEnabled(_showExpired->isChecked());
  _expiredDays->setEnabled(_showExpired->isChecked());
  _effectiveDaysLit->setEnabled(_showFuture->isChecked());
  _effectiveDays->setEnabled(_showFuture->isChecked());

  connect(omfgThis, SIGNAL(bomsUpdated(int, bool)), this, SLOT(sFillList(int, bool)));

  _revision->setMode(RevisionLineEdit::View);
  _revision->setType("BOM");

  _revision->setVisible(_metrics->boolean("RevControl"));
}

dspCostedSummarizedBOM::~dspCostedSummarizedBOM()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspCostedSummarizedBOM::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspCostedSummarizedBOM::set(const ParameterList &pParams)
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
    return NoError_Print;
  }

  return NoError;
}

bool dspCostedSummarizedBOM::setParams(ParameterList &params)
{
  params.append("item_id", _item->id());
  params.append("revision_id", _revision->id());

  if(_showExpired->isChecked())
    params.append("expiredDays", _expiredDays->value());
  else
    params.append("expiredDays", 0);

  if(_showFuture->isChecked())
    params.append("futureDays", _effectiveDays->value());
  else
    params.append("futureDays", 0);

  if (_useStandardCosts->isChecked())
    params.append("useStandardCosts");

  if (_useActualCosts->isChecked())
    params.append("useActualCosts");

  return true;
}

void dspCostedSummarizedBOM::sPrint()
{
  ParameterList params;
  if (! setParams(params))
    return;

  orReport report("CostedSummarizedBOM", params);

  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspCostedSummarizedBOM::sFillList()
{
  if (! _item->isValid())
    return;

  MetaSQLQuery mql("SELECT -1, *,"
                   "<? if exists(\"useActualCosts\") ?>"
                   "       bomdata_actunitcost AS unitcost,"
                   "       bomdata_actextendedcost AS extendedcost,"
                   "<? elseif exists(\"useStandardCosts\") ?>"
                   "       bomdata_stdunitcost AS unitcost,"
                   "       bomdata_stdextendedcost AS extendedcost,"
                   "<? endif ?>"
                   "       'qtyper' AS bomdata_qtyper_xtnumericrole,"
                   "       'cost' AS unitcost_xtnumericrole,"
                   "       'cost' AS extendedcost_xtnumericrole,"
                   "       CASE WHEN COALESCE(bomdata_effective, startOfTime()) <= startOfTime() THEN <? value(\"always\") ?> END AS bomdata_effective_qtdisplayrole,"
                   "       CASE WHEN COALESCE(bomdata_expires, endOfTime()) <= endOfTime() THEN <? value(\"never\") ?> END AS bomdata_expires_qtdisplayrole,"
                   "       CASE WHEN bomdata_expired THEN 'expired'"
                   "            WHEN bomdata_future  THEN 'future'"
                   "       END AS qtforegroundrole "
                   "FROM summarizedBOM(<? value(\"item_id\") ?>,"
                   "                   <? value(\"revision_id\") ?>,"
                   "                   <? value(\"expiredDays\") ?>,"
                   "                   <? value(\"futureDays\") ?>);");

  ParameterList params;
  if (! setParams(params))
    return;
  q = mql.toQuery(params);
  _bomitem->populate(q);
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  for (int i = 0; i < _bomitem->topLevelItemCount(); i++)
    _bomitem->collapseItem(_bomitem->topLevelItem(i));
}

void dspCostedSummarizedBOM::sFillList(int pid, bool)
{
  if (pid == _item->id())
    sFillList();
}
