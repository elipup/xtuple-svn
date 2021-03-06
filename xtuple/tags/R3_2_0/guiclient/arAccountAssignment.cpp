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

#include "arAccountAssignment.h"

#include <QVariant>
#include <QMessageBox>

/*
 *  Constructs a arAccountAssignment as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
arAccountAssignment::arAccountAssignment(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);


  // signals and slots connections
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_selectedCustomerType, SIGNAL(toggled(bool)), _customerTypes, SLOT(setEnabled(bool)));
  connect(_customerTypePattern, SIGNAL(toggled(bool)), _customerType, SLOT(setEnabled(bool)));

  _customerTypes->setType(XComboBox::CustomerTypes);

  if(!_metrics->boolean("EnableCustomerDeposits"))
  {
    _deferred->hide();
    _deferredLit->hide();
  }
}

/*
 *  Destroys the object and frees any allocated resources
 */
arAccountAssignment::~arAccountAssignment()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void arAccountAssignment::languageChange()
{
  retranslateUi(this);
}

enum SetResponse arAccountAssignment::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("araccnt_id", &valid);
  if (valid)
  {
    _araccntid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _customerType->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
      _save->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _customerTypeGroup->setEnabled(FALSE);
      _ar->setReadOnly(TRUE);
      _prepaid->setReadOnly(TRUE);
      _freight->setReadOnly(TRUE);
      _deferred->setReadOnly(TRUE);
      _close->setText(tr("&Close"));
      _save->hide();

      _close->setFocus();
    }
  }

  return NoError;
}

void arAccountAssignment::sSave()
{
  if (!_ar->isValid())
  {
    QMessageBox::warning( this, tr("Cannot Save A/R Account Assignment"),
                          tr("You must select a A/R Account before saving this A/R Account Assignment") );
    _ar->setFocus();
    return;
  }

  if (!_prepaid->isValid())
  {
    QMessageBox::warning( this, tr("Cannot Save A/R Account Assignment"),
                          tr("You must select a Prepaid Receivables Account before saving this A/R Account Assignment") );
    _ar->setFocus();
    return;
  }

  if (!_freight->isValid())
  {
    QMessageBox::warning( this, tr("Cannot Save A/R Account Assignment"),
                          tr("You must select a Freight Account before saving this A/R Account Assignment") );
    _freight->setFocus();
    return;
  }

  if(_metrics->boolean("EnableCustomerDeposits") && !_deferred->isValid())
  {
    QMessageBox::warning( this, tr("Cannot Save A/R Account Assignment"),
                          tr("You must select a Deferred Revenue Account before saving this A/R Account Assignment") );
    _deferred->setFocus();
    return;
  }

  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('araccnt_araccnt_id_seq') AS _araccntid;");
    if (q.first())
      _araccntid = q.value("_araccntid").toInt();
//  ToDo

    q.prepare( "INSERT INTO araccnt "
               "( araccnt_id, araccnt_custtype_id, araccnt_custtype,"
               "  araccnt_ar_accnt_id, araccnt_prepaid_accnt_id, araccnt_freight_accnt_id,"
               "  araccnt_deferred_accnt_id ) "
               "VALUES "
               "( :araccnt_id, :araccnt_custtype_id, :araccnt_custtype,"
               "  :araccnt_ar_accnt_id, :araccnt_prepaid_accnt_id, :araccnt_freight_accnt_id,"
               "  :araccnt_deferred_accnt_id );" );
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE araccnt "
               "SET araccnt_custtype_id=:araccnt_custtype_id, araccnt_custtype=:araccnt_custtype,"
               "    araccnt_ar_accnt_id=:araccnt_ar_accnt_id,"
               "    araccnt_prepaid_accnt_id=:araccnt_prepaid_accnt_id,"
               "    araccnt_freight_accnt_id=:araccnt_freight_accnt_id,"
               "    araccnt_deferred_accnt_id=:araccnt_deferred_accnt_id "
               "WHERE (araccnt_id=:araccnt_id);" );

  q.bindValue(":araccnt_id", _araccntid);

  if (_selectedCustomerType->isChecked())
  {
    q.bindValue(":araccnt_custtype_id", _customerTypes->id());
    q.bindValue(":araccnt_custtype", "^[a-zA-Z0-9_]");
  }
  else if (_customerTypePattern->isChecked())
  {
    q.bindValue(":araccnt_custtype_id", -1);
    q.bindValue(":araccnt_custtype", _customerType->text());
  }

  q.bindValue(":araccnt_ar_accnt_id", _ar->id());
  q.bindValue(":araccnt_prepaid_accnt_id", _prepaid->id());
  q.bindValue(":araccnt_freight_accnt_id", _freight->id());
  q.bindValue(":araccnt_deferred_accnt_id", _deferred->id());
  q.exec();

  done(_araccntid);
}

void arAccountAssignment::populate()
{
  q.prepare( "SELECT araccnt_custtype_id, araccnt_custtype,"
             "       araccnt_ar_accnt_id, araccnt_prepaid_accnt_id,"
             "       araccnt_freight_accnt_id, araccnt_deferred_accnt_id "
             "FROM araccnt "
             "WHERE (araccnt_id=:araccnt_id);" );
  q.bindValue(":araccnt_id", _araccntid);
  q.exec();
  if (q.first())
  {
    if (q.value("araccnt_custtype_id").toInt() == -1)
    {
      _customerTypePattern->setChecked(TRUE);
      _customerType->setText(q.value("araccnt_custtype").toString());
    }
    else
    {
      _selectedCustomerType->setChecked(TRUE);
      _customerTypes->setId(q.value("araccnt_custtype_id").toInt());
    }

    _ar->setId(q.value("araccnt_ar_accnt_id").toInt());
    _prepaid->setId(q.value("araccnt_prepaid_accnt_id").toInt());
    _freight->setId(q.value("araccnt_freight_accnt_id").toInt());
    _deferred->setId(q.value("araccnt_deferred_accnt_id").toInt());
  }
}

