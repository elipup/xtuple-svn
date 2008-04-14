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

//  xlineedit.cpp
//  Created 01/03/2003 JSL
//  Copyright (c) 2003-2008, OpenMFG, LLC

#include "xlineedit.h"
#include <QMouseEvent>
#include <QFocusEvent>
#include <QKeyEvent>

XLineEdit::XLineEdit(QWidget *parent, const char *name) :
  QLineEdit(parent, name)
{
  setAcceptDrops(FALSE);

#ifdef Q_WS_MAC
  QFont f = font();
  f.setPointSize(f.pointSize() - 2);
  setFont(f);
#endif

  _parsed = TRUE;
  _valid = FALSE;

  _id = -1;
}

bool XLineEdit::isValid()
{
  sParse();
  return _valid;
}

int XLineEdit::id()
{
  sParse();
  return _id;
}

void XLineEdit::sParse()
{
}

double XLineEdit::toDouble(bool *pIsValid)
{
  QString value;

  value = text();

//  Remove all 's
  value = value.remove(',');

//  Remove all but 1 .
  int pos = value.find('.');
  if (pos > -1)
    value = (value.left(pos + 1) + value.right(value.length() - pos - 1).remove('.'));

  return value.toDouble(pIsValid);
}

void XLineEdit::setDataWidgetMap(XDataWidgetMapper* m)
{
  m->addFieldMapping(this, _fieldName);
}

void XLineEdit::setText(const QVariant &pVariant)
{
  QLineEdit::setText(pVariant.toString());
}

void XLineEdit::mousePressEvent(QMouseEvent *event)
{
  emit clicked();

  QLineEdit::mousePressEvent(event);
}

void XLineEdit::keyPressEvent(QKeyEvent *event)
{
  if (event->state() == Qt::ControlModifier)
  {
    if (event->key() == Qt::Key_L)
    {
      _parsed = TRUE;
      emit (requestList());
    }

    else if (event->key() == Qt::Key_S)
    {
      _parsed = TRUE;
      emit (requestSearch());
    }

    else if (event->key() == Qt::Key_A)
    {
      _parsed = TRUE;
      emit (requestAlias());
    }
  }
  else
    _parsed = FALSE;

  if (_parsed)
    event->accept();
  else
    QLineEdit::keyPressEvent(event);
}

void XLineEdit::focusInEvent(QFocusEvent *pEvent)
{
  if(!text().isEmpty())
    setSelection(0, text().length());

  QLineEdit::focusInEvent(pEvent);
}

