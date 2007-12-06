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

#include "itemImage.h"

#include <QVariant>
#include "image.h"

/*
 *  Constructs a itemImage as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
itemImage::itemImage(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : QDialog(parent, name, modal, fl)
{
  setupUi(this);


  // signals and slots connections
  connect(_image, SIGNAL(valid(bool)), _save, SLOT(setEnabled(bool)));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));

  _image->addColumn(tr("Name"),        _itemColumn, Qt::AlignLeft );
  _image->addColumn(tr("Description"), -1,          Qt::AlignLeft );

  sFillList();
}

/*
 *  Destroys the object and frees any allocated resources
 */
itemImage::~itemImage()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void itemImage::languageChange()
{
  retranslateUi(this);
}

enum SetResponse itemImage::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
    _itemid = param.toInt();

  param = pParams.value("itemimage_id", &valid);
  if (valid)
  {
    _itemimageid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _purpose->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
      _save->setFocus();
    }
  }

  return NoError;
}

void itemImage::sSave()
{
  QString purpose;
  if (_purpose->currentItem() == 0)  
    purpose = "I";
  if (_purpose->currentItem() == 1)  
    purpose = "P";
  if (_purpose->currentItem() == 2)  
    purpose = "E";
  if (_purpose->currentItem() == 3)  
    purpose = "M";

  XSqlQuery newImage;

  if (_mode == cNew)
  {
    _itemimageid = -1;

    if (purpose != "M")
    {
      XSqlQuery itemimageid;
      itemimageid.prepare( "SELECT itemimage_id "
                           "FROM itemimage "
                           "WHERE ( (itemimage_item_id=:item_id)"
                           " AND (itemimage_purpose=:itemimage_purpose) );" );
      itemimageid.bindValue(":item_id", _itemid);
      itemimageid.bindValue(":itemimage_purpose", purpose);
      itemimageid.exec();
      if (itemimageid.first())
      {
        _itemimageid = itemimageid.value("itemimage_id").toInt();
        _mode = cEdit;
      }
    }
      
    if (_itemimageid == -1)
    {
      XSqlQuery itemimageid;
      itemimageid.prepare("SELECT NEXTVAL('itemimage_itemimage_id_seq') AS _itemimage_id;");
      itemimageid.exec();
      if (itemimageid.first())
        _itemimageid = itemimageid.value("_itemimage_id").toInt();
//  ToDo
    }

    newImage.prepare( "INSERT INTO itemimage "
                      "( itemimage_id, itemimage_item_id, itemimage_purpose, itemimage_image_id ) "
                      "VALUES "
                      "( :itemimage_id, :itemimage_item_id, :itemimage_purpose, :itemimage_image_id );" );
  }

  if (_mode == cEdit)
    newImage.prepare( "UPDATE itemimage "
                      "SET itemimage_purpose=:itemimage_purpose, itemimage_image_id=:itemimage_image_id "
                      "WHERE (itemimage_id=:itemimage_id);" );

  newImage.bindValue(":itemimage_id", _itemimageid);
  newImage.bindValue(":itemimage_item_id", _itemid);
  newImage.bindValue(":itemimage_image_id", _image->id());
  newImage.bindValue(":itemimage_purpose", purpose);

  newImage.exec();

  done(_itemimageid);
}

void itemImage::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  image newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != QDialog::Rejected)
    sFillList();
}

void itemImage::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("image_id", _image->id());

  image newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void itemImage::sFillList()
{
  _image->populate( "SELECT image_id, image_name, firstLine(image_descrip) "
                    "FROM image "
                    "ORDER BY image_name;" );
}

void itemImage::populate()
{
  XSqlQuery itemimage;
  itemimage.prepare( "SELECT itemimage_purpose, itemimage_image_id "
                     "FROM itemimage "
                     "WHERE (itemimage_id=:itemimage_id);" );
  itemimage.bindValue(":itemimage_id", _itemimageid);
  itemimage.exec();
  if (itemimage.first())
  {
    QString purpose = itemimage.value("itemimage_purpose").toString();

    if (purpose == "I")
      _purpose->setCurrentItem(0);
    else if (purpose == "P")
      _purpose->setCurrentItem(1);
    else if (purpose == "E")
      _purpose->setCurrentItem(2);
    else if (purpose == "M")
      _purpose->setCurrentItem(3);

    _image->setId(itemimage.value("itemimage_image_id").toInt());
  }
}

