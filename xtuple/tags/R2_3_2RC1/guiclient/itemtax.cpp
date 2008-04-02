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

#include "itemtax.h"

#include <QMessageBox>
#include <QVariant>

/*
 *  Constructs a itemtax as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
itemtax::itemtax(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));

  _itemtaxid = -1;
  _itemid = -1;
}

/*
 *  Destroys the object and frees any allocated resources
 */
itemtax::~itemtax()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void itemtax::languageChange()
{
  retranslateUi(this);
}

enum SetResponse itemtax::set(const ParameterList & pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
    _itemid = param.toInt();

  param = pParams.value("itemtax_id", &valid);
  if (valid)
  {
    _itemtaxid = param.toInt();
    q.prepare("SELECT itemtax_item_id,"
              "       COALESCE(itemtax_taxauth_id,-1) AS taxauth_id,"
              "       itemtax_taxtype_id"
              "  FROM itemtax"
              " WHERE (itemtax_id=:itemtax_id);");
    q.bindValue(":itemtax_id", _itemtaxid);
    q.exec();
    if(q.first())
    {
      _itemid = q.value("itemtax_item_id").toInt();
      _taxauth->setId(q.value("taxauth_id").toInt());
      _taxtype->setId(q.value("itemtax_taxtype_id").toInt());
    }
    // TODO: catch any possible errors
  }

  param = pParams.value("mode", &valid);
  if(valid)
  {
    if(param.toString() == "new")
      _mode = cNew;
    else if(param.toString() == "edit")
      _mode = cEdit;
    else if(param.toString() == "view")
    {
      _mode = cView;
      _save->hide();
      _taxauth->setEnabled(false);
      _taxtype->setEnabled(false);
    }
  }

  return NoError;
}

void itemtax::sSave()
{
  q.prepare("SELECT itemtax_id"
            "  FROM itemtax"
            " WHERE ((itemtax_taxauth_id=:taxauth_id)"
            "   AND  (itemtax_item_id=:item_id)"
            "   AND  (itemtax_id != :itemtax_id))");
  q.bindValue(":item_id", _itemid);
  q.bindValue(":itemtax_id", _itemtaxid);
  if(_taxauth->isValid())
    q.bindValue(":taxauth_id", _taxauth->id());
  q.exec();
  if(q.first())
  {
    QMessageBox::warning(this, tr("Tax Authority Already Exists"),
                      tr("The Tax Authority you have choosen already exists for this item."));
    return;
  }

  if(cNew == _mode)
    q.prepare("INSERT INTO itemtax"
              "      (itemtax_item_id, itemtax_taxauth_id, itemtax_taxtype_id) "
              "VALUES(:item_id, :taxauth_id, :taxtype_id)");
  else if(cEdit == _mode)
    q.prepare("UPDATE itemtax"
              "   SET itemtax_taxauth_id=:taxauth_id,"
              "       itemtax_taxtype_id=:taxtype_id"
              " WHERE (itemtax_id=:itemtax_id);");

  q.bindValue(":item_id", _itemid);
  q.bindValue(":itemtax_id", _itemtaxid);
  if(_taxauth->isValid())
    q.bindValue(":taxauth_id", _taxauth->id());
  q.bindValue(":taxtype_id", _taxtype->id());
  q.exec();

  accept();
}

