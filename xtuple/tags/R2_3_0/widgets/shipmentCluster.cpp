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

// shipmentCluster.cpp
// Created 12/19/2006 GJM
// Copyright (c) 2006-2007, OpenMFG, LLC

#include "shipmentcluster.h"

#include <QMessageBox>

ShipmentCluster::ShipmentCluster(QWidget* pParent, const char* pName) :
    VirtualCluster(pParent, pName)
{
    addNumberWidget(new ShipmentClusterLineEdit(this, pName));
    setInfoVisible(false);
}

// should limitToOrder and removeOrderLimit be at the lineedit level?
void ShipmentCluster::limitToOrder(const int head_id)
{
  if (head_id > 0)
  {
    switch (static_cast<ShipmentClusterLineEdit*>(_number)->type())
    {
      case ShipmentClusterLineEdit::SalesOrder:
	setExtraClause(QString("   ((shiphead_order_id=%1)"
			       "AND (shiphead_order_type='SO')) ").arg(head_id));
	break;
      case ShipmentClusterLineEdit::TransferOrder:
	setExtraClause(QString("   ((shiphead_order_id=%1)"
			       "AND (shiphead_order_type='TO')) ").arg(head_id));
	break;
      case ShipmentClusterLineEdit::All:
      default:
	setExtraClause(QString(" (shiphead_order_id=%1) ").arg(head_id));
	break;
    }
  }
//  else
//    removeOrderLimit();
}

void ShipmentCluster::setId(const int pid)
{
//  removeOrderLimit();
  static_cast<ShipmentClusterLineEdit*>(_number)->setId(pid);
}

void ShipmentCluster::removeOrderLimit()
{
  ShipmentClusterLineEdit::ShipmentType type = static_cast<ShipmentClusterLineEdit*>(_number)->type();
  clearExtraClause();
  setType(type);
}

void ShipmentClusterLineEdit::setId(const int pid)
{
  //setType(All);
  VirtualClusterLineEdit::setId(pid);
}

ShipmentClusterLineEdit::ShipmentType ShipmentCluster::type()
{
  return (static_cast<ShipmentClusterLineEdit*>(_number))->type();
}

void ShipmentCluster::setType(QString ptype)
{
  return (static_cast<ShipmentClusterLineEdit*>(_number))->setType(ptype);
}

void ShipmentCluster::setType(ShipmentClusterLineEdit::ShipmentType ptype)
{
  return (static_cast<ShipmentClusterLineEdit*>(_number))->setType(ptype);
}

ShipmentClusterLineEdit::ShipmentClusterLineEdit(QWidget* pParent, const char* pName) :
    VirtualClusterLineEdit(pParent, "shiphead", "shiphead_id", "shiphead_number",
			   "shiphead_shipdate", "shiphead_tracknum", 0, pName)
{
    _type = All;
    setStrict(false);
    setType(SalesOrder);
    setTitles(tr("Shipment"), tr("Shipments"));
}

ShipmentClusterLineEdit::ShipmentType ShipmentClusterLineEdit::type()
{
  return _type;
}

void ShipmentClusterLineEdit::setType(ShipmentType ptype)
{
  if (ptype != _type)
  {
    switch (ptype)
    {
      case All:
	clearExtraClause();
	break;
      case SalesOrder:
	setExtraClause(" (shiphead_order_type='SO') ");
	break;
      case TransferOrder:
	setExtraClause(" (shiphead_order_type='TO') ");
	break;
      default:
	QMessageBox::critical(this, tr("Invalid Shipment Type"),
			      tr("<p>ShipmentClusterLineEdit::setType received "
				 "an invalid ShipmentType %1").arg(ptype));
	return;
	break;
    }
  }
  _type = ptype;
}

void ShipmentClusterLineEdit::setType(QString ptype)
{
  if (ptype == "SO")
    setType(SalesOrder);
  else if (ptype == "TO")
    setType(TransferOrder);
  else
  {
    QMessageBox::critical(this, tr("Invalid Shipment Type"),
			  tr("ShipmentClusterLineEdit::setType received "
			     "an invalid ShipmentType %1").arg(ptype));
    setType(All);
  }
}

VirtualList* ShipmentClusterLineEdit::listFactory()
{
  return new ShipmentList(this);
}

VirtualSearch* ShipmentClusterLineEdit::searchFactory()
{
  return new ShipmentSearch(this);
}

ShipmentList::ShipmentList(QWidget* pParent, Qt::WindowFlags pFlags) :
  VirtualList(pParent, pFlags)
{
  _listTab->headerItem()->setText(1, tr("Shipped Date"));
  _listTab->headerItem()->setText(2, tr("Tracking Number"));
}

ShipmentSearch::ShipmentSearch(QWidget* pParent, Qt::WindowFlags pFlags) :
  VirtualSearch(pParent, pFlags)
{
  _listTab->headerItem()->setText(1, tr("Shipped Date"));
  _listTab->headerItem()->setText(2, tr("Tracking Number"));

  _searchName->setText(tr("Search through Shipped Date"));
  _searchDescrip->setText(tr("Search through Tracking Number"));
}
