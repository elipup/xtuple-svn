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

#include "glSeriesItem.h"

#include <math.h>

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

glSeriesItem::glSeriesItem(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);

    connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));

    _standardJournalItem = false;
}

glSeriesItem::~glSeriesItem()
{
    // no need to delete child widgets, Qt does it all for us
}

void glSeriesItem::languageChange()
{
    retranslateUi(this);
}

enum SetResponse glSeriesItem::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("glseries_id", &valid);
  if (valid)
  {
    _glseriesid = param.toInt();
    populate();
  }

  param = pParams.value("glSequence", &valid);
  if (valid)
    _glsequence = param.toInt();

  param = pParams.value("distDate", &valid);
  if (valid)
    _amount->setEffective(param.toDate());

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
      _mode = cNew;
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      _save->setFocus();
    }
  }

  _standardJournalItem = pParams.inList("postStandardJournal");

  return NoError;
}

void glSeriesItem::sSave()
{
  if (! _amount->isBase() &&
      QMessageBox::question(this, tr("G/L Transaction Not In Base Currency"),
		          tr("G/L transactions are recorded in the base currency.\n"
			  "Do you wish to convert %1 %2 at the rate effective on %3?")
			  .arg(_amount->localValue()).arg(_amount->currAbbr())
			  .arg(_amount->effective().toString(Qt::LocalDate)),
			  QMessageBox::Yes|QMessageBox::Escape,
			  QMessageBox::No |QMessageBox::Default) != QMessageBox::Yes)
  {
	_amount->setFocus();
	return;
  }

  double amount = _amount->baseValue();
  if (_debit->isChecked())
    amount *= -1;

  if (_mode == cNew)
    q.prepare("SELECT insertIntoGLSeries(:glsequence, 'G/L', :doctype, '', :accnt_id, :amount, CURRENT_DATE) AS result;");
  else if (_mode == cEdit)
    q.prepare( "UPDATE glseries "
               "SET glseries_accnt_id=:accnt_id,"
	       "    glseries_amount=:amount "
               "WHERE (glseries_id=:glseries_id);" );

  q.bindValue(":glseries_id",	_glseriesid);
  q.bindValue(":glsequence",	_glsequence);
  q.bindValue(":accnt_id",	_account->id());
  q.bindValue(":amount",	amount);
  if (_standardJournalItem)
    q.bindValue(":doctype", "ST");
  else
    q.bindValue(":doctype", "");
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  accept();
}

void glSeriesItem::populate()
{
  q.prepare( "SELECT glseries_amount, glseries_accnt_id "
             "FROM glseries "
             "WHERE (glseries_id=:glseries_id);" );
  q.bindValue(":glseries_id", _glseriesid);
  q.exec();
  if (q.first())
  {
    if (q.value("glseries_amount").toDouble() < 0)
    {
      _debit->setChecked(TRUE);
      _amount->setBaseValue(fabs(q.value("glseries_amount").toDouble()));
    }
    else
    {
      _credit->setChecked(TRUE);
      _amount->setBaseValue(q.value("glseries_amount").toDouble());
    }

    _account->setId(q.value("glseries_accnt_id").toInt());
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
