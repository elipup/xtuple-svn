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

#include "selectPayment.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include "applyDiscount.h"

selectPayment::selectPayment(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_discount, SIGNAL(clicked()),      this, SLOT(sDiscount()));
  connect(_save,     SIGNAL(clicked()),      this, SLOT(sSave()));
  connect(_selected, SIGNAL(idChanged(int)), this, SLOT(sPriceGroup()));

  _bankaccnt->setAllowNull(TRUE);
  _bankaccnt->setType(XComboBox::APBankAccounts);

  _vendor->setReadOnly(TRUE);
  sPriceGroup();
}

selectPayment::~selectPayment()
{
  // no need to delete child widgets, Qt does it all for us
}

void selectPayment::languageChange()
{
  retranslateUi(this);
}

enum SetResponse selectPayment::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("bankaccnt_id", &valid);
  if (valid)
    _bankaccnt->setId(param.toInt());

  param = pParams.value("apopen_id", &valid);
  if (valid)
  {
    _apopenid = param.toInt();

    q.prepare( "SELECT apselect_id "
               "FROM apselect "
               "WHERE (apselect_apopen_id=:apopen_id);" );
    q.bindValue(":apopen_id", _apopenid);
    q.exec();
    if (q.first())
    {
      _mode = cEdit;
      _apselectid = q.value("apselect_id").toInt();
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return UndefinedError;
    }
    else
    {
      _mode = cNew;
      _apselectid = -1;
      _discountAmount->setLocalValue(0.0);
    }

    populate();
  }

  return NoError;
}

void selectPayment::sSave()
{
  if (_selected->isZero())
  {
    QMessageBox::warning( this, tr("Cannot Select for Payment"),
      tr("<p>You must specify an amount greater than zero. "
         "If you want to clear this selection you may do so "
         "from the screen you selected this payment from.") );
    return;
  }
  else if ((_selected->localValue() + _discountAmount->localValue()) > (_amount->localValue() + 0.0000001))
  {
    QMessageBox::warning( this, tr("Cannot Select for Payment"),
      tr("You must specify an amount smaller than or equal to the Balance.") );
    _selected->setFocus();
    return;
  }

  if (_bankaccnt->id() == -1)
  {
    QMessageBox::warning( this, tr("Cannot Select for Payment"),
                          tr("<p>You must select a Bank Account from which "
			     "this Payment is to be paid.") );
    _bankaccnt->setFocus();
    return;
  }

  q.prepare("SELECT bankaccnt_curr_id, currConcat(bankaccnt_curr_id) AS currAbbr "
	    "FROM bankaccnt "
	    "WHERE bankaccnt_id = :accntid;");
  q.bindValue(":accntid", _bankaccnt->id());
  q.exec();
  if (q.first())
  {
    if (q.value("bankaccnt_curr_id").toInt() != _selected->id())
    {
	int response = QMessageBox::question(this,
			     tr("Currencies Do Not Match"),
			     tr("<p>The currency selected for this payment "
				 "(%1) is not the same as the currency for the "
				 "Bank Account (%2). Would you like to use "
				 "this Bank Account anyway?")
			       .arg(_selected->currAbbr())
			       .arg(q.value("currAbbr").toString()),
			      QMessageBox::Yes,
			      QMessageBox::No | QMessageBox::Default);
	if (response == QMessageBox::No)
	{
	    _bankaccnt->setFocus();
	    return;
	}
    }
  }
  else
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('apselect_apselect_id_seq') AS apselect_id;");
    if (q.first())
      _apselectid = q.value("apselect_id").toInt();
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    q.prepare( "INSERT INTO apselect "
               "( apselect_id, apselect_apopen_id,"
               "  apselect_amount, apselect_bankaccnt_id, "
	       "  apselect_curr_id, apselect_date, apselect_discount ) "
               "VALUES "
               "( :apselect_id, :apselect_apopen_id,"
               "  :apselect_amount, :apselect_bankaccnt_id, "
	       "  :apselect_curr_id, :apselect_docdate, :apselect_discount );" );
    q.bindValue(":apselect_apopen_id", _apopenid);
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE apselect "
               "SET apselect_amount=:apselect_amount, "
	       " apselect_bankaccnt_id=:apselect_bankaccnt_id, "
	       " apselect_curr_id=:apselect_curr_id, "
	       " apselect_date=:apselect_docdate,"
               " apselect_discount=:apselect_discount "
               "WHERE (apselect_id=:apselect_id);" );

  q.bindValue(":apselect_id", _apselectid);
  q.bindValue(":apselect_amount", _selected->localValue());
  q.bindValue(":apselect_bankaccnt_id", _bankaccnt->id());
  q.bindValue(":apselect_curr_id", _selected->id());
  q.bindValue(":apselect_docdate", _docDate->date());
  q.bindValue(":apselect_discount", _discountAmount->localValue());
  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  omfgThis->sPaymentsUpdated(_bankaccnt->id(), _apselectid, TRUE);

  done (_apselectid);
}

