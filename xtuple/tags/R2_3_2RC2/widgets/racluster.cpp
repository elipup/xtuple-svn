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

// racluster.cpp
// Created 11/05/2007 GJM
// Copyright (c) 2007, OpenMFG, LLC

/*
#include <QGridLayout>
#include <QMessageBox>
#include <QSqlError>

#include <metasql.h>

#include "xsqlquery.h"
*/

#include "racluster.h"

RaCluster::RaCluster(QWidget *pParent, const char *pName) :
  VirtualCluster(pParent, pName)
{
  addNumberWidget(new RaLineEdit(this, pName));
  connect(_number, SIGNAL(numberChanged(const QString &)), this, SIGNAL(numberChanged(const QString &)));
}

RaLineEdit::RaStatuses RaCluster::allowedStatuses() const
{
  return ((RaLineEdit*)_number)->allowedStatuses();
}

void RaCluster::setAllowedStatuses(const RaLineEdit::RaStatuses p)
{
  ((RaLineEdit*)_number)->setAllowedStatuses(p);
}

bool RaCluster::isClosed() const
{
  return ((RaLineEdit*)_number)->isClosed();
}

bool RaCluster::isOpen() const
{
  return ((RaLineEdit*)_number)->isOpen();
}

RaLineEdit::RaStatus RaCluster::status() const
{
  return ((RaLineEdit*)_number)->status();
}

// LineEdit ///////////////////////////////////////////////////////////////////

RaLineEdit::RaLineEdit(QWidget *pParent, const char *pName) :
  VirtualClusterLineEdit(pParent, "rahead", "rahead_id",
			 "rahead_number", "rahead_billtoname",
			 0, pName)
{
  setTitles(tr("Return Authorization"), tr("Return Authorizations"));

  setValidator(new QIntValidator(0, 9999999, this));
  connect(this, SIGNAL(textChanged(const QString &)), this, SIGNAL(numberChanged(const QString &)));
}

RaLineEdit::RaStatuses RaLineEdit::allowedStatuses() const
{
  return _statuses;
}

void RaLineEdit::setAllowedStatuses(const RaLineEdit::RaStatuses p)
{
  if (p & (Open | Closed) || p == 0)
    clearExtraClause();
  else if (p & Open)
    setExtraClause(" AND EXISTS ( SELECT raitem_id "
                   " FROM ratiem "
                   " WHERE ((raitem_status='O') "
                   " AND (raitem_rahead_id=rahead_id)) ");
  else if (p & Closed)
    setExtraClause(" AND NOT EXISTS ( SELECT raitem_id "
                   " FROM ratiem "
                   " WHERE ((raitem_status='O') "
                   " AND (raitem_rahead_id=rahead_id)) ");
  else
    clearExtraClause();

  _statuses = p;
}

bool RaLineEdit::isClosed() const
{
  return _description == "C";
}

bool RaLineEdit::isOpen() const
{
  return _description == "O";
}

RaLineEdit::RaStatus RaLineEdit::status() const
{
  if (_description == "C")	return Closed;
  else if (_description == "O")	return Open;
  else return AnyStatus;
}

// List ///////////////////////////////////////////////////////////////////////

RaList::RaList(QWidget *pParent, Qt::WindowFlags pFlags) :
  VirtualList(pParent, pFlags)
{
  _listTab->headerItem()->setData(1, Qt::DisplayRole, tr("Disposition"));
  _listTab->headerItem()->setData(2, Qt::DisplayRole, tr("Status"));
}
