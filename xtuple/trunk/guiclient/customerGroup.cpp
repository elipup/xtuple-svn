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

#include "customerGroup.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include "crmacctcluster.h"
#include "customerGroup.h"

customerGroup::customerGroup(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);

    connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
    connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
    connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
    connect(_close, SIGNAL(clicked()), this, SLOT(sClose()));
    connect(_name, SIGNAL(lostFocus()), this, SLOT(sCheck()));

    _custgrpitem->addColumn(tr("Number"), _itemColumn, Qt::AlignLeft );
    _custgrpitem->addColumn(tr("Name"),   -1,          Qt::AlignLeft );
}

customerGroup::~customerGroup()
{
    // no need to delete child widgets, Qt does it all for us
}

void customerGroup::languageChange()
{
    retranslateUi(this);
}

enum SetResponse customerGroup::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("custgrp_id", &valid);
  if (valid)
  {
    _custgrpid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      q.exec("SELECT NEXTVAL('custgrp_custgrp_id_seq') AS _custgrp_id;");
      if (q.first())
        _custgrpid = q.value("_custgrp_id").toInt();
      else if (q.lastError().type() != QSqlError::None)
      {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return UndefinedError;
      }

      connect(_custgrpitem, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));

      _name->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      sFillList();

      connect(_custgrpitem, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));

      _name->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _name->setEnabled(FALSE);
      _descrip->setEnabled(FALSE);
      _new->setEnabled(FALSE);
      _close->setText(tr("&Close"));
      _save->hide();

      sFillList();

      _close->setFocus();
    }
  }

  return NoError;
}

void customerGroup::sCheck()
{
  _name->setText(_name->text().stripWhiteSpace());
  if ((_mode == cNew) && (_name->text().length()))
  {
    q.prepare( "SELECT custgrp_id "
               "FROM custgrp "
               "WHERE (UPPER(custgrp_name)=UPPER(:custgrp_name));" );
    q.bindValue(":custgrp_name", _name->text());
    q.exec();
    if (q.first())
    {
      _custgrpid = q.value("custgrp_id").toInt();
      _mode = cEdit;
      populate();

      _name->setEnabled(FALSE);
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void customerGroup::sClose()
{
  if (_mode == cNew)
  {
    q.prepare( "DELETE FROM custgrpitem "
               "WHERE (custgrpitem_custgrp_id=:custgrp_id);"
               "DELETE FROM custgrp "
               "WHERE (custgrp_id=:custgrp_id);" );
    q.bindValue(":custgrp_id", _custgrpid);
    q.exec();
    if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    }
  }

  reject();
}

void customerGroup::sSave()
{
  if (_mode == cNew)
    q.prepare( "INSERT INTO custgrp "
               "(custgrp_id, custgrp_name, custgrp_descrip) "
               "VALUES "
               "(:custgrp_id, :custgrp_name, :custgrp_descrip);" );
  else if (_mode == cEdit)
    q.prepare( "UPDATE custgrp "
               "SET custgrp_name=:custgrp_name, custgrp_descrip=:custgrp_descrip "
               "WHERE (custgrp_id=:custgrp_id);" );

  q.bindValue(":custgrp_id", _custgrpid);
  q.bindValue(":custgrp_name", _name->text());
  q.bindValue(":custgrp_descrip", _descrip->text().stripWhiteSpace());
  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(_custgrpid);
}

void customerGroup::sDelete()
{
  q.prepare( "DELETE FROM custgrpitem "
             "WHERE (custgrpitem_id=:custgrpitem_id);" );
  q.bindValue(":custgrpitem_id", _custgrpitem->id());
  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void customerGroup::sNew()
{
  ParameterList params;

  CRMAcctList newdlg(this, "", TRUE);
  newdlg.setSubtype(CRMAcctLineEdit::Cust);

  int custid;
  if ((custid = newdlg.exec()) != XDialog::Rejected)
  {
    q.prepare( "SELECT custgrpitem_id "
               "FROM custgrpitem "
               "WHERE ( (custgrpitem_custgrp_id=:custgrpitem_custgrp_id)"
               " AND (custgrpitem_cust_id=:custgrpitem_cust_id) );" );
    q.bindValue(":custgrpitem_custgrp_id", _custgrpid);
    q.bindValue(":custgrpitem_cust_id", custid);
    q.exec();
    if (q.first())
      return;
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    q.prepare( "INSERT INTO custgrpitem "
               "(custgrpitem_custgrp_id, custgrpitem_cust_id) "
               "VALUES "
               "(:custgrpitem_custgrp_id, :custgrpitem_cust_id);" );
    q.bindValue(":custgrpitem_custgrp_id", _custgrpid);
    q.bindValue(":custgrpitem_cust_id", custid);
    q.exec();
    if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    sFillList();
  }
}

void customerGroup::sFillList()
{
  q.prepare( "SELECT custgrpitem_id, cust_number, cust_name "
             "FROM custgrpitem, cust "
             "WHERE ( (custgrpitem_cust_id=cust_id) "
             " AND (custgrpitem_custgrp_id=:custgrp_id) ) "
             "ORDER BY cust_number;" );
  q.bindValue(":custgrp_id", _custgrpid);
  q.exec();
  _custgrpitem->clear();
  _custgrpitem->populate(q);
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void customerGroup::populate()
{
  q.prepare( "SELECT custgrp_name, custgrp_descrip "
             "FROM custgrp "
             "WHERE (custgrp_id=:custgrp_id);" );
  q.bindValue(":custgrp_id", _custgrpid);
  q.exec();
  if (q.first())
  {
    _name->setText(q.value("custgrp_name").toString());
    _descrip->setText(q.value("custgrp_descrip").toString());
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
