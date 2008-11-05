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

#include "woList.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <parameter.h>

#include "xtreewidget.h"
#include "warehousegroup.h"
#include "wocluster.h"

woList::woList(QWidget * parent, const char * name, bool modal, Qt::WFlags fl) :
  QDialog( parent, name, modal, fl )
{
  setWindowTitle(tr("Work Orders"));

  _woid = -1;
  _type = 0;

  if ( !name )
    setObjectName( "woList" );

  QVBoxLayout *mainLayout = new QVBoxLayout( this, 5, 5, "woListLayout"); 
  QHBoxLayout *topLayout = new QHBoxLayout( 0, 0, 7, "topLayout"); 
  QVBoxLayout *warehouseLayout = new QVBoxLayout( 0, 0, 0, "warehouseLayout"); 
  QVBoxLayout *buttonsLayout = new QVBoxLayout( 0, 0, 5, "buttonsLayout"); 
  QVBoxLayout *listLayout = new QVBoxLayout( 0, 0, 0, "listLayout"); 

  _warehouse = new WarehouseGroup(this, "_warehouse");
  warehouseLayout->addWidget(_warehouse);

  QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Preferred);
  warehouseLayout->addItem(spacer);

  topLayout->addLayout(warehouseLayout);

  QSpacerItem* spacer_2 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
  topLayout->addItem(spacer_2);

  _close = new QPushButton(tr("&Cancel"), this, "_close");
  buttonsLayout->addWidget(_close);

  _select = new QPushButton(tr("&Select"), this, "_select");
  _select->setAutoDefault(TRUE);
  _select->setDefault(TRUE);
  buttonsLayout->addWidget(_select);
  topLayout->addLayout(buttonsLayout);
  mainLayout->addLayout(topLayout);

  QLabel *_workOrdersLit = new QLabel(tr("Work Orders:"), this, "_workOrdersLit");
  listLayout->addWidget(_workOrdersLit);

  _wo = new XTreeWidget(this);
  _wo->setObjectName("_wo");
  listLayout->addWidget( _wo );
  mainLayout->addLayout(listLayout);

  resize( QSize(484, 366).expandedTo(minimumSizeHint()) );

  connect( _close, SIGNAL( clicked() ), this, SLOT( sClose() ) );
  connect( _select, SIGNAL( clicked() ), this, SLOT( sSelect() ) );
  connect( _wo, SIGNAL( itemSelected(int) ), this, SLOT( sSelect() ) );
  connect( _warehouse, SIGNAL(updated()), this, SLOT( sFillList() ) );

  setTabOrder(_warehouse, _wo);
  setTabOrder(_wo, _select);
  setTabOrder(_select, _close);
  setTabOrder(_close, _warehouse);
  _wo->setFocus();

  _wo->addColumn(tr("W/O #"),     _orderColumn, Qt::AlignLeft,  true, "wonumber");
  _wo->addColumn(tr("Status"),              40, Qt::AlignCenter,true, "wo_status");
  _wo->addColumn(tr("Whs."),        _whsColumn, Qt::AlignCenter,true, "warehous_code");
  _wo->addColumn(tr("Item Number"),_itemColumn, Qt::AlignLeft,  true, "item_number");
  _wo->addColumn(tr("Description"),         -1, Qt::AlignLeft,  true, "descrip");
}

void woList::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("wo_id", &valid);
  if (valid)
    _woid = param.toInt();

  param = pParams.value("warehous_id", &valid);
  if (valid)
    _warehouse->setId(param.toInt());

  param = pParams.value("woType", &valid);
  if (valid)
    _type = param.toInt();

  param = pParams.value("sql", &valid);
  if (valid)
    _sql = param.toString();

  sFillList();
}

void woList::sClose()
{
  done(_woid);
}

void woList::sSelect()
{
  done(_wo->id());
}

void woList::sFillList()
{
  QString sql;

  if (_sql.length())
    sql = _sql;
  else
  {
    sql = "SELECT wo_id,"
          "       formatWONumber(wo_id) AS wonumber,"
          "       wo_status, warehous_code, item_number,"
          "       (item_descrip1 || ' ' || item_descrip2) AS descrip "
          "FROM wo, itemsite, warehous, item "
          "WHERE ( (wo_itemsite_id=itemsite_id)"
          " AND (itemsite_warehous_id=warehous_id)"
          " AND (itemsite_item_id=item_id)";

    if (_type != 0)
    {
      QStringList statuslist;

      if (_type & cWoOpen)
        statuslist << "'O'";

      if (_type & cWoExploded)
        statuslist << "'E'";

      if (_type & cWoReleased)
        statuslist << "'R'";

      if (_type & cWoIssued)
        statuslist << "'I'";

      if (_type & cWoClosed)
        statuslist << "'C'";

      sql += "AND (wo_status IN (" + statuslist.join(",") + "))";
    }

    if (_warehouse->isSelected())
      sql += " AND (itemsite_warehous_id=:warehous_id)";

    sql += ") ORDER BY wo_number, wo_subnumber, warehous_code, item_number";
  }

  XSqlQuery wo;
  wo.prepare(sql);
  _warehouse->bindValue(wo);
  wo.exec();
  _wo->populate(wo, _woid);
}
