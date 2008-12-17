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

#include "dspValidLocationsByItem.h"

#include <QMenu>
#include <QMessageBox>
#include <QVariant>

#include <openreports.h>
#include <parameter.h>

dspValidLocationsByItem::dspValidLocationsByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_location, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));

  _location->addColumn(tr("Site"),        _whsColumn,   Qt::AlignCenter, true,  "warehous_code" );
  _location->addColumn(tr("Location"),    _itemColumn,  Qt::AlignLeft,   true,  "locationname"   );
  _location->addColumn(tr("Description"), -1,           Qt::AlignLeft,   true,  "locationdescrip"   );
  _location->addColumn(tr("Restricted"),  _orderColumn, Qt::AlignCenter, true,  "location_restrict"  );
  _location->addColumn(tr("Netable"),     _orderColumn, Qt::AlignCenter, true,  "location_netable" );

  _item->setFocus();
}

dspValidLocationsByItem::~dspValidLocationsByItem()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspValidLocationsByItem::languageChange()
{
  retranslateUi(this);
}

void dspValidLocationsByItem::sPrint()
{
  ParameterList params;
  params.append("item_id", _item->id());

  _warehouse->appendValue(params);

  orReport report("ValidLocationsByItem", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspValidLocationsByItem::sPopulateMenu(QMenu *)
{
}

void dspValidLocationsByItem::sFillList()
{
  QString sql = "SELECT location_id, warehous_code,"
                "       formatLocationName(location_id) AS locationname,"
                "       firstLine(location_descrip) AS locationdescrip,"
                "       location_restrict, location_netable "
                "FROM itemsite, location, warehous "
                "WHERE ( (validLocation(location_id, itemsite_id))"
                " AND ( (itemsite_loccntrl) OR (itemsite_location_id=location_id) )"
                " AND (itemsite_item_id=:item_id)"
                " AND (itemsite_warehous_id=warehous_id)";

  if (_warehouse->isSelected())
    sql += " AND (warehous_id=:warehous_id)";

  sql += ") "
         "ORDER BY warehous_code, locationname;";

  q.prepare(sql);
  q.bindValue(":item_id", _item->id());
  _warehouse->bindValue(q);
  q.exec();
  _location->populate(q);
}

