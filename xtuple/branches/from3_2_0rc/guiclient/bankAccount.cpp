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

#include "bankAccount.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

bankAccount::bankAccount(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_bankName,SIGNAL(textChanged(QString)), this, SLOT(sNameChanged(QString)));
  connect(_save,               SIGNAL(clicked()), this, SLOT(sSave()));

  _nextCheckNum->setValidator(omfgThis->orderVal());
  _routing->setValidator(new QIntValidator(100000000, 999999999, this));
  _federalReserveDest->setValidator(new QIntValidator(100000000, 999999999, this));

  _assetAccount->setType(GLCluster::cAsset);
  _currency->setType(XComboBox::Currencies);
  _currency->setLabel(_currencyLit);

  _form->setAllowNull(TRUE);
  _form->populate( "SELECT form_id, form_name, form_name "
                   "FROM form "
                   "WHERE form_key='Chck' "
                   "ORDER BY form_name;" );

  if (_metrics->boolean("ACHEnabled"))
  {
    q.prepare("SELECT fetchMetricText('ACHCompanyName') AS name,"
              "       formatACHCompanyId() AS number;");
    q.exec();
    if (q.first())
    {
      _useCompanyIdOrigin->setText(q.value("name").toString());
      _defaultOrigin->setText(q.value("number").toString());
    }
    else if (q.lastError().type() != QSqlError::None)
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);

    if (omfgThis->_key.isEmpty())
      _transmitTab->setEnabled(false);
  }
  else
    _tab->removeTab(_tab->indexOf(_transmitTab));

  _bankaccntid = -1;
}

bankAccount::~bankAccount()
{
  // no need to delete child widgets, Qt does it all for us
}

void bankAccount::languageChange()
{
  retranslateUi(this);
}

enum SetResponse bankAccount::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("bankaccnt_id", &valid);
  if (valid)
  {
    _bankaccntid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _useCompanyIdOrigin->setChecked(true);
      _name->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
      _description->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _name->setEnabled(FALSE);
      _description->setEnabled(FALSE);
      _bankName->setEnabled(FALSE);
      _accountNumber->setEnabled(FALSE);
      _type->setEnabled(FALSE);
      _currency->setEnabled(FALSE);
      _ap->setEnabled(FALSE);
      _nextCheckNum->setEnabled(FALSE);
      _form->setEnabled(FALSE);
      _ar->setEnabled(FALSE);
      _assetAccount->setReadOnly(TRUE);
      _close->setText(tr("&Close"));
      _save->hide();

      _close->setFocus();
    }
  }

  return NoError;
}

