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

#include "bankAdjustmentType.h"

#include <qvariant.h>
#include <qmessagebox.h>

/*
 *  Constructs a bankAdjustmentType as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
bankAdjustmentType::bankAdjustmentType(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
    connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_name, SIGNAL(lostFocus()), this, SLOT(sCheck()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
bankAdjustmentType::~bankAdjustmentType()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void bankAdjustmentType::languageChange()
{
    retranslateUi(this);
}


void bankAdjustmentType::init()
{
}

enum SetResponse bankAdjustmentType::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("bankadjtype_id", &valid);
  if (valid)
  {
    _bankadjtypeid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
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
      _accnt->setEnabled(FALSE);
      _senseGroup->setEnabled(FALSE);
      _close->setText(tr("&Close"));
      _save->hide();
      _close->setFocus();
    }
  }

  return NoError;
}

void bankAdjustmentType::sSave()
{
  if (_name->text().length() == 0)
  {
    QMessageBox::information( this, tr("Cannot Save Adjustment Type"),
                              tr("You must enter a valid name for this Adjustment Type.") );
    _name->setFocus();
    return;
  }

  if (_accnt->id() == -1)
  {
    QMessageBox::information( this, tr("Cannot Save Adjustment Type"),
                              tr("You must select a valid account for this Adjustment Type.") );
    return;
  }
  
  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('bankadjtype_bankadjtype_id_seq') AS bankadjtype_id");
    if (q.first())
      _bankadjtypeid = q.value("bankadjtype_id").toInt();

    q.prepare( "INSERT INTO bankadjtype "
               "(bankadjtype_id, bankadjtype_name, bankadjtype_descrip, bankadjtype_accnt_id, bankadjtype_iscredit) "
               "VALUES "
               "(:bankadjtype_id, :bankadjtype_name, :bankadjtype_descrip, :bankadjtype_accnt_id, :bankadjtype_iscredit);" );
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE bankadjtype "
               "SET bankadjtype_name=:bankadjtype_name,"
               "    bankadjtype_descrip=:bankadjtype_descrip, "
               "    bankadjtype_accnt_id=:bankadjtype_accnt_id,"
               "    bankadjtype_iscredit=:bankadjtype_iscredit "
               "WHERE (bankadjtype_id=:bankadjtype_id);" );

  q.bindValue(":bankadjtype_id", _bankadjtypeid);
  q.bindValue(":bankadjtype_name", _name->text());
  q.bindValue(":bankadjtype_descrip", _description->text().stripWhiteSpace());
  q.bindValue(":bankadjtype_accnt_id", _accnt->id());
  q.bindValue(":bankadjtype_iscredit", QVariant(_credit->isChecked(),0));
  q.exec();

  done(_bankadjtypeid);
}

void bankAdjustmentType::sCheck()
{
  _name->setText(_name->text().stripWhiteSpace());
  if ((_mode == cNew) && (_name->text().length()))
  {
    q.prepare( "SELECT bankadjtype_id "
               "FROM bankadjtype "
               "WHERE (UPPER(bankadjtype_name)=UPPER(:bankadjtype_name));" );
    q.bindValue(":bankadjtype_name", _name->text());
    q.exec();
    if (q.first())
    {
      _bankadjtypeid = q.value("bankadjtype_id").toInt();
      _mode = cEdit;
      populate();

      _name->setEnabled(FALSE);
    }
  }
}

void bankAdjustmentType::populate()
{
  q.prepare( "SELECT bankadjtype_name, bankadjtype_descrip,"
             "       bankadjtype_accnt_id, bankadjtype_iscredit "
             "FROM bankadjtype "
             "WHERE (bankadjtype_id=:bankadjtype_id);" );
  q.bindValue(":bankadjtype_id", _bankadjtypeid);
  q.exec();
  if (q.first())
  {
    _name->setText(q.value("bankadjtype_name"));
    _description->setText(q.value("bankadjtype_descrip"));
    _accnt->setId(q.value("bankadjtype_accnt_id").toInt());
    if(q.value("bankadjtype_iscredit").toBool())
      _credit->setChecked(true);
  }
} 
