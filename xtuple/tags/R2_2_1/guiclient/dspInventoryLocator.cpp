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

#include "dspInventoryLocator.h"

#include <qvariant.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include "relocateInventory.h"
#include "reassignLotSerial.h"
#include "rptInventoryLocator.h"

/*
 *  Constructs a dspInventoryLocator as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspInventoryLocator::dspInventoryLocator(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_itemloc, SIGNAL(populateMenu(Q3PopupMenu*,Q3ListViewItem*,int)), this, SLOT(sPopulateMenu(Q3PopupMenu*,Q3ListViewItem*)));
    connect(_item, SIGNAL(warehouseIdChanged(int)), _warehouse, SLOT(setId(int)));
    connect(_item, SIGNAL(newId(int)), _warehouse, SLOT(findItemSites(int)));
    connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
    connect(_item, SIGNAL(valid(bool)), _query, SLOT(setEnabled(bool)));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspInventoryLocator::~dspInventoryLocator()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspInventoryLocator::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <Q3PopupMenu>

void dspInventoryLocator::init()
{
  statusBar()->hide();

  _itemloc->addColumn(tr("Whs."),         _whsColumn,   Qt::AlignCenter );
  _itemloc->addColumn(tr("Location"),     200,          Qt::AlignLeft   );
  _itemloc->addColumn(tr("Netable"),      _orderColumn, Qt::AlignCenter );
  _itemloc->addColumn(tr("Lot/Serial #"), -1,           Qt::AlignLeft   );
  _itemloc->addColumn(tr("Expiration"),   _dateColumn,  Qt::AlignCenter );
  _itemloc->addColumn(tr("Qty."),         _qtyColumn,   Qt::AlignRight  );

  _item->setFocus();
}

enum SetResponse dspInventoryLocator::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
    _item->setId(param.toInt());

  param = pParams.value("warehous_id", &valid);
  if (valid)
    _warehouse->setId(param.toInt());

  param = pParams.value("itemsite_id", &valid);
  if (valid)
    _item->setItemsiteid(param.toInt());

  return NoError;
}

void dspInventoryLocator::sPrint()
{
  ParameterList params;

  params.append("item_id", _item->id());
  _warehouse->appendValue(params);
  params.append("print");

  rptInventoryLocator newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspInventoryLocator::sRelocateInventory()
{
  ParameterList params;
  params.append("itemloc_id", _itemloc->id());

  relocateInventory newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec())
    sFillList();
}

void dspInventoryLocator::sReassignLotSerial()
{
  ParameterList params;
  params.append("itemloc_id", _itemloc->id());

  reassignLotSerial newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != QDialog::Rejected)
    sFillList();
}

void dspInventoryLocator::sPopulateMenu(Q3PopupMenu *pMenu, Q3ListViewItem *pSelected)
{
  int menuItem;

  if (((XListViewItem *)pSelected)->altId() == -1)
  {
    menuItem = pMenu->insertItem(tr("Relocate..."), this, SLOT(sRelocateInventory()), 0);
    if (!_privleges->check("RelocateInventory"))
      pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem(tr("Reassign Lot/Serial #..."), this, SLOT(sReassignLotSerial()), 0);
    if (!_privleges->check("ReassignLotSerial"))
      pMenu->setItemEnabled(menuItem, FALSE);
  }
}

void dspInventoryLocator::sFillList()
{
  if (_item->isValid())
  {
    QString sql( "SELECT itemloc_id, 1 AS type, warehous_code,"
                 "       CASE WHEN (location_id IS NULL) THEN :na"
                 "            ELSE (formatLocationName(location_id) || '-' || firstLine(location_descrip))"
                 "       END AS locationname,"
                 "       CASE WHEN (location_id IS NULL) THEN :na"
                 "            WHEN (location_netable) THEN :yes"
                 "            ELSE :no"
                 "       END AS netable,"
                 "       CASE WHEN (itemsite_controlmethod NOT IN ('L', 'S')) THEN :na"
                 "            ELSE itemloc_lotserial"
                 "       END AS lotserial,"
                 "       CASE WHEN (itemsite_perishable) THEN formatDate(itemloc_expiration)"
                 "            ELSE :na"
                 "       END AS f_expiration,"
                 "       CASE WHEN (itemsite_perishable) THEN (itemloc_expiration <= CURRENT_DATE)"
                 "            ELSE FALSE"
                 "       END AS expired,"
                 "       formatQty(itemloc_qty) AS f_qoh "
                 "FROM itemsite, warehous,"
                 "     itemloc LEFT OUTER JOIN location ON (itemloc_location_id=location_id) "
                 "WHERE ( ( (itemsite_loccntrl) OR (itemsite_controlmethod IN ('L', 'S')) )"
                 " AND (itemloc_itemsite_id=itemsite_id)"
                 " AND (itemsite_warehous_id=warehous_id)"
                 " AND (itemsite_item_id=:item_id)" );

    if (_warehouse->isSelected())
      sql += " AND (itemsite_warehous_id=:warehous_id)";

    sql += ") "
           "UNION SELECT itemsite_id, 2 AS type, warehous_code,"
           "             :na AS locationname,"
           "             :na AS netable,"
           "             :na AS lotserial,"
           "             :na AS f_expiration,"
           "             FALSE  AS expired,"
           "             formatQty(itemsite_qtyonhand) AS f_qoh "
           "FROM itemsite, warehous "
           "WHERE ( (NOT itemsite_loccntrl)"
           " AND (itemsite_controlmethod NOT IN ('L', 'S'))"
           " AND (itemsite_warehous_id=warehous_id)"
           " AND (itemsite_item_id=:item_id)";

    if (_warehouse->isSelected())
      sql += " AND (itemsite_warehous_id=:warehous_id)";

    sql += ") "
           "ORDER BY warehous_code, locationname, lotserial;";

    q.prepare(sql);
    q.bindValue(":yes", tr("Yes"));
    q.bindValue(":no", tr("No"));
    q.bindValue(":na", tr("N/A"));
    q.bindValue(":undefined", tr("Undefined"));
    q.bindValue(":item_id", _item->id());
    _warehouse->bindValue(q);
    q.exec();

    _itemloc->clear();
    while (q.next())
    {
      XListViewItem *last = new XListViewItem( _itemloc, _itemloc->lastItem(),
                                               q.value("itemloc_id").toInt(), q.value("type").toInt(),
                                               q.value("warehous_code"), q.value("locationname"),
                                               q.value("netable"), q.value("lotserial"),
                                               q.value("f_expiration"), q.value("f_qoh") );
      if (q.value("expired").toBool())
        last->setColor("red");
    }
  }
  else
    _itemloc->clear();
}