void bankAccount::sCheck()
{
  _name->setText(_name->text().trimmed());
  if ((_mode == cNew) && (_name->text().length()))
  {
    q.exec( "SELECT bankaccnt_id "
            "FROM bankaccnt "
            "WHERE (UPPER(bankaccnt_name)=UPPER(:bankaccnt_name));" );
    q.bindValue(":bankaccnt_name", _name->text());
    q.exec();
    if (q.first())
    {
      _bankaccntid = q.value("bankaccnt_id").toInt();
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

void bankAccount::sSave()
{
  struct {
    bool        condition;
    QString     msg;
    QWidget    *widget;
  } error[] = {
    { !_assetAccount->isValid(), 
      tr("<p>Select an G/L Account for this Bank Account before saving it."),
      _assetAccount
    },
    { _transmitGroup->isChecked() && _routing->text().trimmed().isEmpty(),
      tr("<p>The bank's Routing Number is required for ACH Check handling."),
      _routing
    },
    { _transmitGroup->isChecked() && _routing->text().trimmed().size() < 9,
      tr("<p>The bank's Routing Number must be a 9 digit number."),
      _routing
    },
    { _transmitGroup->isChecked() &&
      ! (_useCompanyIdOrigin->isChecked() ||
         _useRoutingNumberOrigin->isChecked() ||
         _useOtherOrigin->isChecked()),
      tr("<p>You must choose which value to use for the Immediate Origin."),
      _useCompanyIdOrigin
    },
    { _transmitGroup->isChecked() && _useOtherOrigin->isChecked() &&
      _otherOriginName->text().trimmed().isEmpty(),
      tr("<p>You must enter an Immediate Origin Name if you choose 'Other'."),
      _otherOriginName
    },
    { _transmitGroup->isChecked() && _useOtherOrigin->isChecked() &&
      _otherOrigin->text().trimmed().isEmpty(),
      tr("<p>You must enter an Immediate Origin if you choose 'Other'."),
      _otherOrigin
    },
    { _transmitGroup->isChecked() &&
      ! (_useRoutingNumberDest->isChecked() ||
         _useFederalReserveDest->isChecked() ||
         _useOtherDest->isChecked()),
      tr("<p>You must choose which value to use for the Immediate Destination."),
      _useRoutingNumberDest
    },
    { _transmitGroup->isChecked() && _useFederalReserveDest->isChecked() &&
      _federalReserveDest->text().trimmed().isEmpty(),
      tr("<p>You must enter a Federal Reserve Routing Number if you choose "
         "'Federal Reserve'."),
      _federalReserveDest
    },
    { _transmitGroup->isChecked() && _useOtherDest->isChecked() &&
      _otherDestName->text().trimmed().isEmpty(),
      tr("<p>You must enter an Immediate Destination Name if you choose "
         "'Other'."),
      _otherDestName
    },
    { _transmitGroup->isChecked() && _useOtherDest->isChecked() &&
      _otherDest->text().trimmed().isEmpty(),
      tr("<p>You must enter an Immediate Destination number if you choose "
         "'Other'."),
      _otherDest
    }
  };

  for (unsigned int i = 0; i < sizeof(error) / sizeof(error[0]); i++)
    if (error[i].condition)
    {
      QMessageBox::critical(this, tr("Cannot Save Bank Account"), error[i].msg);
      error[i].widget->setFocus();
      return;
    }

  if (_transmitGroup->isChecked())
  {
    if (_useOtherOrigin->isChecked() && _otherOrigin->text().size() < 10 &&
      QMessageBox::question(this, tr("Immediate Origin Too Small?"),
                            tr("<p>The Immediate Origin is usually either a "
                               "10 digit Company Id number or a space followed "
                               "by a 9 digit Routing Number. Does %1 "
                               "match what your bank told you to enter here?")
                            .arg(_otherOrigin->text()),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No) == QMessageBox::No)
    {
      _otherOrigin->setFocus();
      return;
    }

    if (_useRoutingNumberOrigin->isChecked() &&
        _useRoutingNumberDest->isChecked() &&
        QMessageBox::question(this, tr("Use Bank Routing Number twice?"),
                              tr("<p>Are you sure your bank expects the "
                                 "Bank Routing Number as both the Immediate "
                                 "Origin and the Immediate Destination?"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::No)
    {
      _useRoutingNumberDest->setFocus();
      return;
    }

    if (_useOtherDest->isChecked() && _otherDest->text().size() < 10 &&
      QMessageBox::question(this, tr("Immediate Destination Too Small?"),
                            tr("<p>The Immediate Destination is usually either "
                               "a 10 digit Company Id number or a space "
                               "followed by a 9 digit Routing Number. Does %1 "
                               "match what your bank told you to enter here?")
                            .arg(_otherDest->text()),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No) == QMessageBox::No)
    {
      _otherOrigin->setFocus();
      return;
    }
  }

  q.prepare( "SELECT bankaccnt_id FROM bankaccnt "
             "WHERE ((bankaccnt_name = :bankaccnt_name) "
             "AND (bankaccnt_id != :bankaccnt_id));");
  q.bindValue(":bankaccnt_name", _name->text());
  q.bindValue(":bankaccnt_id",   _bankaccntid);
  q.exec();
  if (q.first())
  {
    QMessageBox::critical( this, tr("Cannot Save Bank Account"),
                           tr("<p>Bank Account name is already in use. Please "
                              "enter a unique name.") );

    _name->setFocus();
    return;
  }
  
  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('bankaccnt_bankaccnt_id_seq') AS _bankaccnt_id");
    if (q.first())
      _bankaccntid = q.value("_bankaccnt_id").toInt();

    q.prepare( "INSERT INTO bankaccnt "
               "( bankaccnt_id, bankaccnt_name, bankaccnt_descrip,"
               "  bankaccnt_bankname, bankaccnt_accntnumber,"
               "  bankaccnt_type, bankaccnt_ap, bankaccnt_ar,"
               "  bankaccnt_accnt_id, bankaccnt_notes,"
               "  bankaccnt_nextchknum, bankaccnt_check_form_id, "
	       "  bankaccnt_curr_id, bankaccnt_ach_enabled,"
               "  bankaccnt_routing, bankaccnt_ach_origintype,"
               "  bankaccnt_ach_originname, bankaccnt_ach_origin,"
               "  bankaccnt_ach_desttype, bankaccnt_ach_fed_dest,"
               "  bankaccnt_ach_destname, bankaccnt_ach_dest,"
               "  bankaccnt_ach_genchecknum, bankaccnt_ach_leadtime)"
               "VALUES "
               "( :bankaccnt_id, :bankaccnt_name, :bankaccnt_descrip,"
               "  :bankaccnt_bankname, :bankaccnt_accntnumber,"
               "  :bankaccnt_type, :bankaccnt_ap, :bankaccnt_ar,"
               "  :bankaccnt_accnt_id, :bankaccnt_notes,"
               "  :bankaccnt_nextchknum, :bankaccnt_check_form_id, "
	       "  :bankaccnt_curr_id, :bankaccnt_ach_enabled,"
               "  :bankaccnt_routing, :bankaccnt_ach_origintype,"
               "  :bankaccnt_ach_originname, :bankaccnt_ach_origin,"
               "  :bankaccnt_ach_desttype, :bankaccnt_ach_fed_dest,"
               "  :bankaccnt_ach_destname, :bankaccnt_ach_dest,"
               "  :bankaccnt_ach_genchecknum, :bankaccnt_ach_leadtime);" );
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE bankaccnt "
               "SET bankaccnt_name=:bankaccnt_name,"
               "    bankaccnt_descrip=:bankaccnt_descrip,"
               "    bankaccnt_bankname=:bankaccnt_bankname,"
               "    bankaccnt_accntnumber=:bankaccnt_accntnumber,"
               "    bankaccnt_type=:bankaccnt_type,"
               "    bankaccnt_ap=:bankaccnt_ap,"
               "    bankaccnt_ar=:bankaccnt_ar,"
               "    bankaccnt_accnt_id=:bankaccnt_accnt_id,"
               "    bankaccnt_nextchknum=:bankaccnt_nextchknum, "
	       "    bankaccnt_check_form_id=:bankaccnt_check_form_id, "
	       "    bankaccnt_curr_id=:bankaccnt_curr_id,"
	       "    bankaccnt_notes=:bankaccnt_notes,"
	       "    bankaccnt_ach_enabled=:bankaccnt_ach_enabled,"
	       "    bankaccnt_routing=:bankaccnt_routing,"
               "    bankaccnt_ach_origintype=:bankaccnt_ach_origintype,"
	       "    bankaccnt_ach_originname=:bankaccnt_ach_originname,"
	       "    bankaccnt_ach_origin=:bankaccnt_ach_origin,"
               "    bankaccnt_ach_desttype=:bankaccnt_ach_desttype,"
               "    bankaccnt_ach_fed_dest=:bankaccnt_ach_fed_dest,"
               "    bankaccnt_ach_destname=:bankaccnt_ach_destname,"
               "    bankaccnt_ach_dest=:bankaccnt_ach_dest,"
               "    bankaccnt_ach_genchecknum=:bankaccnt_ach_genchecknum,"
               "    bankaccnt_ach_leadtime=:bankaccnt_ach_leadtime "
               "WHERE (bankaccnt_id=:bankaccnt_id);" );
  
  q.bindValue(":bankaccnt_id",          _bankaccntid);
  q.bindValue(":bankaccnt_name",        _name->text());
  q.bindValue(":bankaccnt_descrip",     _description->text().trimmed());
  q.bindValue(":bankaccnt_bankname",    _bankName->text());
  q.bindValue(":bankaccnt_accntnumber", _accountNumber->text());
  q.bindValue(":bankaccnt_ap",          QVariant(_ap->isChecked()));
  q.bindValue(":bankaccnt_ar",          QVariant(_ar->isChecked()));
  q.bindValue(":bankaccnt_accnt_id",    _assetAccount->id());
  q.bindValue(":bankaccnt_curr_id",     _currency->id());
  q.bindValue(":bankaccnt_notes",       _notes->text().stripWhiteSpace());
  q.bindValue(":bankaccnt_ach_enabled", _transmitGroup->isChecked());
  q.bindValue(":bankaccnt_routing",     _routing->text());

  if (_useCompanyIdOrigin->isChecked())
    q.bindValue(":bankaccnt_ach_origintype",  "I");
  else if (_useRoutingNumberOrigin->isChecked())
    q.bindValue(":bankaccnt_ach_origintype",  "B");
  else if (_useOtherOrigin->isChecked())
    q.bindValue(":bankaccnt_ach_origintype",  "O");
  q.bindValue(":bankaccnt_ach_originname",    _otherOriginName->text());
  q.bindValue(":bankaccnt_ach_origin",        _otherOrigin->text());
  q.bindValue(":bankaccnt_ach_genchecknum",   _genCheckNumber->isChecked());
  q.bindValue(":bankaccnt_ach_leadtime",      _settlementLeadtime->value());

  if (_useRoutingNumberDest->isChecked())
    q.bindValue(":bankaccnt_ach_desttype",    "B");
  else if (_useFederalReserveDest->isChecked())
    q.bindValue(":bankaccnt_ach_desttype",    "F");
  else if (_useOtherDest->isChecked())
    q.bindValue(":bankaccnt_ach_desttype",    "O");
  q.bindValue(":bankaccnt_ach_fed_dest",      _federalReserveDest->text());
  q.bindValue(":bankaccnt_ach_destname",      _otherDestName->text());
  q.bindValue(":bankaccnt_ach_dest",          _otherDest->text());

  q.bindValue(":bankaccnt_nextchknum",    _nextCheckNum->text().toInt());
  q.bindValue(":bankaccnt_check_form_id", _form->id());

  if (_type->currentIndex() == 0)
    q.bindValue(":bankaccnt_type", "K");
  else if (_type->currentIndex() == 1)
    q.bindValue(":bankaccnt_type", "C");

  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(_bankaccntid);
}

void bankAccount::populate()
{
  q.prepare( "SELECT * "
             "FROM bankaccnt "
             "WHERE (bankaccnt_id=:bankaccnt_id);" );
  q.bindValue(":bankaccnt_id", _bankaccntid);
  q.exec();
  if (q.first())
  {
    _name->setText(q.value("bankaccnt_name"));
    _description->setText(q.value("bankaccnt_descrip"));
    _bankName->setText(q.value("bankaccnt_bankname"));
    _accountNumber->setText(q.value("bankaccnt_accntnumber"));
    _ap->setChecked(q.value("bankaccnt_ap").toBool());
    _ar->setChecked(q.value("bankaccnt_ar").toBool());
    _nextCheckNum->setText(q.value("bankaccnt_nextchknum"));
    _form->setId(q.value("bankaccnt_check_form_id").toInt());

    _assetAccount->setId(q.value("bankaccnt_accnt_id").toInt());
    _currency->setId(q.value("bankaccnt_curr_id").toInt());
    _notes->setText(q.value("bankaccnt_notes").toString());

    _transmitGroup->setChecked(q.value("bankaccnt_ach_enabled").toBool());   
    _routing->setText(q.value("bankaccnt_routing").toString());      
    _genCheckNumber->setChecked(q.value("bankaccnt_ach_genchecknum").toBool());
    _settlementLeadtime->setValue(q.value("bankaccnt_ach_leadtime").toInt());

    if (q.value("bankaccnt_ach_origintype").toString() == "I")   
      _useCompanyIdOrigin->setChecked(true);
    else if (q.value("bankaccnt_ach_origintype").toString() == "B")   
      _useRoutingNumberOrigin->setChecked(true);
    else if (q.value("bankaccnt_ach_origintype").toString() == "O")   
      _useOtherOrigin->setChecked(true);
    _otherOriginName->setText(q.value("bankaccnt_ach_originname").toString());
    _otherOrigin->setText(q.value("bankaccnt_ach_origin").toString());    

    if (q.value("bankaccnt_ach_desttype").toString() == "B")   
      _useRoutingNumberDest->setChecked(true);
    else if (q.value("bankaccnt_ach_desttype").toString() == "F")   
      _useFederalReserveDest->setChecked(true);
    else if (q.value("bankaccnt_ach_desttype").toString() == "O")   
      _useOtherDest->setChecked(true);
    _federalReserveDest->setText(q.value("bankaccnt_ach_fed_dest").toString());
    _otherDestName->setText(q.value("bankaccnt_ach_destname").toString());
    _otherDest->setText(q.value("bankaccnt_ach_dest").toString());

    if (q.value("bankaccnt_type").toString() == "K")
      _type->setCurrentIndex(0);
    else if (q.value("bankaccnt_type").toString() == "C")
      _type->setCurrentIndex(1);
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void bankAccount::sNameChanged(QString pName)
{
  _useRoutingNumberDest->setText(pName);
  _useRoutingNumberOrigin->setText(pName);
}
