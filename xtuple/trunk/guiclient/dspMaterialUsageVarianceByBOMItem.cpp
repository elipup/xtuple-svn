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

#include "dspMaterialUsageVarianceByBOMItem.h"

#include <QVariant>
#include <QStatusBar>
#include <QMenu>
#include <QMessageBox>
#include <openreports.h>
#include <parameter.h>

/*
 *  Constructs a dspMaterialUsageVarianceByBOMItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspMaterialUsageVarianceByBOMItem::dspMaterialUsageVarianceByBOMItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_womatlvar, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_componentItem, SIGNAL(newID(int)), this, SLOT(sFillList()));
  connect(_item, SIGNAL(newId(int)), this, SLOT(sPopulateComponentItems(int)));

  _item->setType(ItemLineEdit::cGeneralManufactured);
  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _womatlvar->addColumn(tr("Post Date"),      _dateColumn,  Qt::AlignCenter );
  _womatlvar->addColumn(tr("Ordered"),        _qtyColumn,   Qt::AlignRight  );
  _womatlvar->addColumn(tr("Produced"),       _qtyColumn,   Qt::AlignRight  );
  _womatlvar->addColumn(tr("Proj. Req."),     _qtyColumn,   Qt::AlignRight  );
  _womatlvar->addColumn(tr("Proj. Qty. per"), _qtyColumn,   Qt::AlignRight  );
  _womatlvar->addColumn(tr("Act. Iss."),      _qtyColumn,   Qt::AlignRight  );
  _womatlvar->addColumn(tr("Act. Qty. per"),  _qtyColumn,   Qt::AlignRight  );
  _womatlvar->addColumn(tr("Qty. per Var."),  _qtyColumn,   Qt::AlignRight  );
  _womatlvar->addColumn(tr("%"),              _prcntColumn, Qt::AlignRight  );
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspMaterialUsageVarianceByBOMItem::~dspMaterialUsageVarianceByBOMItem()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspMaterialUsageVarianceByBOMItem::languageChange()
{
  retranslateUi(this);
}

void dspMaterialUsageVarianceByBOMItem::sPrint()
{
  if(!_dates->allValid())
  {
    QMessageBox::warning(this, tr("Invalid Date Range"),
      tr("You must specify a valid date range.") );
    return;
  }

  ParameterList params;
  _warehouse->appendValue(params);
  _dates->appendValue(params);
  params.append("item_id", _item->id());
  params.append("component_item_id", _componentItem->id());

  orReport report("MaterialUsageVarianceByBOMItem", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspMaterialUsageVarianceByBOMItem::sPopulateMenu(QMenu *)
{
}

void dspMaterialUsageVarianceByBOMItem::sPopulateComponentItems(int pItemid)
{
  if (pItemid != -1)
  {
    q.prepare( "SELECT bomitem_id,"
               "       (bomitem_seqnumber || '-' || item_number || ' ' || item_descrip1 || ' ' || item_descrip2) "
               "FROM bomitem, item "
               "WHERE ( (bomitem_item_id=item_id)"
               " AND (bomitem_parent_item_id=:item_id) ) "
               "ORDER BY bomitem_seqnumber;" );
    q.bindValue(":item_id", pItemid);
    q.exec();
    _componentItem->populate(q);
  }
  else
    _componentItem->clear();
}

void dspMaterialUsageVarianceByBOMItem::sFillList()
{
  if ((_componentItem->id() != -1) && (_dates->allValid()))
  {
    QString sql( "SELECT womatlvar_id, formatDate(posted),"
                 "       formatQty(ordered), formatQty(received),"
                 "       formatQty(projreq), formatQtyPer(projqtyper),"
                 "       formatQty(actiss), formatQtyPer(actqtyper),"
                 "       formatQtyPer(actqtyper - projqtyper),"
                 "       formatPrcnt((1 - (actqtyper / projqtyper)) * -1) "
                 "FROM ( SELECT womatlvar_id, womatlvar_posted AS posted,"
                 "              womatlvar_qtyord AS ordered, womatlvar_qtyrcv AS received,"
                 "              (womatlvar_qtyrcv * (womatlvar_qtyper * (1 + womatlvar_scrap))) AS projreq,"
                 "              womatlvar_qtyper AS projqtyper,"
                 "              (womatlvar_qtyiss) AS actiss, (womatlvar_qtyiss / (womatlvar_qtyrcv * (1 + womatlvar_scrap))) AS actqtyper "
                 "       FROM womatlvar, itemsite AS component, itemsite AS parent "
                 "       WHERE ((womatlvar_parent_itemsite_id=parent.itemsite_id)"
                 "        AND (womatlvar_component_itemsite_id=component.itemsite_id)"
                 "        AND (womatlvar_bomitem_id=:bomitem_id)"
                 "        AND (womatlvar_posted BETWEEN :startDate AND :endDate)" );

    if (_warehouse->isSelected())
      sql += " AND (component.itemsite_warehous_id=:warehous_id)";

    sql += ") ) AS data "
           "ORDER BY posted";

    q.prepare(sql);
    _warehouse->bindValue(q);
    _dates->bindValue(q);
    q.bindValue(":bomitem_id", _componentItem->id());
    q.exec();
    _womatlvar->populate(q);
  }
}

