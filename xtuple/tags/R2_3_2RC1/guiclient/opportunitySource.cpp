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

#include "opportunitySource.h"

#include <QVariant>
#include <QMessageBox>

/*
 *  Constructs a opportunitySource as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
opportunitySource::opportunitySource(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_name, SIGNAL(lostFocus()), this, SLOT(sCheck()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
opportunitySource::~opportunitySource()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void opportunitySource::languageChange()
{
  retranslateUi(this);
}

enum SetResponse opportunitySource::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("opsource_id", &valid);
  if (valid)
  {
    _opsourceid = param.toInt();
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
      _name->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;
      _name->setEnabled(FALSE);
      _description->setEnabled(FALSE);
      _save->hide();
      _close->setText(tr("&Close"));
      _close->setFocus();
    }
  }

  return NoError;
}

void opportunitySource::sCheck()
{
  _name->setText(_name->text().stripWhiteSpace());
  if ((_mode == cNew) && (_name->text().length() != 0))
  {
    q.prepare( "SELECT opsource_id "
               "FROM opsource "
               "WHERE (UPPER(opsource_name)=UPPER(:opsource_name));" );
    q.bindValue(":opsource_name", _name->text());
    q.exec();
    if (q.first())
    {
      _opsourceid = q.value("opsource_id").toInt();
      _mode = cEdit;
      populate();

      _name->setEnabled(FALSE);
    }
  }
}

void opportunitySource::sSave()
{
  _name->setText(_name->text().stripWhiteSpace().upper());
  if (_name->text().length() == 0)
  {
    QMessageBox::information( this, tr("Invalid Name"),
                              tr("You must enter a valid Name for this Opportunity Source.") );
    _name->setFocus();
    return;
  }

  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('opsource_opsource_id_seq') AS opsource_id");
    if (q.first())
      _opsourceid = q.value("opsource_id").toInt();
    else
    {
      systemError(this, tr("A System Error occurred at %1::%2.")
                        .arg(__FILE__)
                        .arg(__LINE__) );
      return;
    }

    q.prepare( "INSERT INTO opsource "
               "( opsource_id, opsource_name, opsource_descrip)"
               "VALUES "
               "( :opsource_id, :opsource_name, :opsource_descrip );" );
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE opsource "
               "   SET opsource_name=:opsource_name,"
               "       opsource_descrip=:opsource_descrip "
               " WHERE(opsource_id=:opsource_id);" );

  q.bindValue(":opsource_id", _opsourceid);
  q.bindValue(":opsource_name", _name->text());
  q.bindValue(":opsource_descrip", _description->text().stripWhiteSpace());
  q.exec();

  done(_opsourceid);
}

void opportunitySource::populate()
{
  q.prepare( "SELECT opsource_name, opsource_descrip "
             "  FROM opsource "
             " WHERE(opsource_id=:opsource_id);" );
  q.bindValue(":opsource_id", _opsourceid);
  q.exec();
  if (q.first())
  {
    _name->setText(q.value("opsource_name"));
    _description->setText(q.value("opsource_descrip"));
  }
} 

