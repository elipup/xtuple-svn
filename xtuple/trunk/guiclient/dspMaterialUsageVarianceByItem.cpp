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

#include "dspMaterialUsageVarianceByItem.h"

#include <QVariant>
//#include <QStatusBar>
#include <QMenu>
#include <openreports.h>

/*
 *  Constructs a dspMaterialUsageVarianceByItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspMaterialUsageVarianceByItem::dspMaterialUsageVarianceByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_womatlvar, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_item, SIGNAL(newId(int)), _warehouse, SLOT(findItemSites(int)));
  connect(_item, SIGNAL(warehouseIdChanged(int)), _warehouse, SLOT(setId(int)));

  _item->setType(ItemLineEdit::cGeneralManufactured | ItemLineEdit::cGeneralPurchased | ItemLineEdit::cJob);
  _item->setDefaultType(ItemLineEdit::cGeneralManufactured | ItemLineEdit::cJob);
  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), 0);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), 0);

  _womatlvar->addColumn(tr("Post Date"),      _dateColumn,  Qt::AlignCenter, true,  "posted" );
  _womatlvar->addColumn(tr("Component Item"), _itemColumn,  Qt::AlignLeft,   true,  "item_number"   );
  _womatlvar->addColumn(tr("Ordered"),        _qtyColumn,   Qt::AlignRight,  true,  "ordered"  );
  _womatlvar->addColumn(tr("Produced"),       _qtyColumn,   Qt::AlignRight,  true,  "received"  );
  _womatlvar->addColumn(tr("Proj. Req."),     _qtyColumn,   Qt::AlignRight,  true,  "projreq"  );
  _womatlvar->addColumn(tr("Proj. Qty. per"), _qtyColumn,   Qt::AlignRight,  true,  "projqtyper"  );
  _womatlvar->addColumn(tr("Act. Iss."),      _qtyColumn,   Qt::AlignRight,  true,  "actiss"  );
  _womatlvar->addColumn(tr("Act. Qty. per"),  _qtyColumn,   Qt::AlignRight,  true,  "actqtyper"  );
  _womatlvar->addColumn(tr("Qty. per Var."),  _qtyColumn,   Qt::AlignRight,  true,  "qtypervar"  );
  _womatlvar->addColumn(tr("%"),              _prcntColumn, Qt::AlignRight,  true,  "qtypervarpercent"  );
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspMaterialUsageVarianceByItem::~dspMaterialUsageVarianceByItem()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspMaterialUsageVarianceByItem::languageChange()
{
  retranslateUi(this);
}

void dspMaterialUsageVarianceByItem::sPrint()
{
  ParameterList params;
  _warehouse->appendValue(params);
  _dates->appendValue(params);
  params.append("item_id", _item->id());

  orReport report("MaterialUsageVarianceByItem", params);
  if(report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspMaterialUsageVarianceByItem::sPopulateMenu(QMenu *)
{
}

void dspMaterialUsageVarianceByItem::sFillList()
{
  if ((_item->isValid()) && (_dates->allValid()))
  {
    QString sql( "SELECT womatlvar_id, posted, item_number,"
                 "       ordered, received,"
                 "       projreq, projqtyper,"
                 "       actiss, actqtyper,"
                 "       (actqtyper - projqtyper) AS qtypervar,"
                 "       ((1 - (actqtyper / projqtyper)) * -1) AS qtypervarpercent,"
                 "       'qty' AS ordered_xtnumericrole,"
                 "       'qty' AS received_xtnumericrole,"
                 "       'qty' AS projreq_xtnumericrole,"
                 "       'qtyper' AS projqtyper_xtnumericrole,"
                 "       'qty' AS actiss_xtnumericrole,"
                 "       'qtyper' AS actqtyper_xtnumericrole,"
                 "       'qtyper' AS qtypervar_xtnumericrole,"
                 "       'percent' AS qtypervarpercent_xtnumericrole "
                 "FROM ( SELECT womatlvar_id, womatlvar_posted AS posted, item_number,"
                 "              womatlvar_qtyord AS ordered, womatlvar_qtyrcv AS received,"
                 "              (womatlvar_qtyrcv * (womatlvar_qtyper * (1 + womatlvar_scrap))) AS projreq,"
                 "              womatlvar_qtyper AS projqtyper,"
                 "              (womatlvar_qtyiss) AS actiss, (womatlvar_qtyiss / (womatlvar_qtyrcv * (1 + womatlvar_scrap))) AS actqtyper "
                 "       FROM womatlvar, itemsite AS component, itemsite AS parent, item "
                 "       WHERE ( (womatlvar_parent_itemsite_id=parent.itemsite_id)"
                 "        AND (womatlvar_component_itemsite_id=component.itemsite_id)"
                 "        AND (component.itemsite_item_id=item_id)"
                 "        AND (parent.itemsite_item_id=:item_id)"
                 "        AND (womatlvar_posted BETWEEN :startDate AND :endDate)" );

    if (_warehouse->isSelected())
      sql += " AND (parent.itemsite_warehous_id=:warehous_id)";

    sql += ") ) AS data "
           "ORDER BY posted";

    q.prepare(sql);
    _warehouse->bindValue(q);
    _dates->bindValue(q);
    q.bindValue(":item_id", _item->id());
    q.exec();
    _womatlvar->populate(q);
  }
}

