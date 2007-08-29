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

#include "dspBreederDistributionVarianceByItem.h"

#include <QVariant>
#include <QStatusBar>
#include "rptBreederDistributionVarianceByItem.h"

/*
 *  Constructs a dspBreederDistributionVarianceByItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspBreederDistributionVarianceByItem::dspBreederDistributionVarianceByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_brdvar, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
    connect(_item, SIGNAL(newId(int)), _warehouse, SLOT(findItemSites(int)));
    connect(_item, SIGNAL(warehouseIdChanged(int)), _warehouse, SLOT(setId(int)));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspBreederDistributionVarianceByItem::~dspBreederDistributionVarianceByItem()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspBreederDistributionVarianceByItem::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <QMenu>

void dspBreederDistributionVarianceByItem::init()
{
  statusBar()->hide();

  _item->setType(ItemLineEdit::cBreeder);
  _dates->setStartNull(tr("Earliest"),omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"),omfgThis->endOfTime(), TRUE);

  _brdvar->addColumn(tr("Post Date"),      _dateColumn,  Qt::AlignCenter );
  _brdvar->addColumn(tr("Component Item"), -1,           Qt::AlignLeft   );
  _brdvar->addColumn(tr("Std. Qty. per"),  _qtyColumn,   Qt::AlignRight  );
  _brdvar->addColumn(tr("Std. Qty."),      _qtyColumn,   Qt::AlignRight  );
  _brdvar->addColumn(tr("Act. Qty. per"),  _qtyColumn,   Qt::AlignRight  );
  _brdvar->addColumn(tr("Act. Qty."),      _qtyColumn,   Qt::AlignRight  );
  _brdvar->addColumn(tr("Qty per Var."),   _qtyColumn,   Qt::AlignRight  );
  _brdvar->addColumn(tr("% Var."),         _prcntColumn, Qt::AlignRight  );
}

void dspBreederDistributionVarianceByItem::sPrint()
{
  ParameterList params;
  _warehouse->appendValue(params);
  _dates->appendValue(params);
  params.append("item_id", _item->id());
  params.append("print");

  rptBreederDistributionVarianceByItem newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspBreederDistributionVarianceByItem::sPopulateMenu(QMenu *)
{
}

void dspBreederDistributionVarianceByItem::sFillList()
{
  QString sql( "SELECT brdvar_id, formatDate(brdvar_postdate), item_number,"
               "       formatQtyPer(brdvar_stdqtyper), formatQty(brdvar_stdqtyper * brdvar_wo_qty),"
               "       formatQtyPer(brdvar_actqtyper), formatQty(brdvar_actqtyper * brdvar_wo_qty),"
               "       formatQtyPer(brdvar_actqtyper - brdvar_stdqtyper),"
               "       formatPrcnt((brdvar_actqtyper - brdvar_stdqtyper) / brdvar_stdqtyper) "
               "FROM brdvar, itemsite AS ps, itemsite AS cs, item "
               "WHERE ( (brdvar_parent_itemsite_id=ps.itemsite_id)"
               " AND (brdvar_itemsite_id=cs.itemsite_id)"
               " AND (cs.itemsite_item_id=item_id)"
               " AND (ps.itemsite_item_id=:item_id)"
               " AND (brdvar_postdate BETWEEN :startDate AND :endDate)" );

  if (_warehouse->isSelected())
    sql += " AND (ps.itemsite_warehous_id=:warehous_id)";

  sql += ") "
         "ORDER BY brdvar_postdate, item_number;";

  q.prepare(sql);
  _warehouse->bindValue(q);
  _dates->bindValue(q);
  q.bindValue(":item_id", _item->id());
  q.exec();
  _brdvar->populate(q);
}

