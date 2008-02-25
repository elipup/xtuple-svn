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

#include "postCheck.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include "storedProcErrorLookup.h"

postCheck::postCheck(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_post, SIGNAL(clicked()), this, SLOT(sPost()));
  connect(_bankaccnt, SIGNAL(newID(int)), this, SLOT(sHandleBankAccount(int)));

  _captive = FALSE;

  _check->setAllowNull(TRUE);

  _bankaccnt->setType(XComboBox::APBankAccounts);
}

postCheck::~postCheck()
{
  // no need to delete child widgets, Qt does it all for us
}

void postCheck::languageChange()
{
  retranslateUi(this);
}

enum SetResponse postCheck::set(const ParameterList &pParams)
{
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("check_id", &valid);
  if (valid)
  {
    populate(param.toInt());
    _bankaccnt->setEnabled(FALSE);
    _check->setEnabled(FALSE);
  }

  return NoError;
}

void postCheck::sPost()
{
  q.prepare( "SELECT checkhead_bankaccnt_id,"
	     "       postCheck(checkhead_id, NULL) AS result "
             "FROM checkhead "
             "WHERE ((checkhead_id=:checkhead_id)"
             " AND  (NOT checkhead_posted) );" );
  q.bindValue(":checkhead_id", _check->id());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("postCheck", result),
		  __FILE__, __LINE__);
      return;
    }
    omfgThis->sChecksUpdated(q.value("checkhead_bankaccnt_id").toInt(), _check->id(), TRUE);

    if (_captive)
      accept();
    else
    {
      sHandleBankAccount(_bankaccnt->id());
      _close->setText(tr("&Close"));
    }
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void postCheck::sHandleBankAccount(int pBankaccntid)
{
  q.prepare( "SELECT checkhead_id,"
	     "       (TEXT(checkhead_number) || '-' || checkrecip_name) "
             "FROM checkhead LEFT OUTER JOIN"
	     "     checkrecip ON ((checkhead_recip_id=checkrecip_id)"
	     "                AND (checkhead_recip_type=checkrecip_type))"
             "WHERE ((NOT checkhead_void)"
             "  AND  (NOT checkhead_posted)"
             "  AND  (checkhead_printed)"
             "  AND  (checkhead_bankaccnt_id=:bankaccnt_id) ) "
             "ORDER BY checkhead_number;" );
  q.bindValue(":bankaccnt_id", pBankaccntid);
  q.exec();
  _check->populate(q);
  _check->setNull();
}

void postCheck::populate(int pcheckid)
{
  q.prepare( "SELECT checkhead_bankaccnt_id "
             "FROM checkhead "
             "WHERE (checkhead_id=:check_id);" );
  q.bindValue(":check_id", pcheckid);
  q.exec();
  if (q.first())
  {
    _bankaccnt->setId(q.value("checkhead_bankaccnt_id").toInt());
    _check->setId(pcheckid);
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
