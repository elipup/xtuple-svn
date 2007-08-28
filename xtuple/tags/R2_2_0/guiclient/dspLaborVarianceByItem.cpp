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

#include "dspLaborVarianceByItem.h"

#include <qvariant.h>
#include <qstatusbar.h>
#include <parameter.h>
#include "rptLaborVarianceByItem.h"
#include "OpenMFGGUIClient.h"

/*
 *  Constructs a dspLaborVarianceByItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspLaborVarianceByItem::dspLaborVarianceByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_woopervar, SIGNAL(populateMenu(Q3PopupMenu*,Q3ListViewItem*,int)), this, SLOT(sPopulateMenu(Q3PopupMenu*)));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_item, SIGNAL(newId(int)), _warehouse, SLOT(findItemSites(int)));
    connect(_item, SIGNAL(warehouseIdChanged(int)), _warehouse, SLOT(setId(int)));
    connect(_item, SIGNAL(newId(int)), this, SLOT(sFillList()));
    connect(_warehouse, SIGNAL(updated()), this, SLOT(sFillList()));
    connect(_dates, SIGNAL(updated()), this, SLOT(sFillList()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspLaborVarianceByItem::~dspLaborVarianceByItem()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspLaborVarianceByItem::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <Q3PopupMenu>

void dspLaborVarianceByItem::init()
{
  statusBar()->hide();

  _item->setType(ItemLineEdit::cGeneralManufactured);
  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _woopervar->addColumn(tr("Post Date"),   _dateColumn,  Qt::AlignCenter );
  _woopervar->addColumn(tr("Seq #"),       _seqColumn,   Qt::AlignCenter );
  _woopervar->addColumn(tr("Work Center"), -1,           Qt::AlignLeft   );
  _woopervar->addColumn(tr("Proj Setup"),  _qtyColumn,   Qt::AlignRight  );
  _woopervar->addColumn(tr("Proj. Run"),   _qtyColumn,   Qt::AlignRight  );
  _woopervar->addColumn(tr("Act. Setup"),  _qtyColumn,   Qt::AlignRight  );
  _woopervar->addColumn(tr("Act. Run"),    _qtyColumn,   Qt::AlignRight  );
  _woopervar->addColumn(tr("Setup Var."),  _qtyColumn,   Qt::AlignRight  );
  _woopervar->addColumn(tr("Run Var."),    _qtyColumn,   Qt::AlignRight  );
}

void dspLaborVarianceByItem::sPrint()
{
  ParameterList params;
  _dates->appendValue(params);
  params.append("item_id", _item->id());
  params.append("print");

  if (_warehouse->isSelected())
    params.append("warehous_id", _warehouse->id());

  rptLaborVarianceByItem newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspLaborVarianceByItem::sPopulateMenu(Q3PopupMenu *)
{
}

void dspLaborVarianceByItem::sFillList()
{
  if (!checkParameters())
    return;

  QString sql( "SELECT woopervar_id, formatDate(woopervar_posted),"
               "       woopervar_seqnumber, wrkcnt_code,"
               "       formatTime(woopervar_stdsutime),"
               "       formatTime(woopervar_stdrntime),"
               "       formatTime(woopervar_sutime),"
               "       formatTime(woopervar_rntime),"
               "       formattime(woopervar_sutime - woopervar_stdsutime),"
               "       formattime(woopervar_rntime - woopervar_stdrntime) "
               "FROM woopervar, itemsite, wrkcnt "
               "WHERE ( (woopervar_parent_itemsite_id=itemsite_id)"
               " AND (woopervar_wrkcnt_id=wrkcnt_id)"
               " AND (itemsite_item_id=:item_id)"
               " AND (woopervar_posted BETWEEN :startDate AND :endDate)" );

  if (_warehouse->isSelected())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  sql += ") "
         "ORDER BY woopervar_posted DESC, woopervar_seqnumber;";

  q.prepare(sql);
  _dates->bindValue(q);
  _warehouse->bindValue(q);
  q.bindValue(":undefined", tr("Undefined"));
  q.bindValue(":item_id", _item->id());
  q.exec();
  _woopervar->populate(q);
}

bool dspLaborVarianceByItem::checkParameters()
{
  if (_item->id() == -1)
  {
    return FALSE;
  }

  if (!_dates->startDate().isValid())
  {
    return FALSE;
  }

  if (!_dates->endDate().isValid())
  {
    return FALSE;
  }

  return TRUE;
}