void selectPayment::populate()
{
  q.prepare( "SELECT apopen_vend_id, apopen_docnumber, apopen_ponumber,"
             "       apopen_docdate, apopen_duedate,"
             "       apopen_amount, "
	     "       COALESCE(apselect_curr_id, apopen_curr_id) AS curr_id, "
             "       (apopen_amount - apopen_paid"
             "          - COALESCE((SELECT SUM(checkitem_amount + checkitem_discount) "
             "                        FROM checkitem, checkhead "
             "                       WHERE ((checkitem_checkhead_id=checkhead_id) "
             "                         AND (checkitem_apopen_id=apopen_id) "
             "                         AND (NOT checkhead_void) "
             "                         AND (NOT checkhead_posted)) "
             "                     ),0)) AS f_amount,"
             "       COALESCE(apselect_amount, (apopen_amount - apopen_paid"
             "          - COALESCE((SELECT SUM(checkitem_amount + checkitem_discount) "
             "                        FROM checkitem, checkhead "
             "                       WHERE ((checkitem_checkhead_id=checkhead_id) "
             "                         AND (checkitem_apopen_id=apopen_id) "
             "                         AND (NOT checkhead_void) "
             "                         AND (NOT checkhead_posted)) "
             "                     ),0))) AS f_selected,"
             "       COALESCE(apselect_discount, 0.0) AS discount,"
             "       COALESCE(apselect_bankaccnt_id, -1) AS bankaccnt_id,"
             "       (terms_code || '-' || terms_descrip) AS f_terms "
             "FROM terms RIGHT OUTER JOIN apopen ON (apopen_terms_id=terms_id) "
	     "      LEFT OUTER JOIN apselect ON (apselect_apopen_id=apopen_id) "
             "WHERE apopen_id=:apopen_id;" );
  q.bindValue(":apopen_id", _apopenid);
  q.exec();
  if (q.first())
  {
    _selected->setId(q.value("curr_id").toInt());
    _selected->setLocalValue(q.value("f_selected").toDouble());
    _vendor->setId(q.value("apopen_vend_id").toInt());
    _docNumber->setText(q.value("apopen_docnumber").toString());
    _poNum->setText(q.value("apopen_ponumber").toString());
    _docDate->setDate(q.value("apopen_docdate").toDate());
    _dueDate->setDate(q.value("apopen_duedate").toDate());
    _terms->setText(q.value("f_terms").toString());
    _total->setLocalValue(q.value("apopen_amount").toDouble());
    _discountAmount->setLocalValue(q.value("discount").toDouble());
    _amount->setLocalValue(q.value("f_amount").toDouble());
    if(q.value("bankaccnt_id").toInt() != -1)
      _bankaccnt->setId(q.value("bankaccnt_id").toInt());
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void selectPayment::sDiscount()
{
  ParameterList params;
  params.append("apopen_id", _apopenid);
  params.append("curr_id", _selected->id());
  if(_discountAmount->localValue() != 0.0)
    params.append("amount", _discountAmount->localValue());

  applyDiscount newdlg(this, "", TRUE);
  newdlg.set(params);

  if(newdlg.exec() != XDialog::Rejected)
  {
    _discountAmount->setLocalValue(newdlg._amount->localValue());
    if(_discountAmount->localValue() + _selected->localValue() > _amount->localValue())
      _selected->setLocalValue(_amount->localValue() - _discountAmount->localValue());
  }
}

void selectPayment::sPriceGroup()
{
  if (! omfgThis->singleCurrency())
    _priceGroup->setTitle(tr("In %1:").arg(_selected->currAbbr()));
}
