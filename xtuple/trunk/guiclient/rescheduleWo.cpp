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

#include "rescheduleWo.h"

#include <QMessageBox>
#include <QVariant>

rescheduleWo::rescheduleWo(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_reschedule, SIGNAL(clicked()), this, SLOT(sReschedule()));

  _captive = FALSE;

  _wo->setType(cWoOpen | cWoExploded);
  _cmnttype->setType(XComboBox::AllCommentTypes);

  Preferences _pref = Preferences(omfgThis->username());
  if (_pref.boolean("XCheckBox/forgetful"))
    _changeChildren->setChecked(true);
  _commentGroup->setEnabled(_postComment->isChecked());
}

rescheduleWo::~rescheduleWo()
{
  // no need to delete child widgets, Qt does it all for us
}

void rescheduleWo::languageChange()
{
  retranslateUi(this);
}

enum SetResponse rescheduleWo::set(const ParameterList &pParams)
{
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("wo_id", &valid);
  if (valid)
  {
    _wo->setId(param.toInt());
    _wo->setReadOnly(TRUE);
    _newStartDate->setFocus();
  }

  return NoError;
}

void rescheduleWo::sReschedule()
{
  if ((_newStartDate->isValid()) && (_newDueDate->isValid()))
  {
    if (_wo->status() == 'R')
    {
      QMessageBox::warning( this, tr("Cannot Reschedule Released W/O"),
                            tr( "The selected Work Order has been Released.\n"
                                "You must Recall this Work Order before Rescheduling it." ) );
      return;
    }

    q.prepare("SELECT changeWoDates(:wo_id, :startDate, :dueDate, :rescheduleChildren);");
    q.bindValue(":wo_id", _wo->id());
    q.bindValue(":startDate", _newStartDate->date());
    q.bindValue(":dueDate", _newDueDate->date());
    q.bindValue(":rescheduleChildren", QVariant(_changeChildren->isChecked(), 0));
    q.exec();

    if (_postComment->isChecked())
    {
      q.prepare("SELECT postComment(:cmnttype_id, 'W', :wo_id, :comment) AS _result");
      q.bindValue(":cmnttype_id", _cmnttype->id());
      q.bindValue(":wo_id", _wo->id());
      q.bindValue(":comment", _comment->text());
      q.exec();
    }

    omfgThis->sWorkOrdersUpdated(_wo->id(), TRUE);

    if (_captive)
      close();
    else
    {
      _wo->setId(-1);
      _close->setText(tr("&Close"));
      _wo->setFocus();
    }
  }
}

