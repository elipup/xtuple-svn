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

#include <QVariant>

#include "imageAssignment.h"
#include "imageview.h"

/*
 *  Constructs a imageAssignment as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
imageAssignment::imageAssignment(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : QDialog(parent, name, modal, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_image, SIGNAL(valid(bool)), _save, SLOT(setEnabled(bool)));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));

  _image->addColumn(tr("Name"),        _itemColumn, Qt::AlignLeft, true, "image_name" );
  _image->addColumn(tr("Description"), -1,          Qt::AlignLeft, true, "image_descrip" );

  _imageassid = -1;
  _mode = cNew;
  _source = Documents::Uninitialized;
  _sourceid = -1;

  sFillList();
}

/*
 *  Destroys the object and frees any allocated resources
 */
imageAssignment::~imageAssignment()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void imageAssignment::languageChange()
{
  retranslateUi(this);
}

void imageAssignment::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("sourceType", &valid);
  if (valid)
  {
    _source = (enum Documents::DocumentSources)param.toInt();
    if (_source != Documents::Item)
    {
      _purpose->setCurrentIndex(3);
      _purpose->hide();
      _purposeLit->hide();
    }
  }

  param = pParams.value("source_id", &valid);
  if (valid)
    _sourceid = param.toInt();

  param = pParams.value("imageass_id", &valid);
  if (valid)
  {
    _imageassid = param.toInt();
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
      _purpose->setEnabled(FALSE);
    }
  }
}

void imageAssignment::sSave()
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
    _imageassid = -1;

    if (purpose != "M")
    {
      XSqlQuery imageassid;
      imageassid.prepare( "SELECT imageass_id "
                           "FROM imageass "
                           "WHERE ( (imageass_source_id=:source_id)"
                           " AND ( imageass_source=:source) "
                           " AND (imageass_purpose=:imageass_purpose) );" );
      imageassid.bindValue(":source", Documents::_documentMap[_source].ident);
      imageassid.bindValue(":source_id", _sourceid);
      imageassid.bindValue(":imageass_purpose", purpose);
      imageassid.exec();
      if (imageassid.first())
      {
        _imageassid = imageassid.value("imageass_id").toInt();
        _mode = cEdit;
      }
    }
      
    if (_imageassid == -1)
    {
      XSqlQuery imageassid;
      imageassid.prepare("SELECT NEXTVAL('imageass_imageass_id_seq') AS _imageass_id;");
      imageassid.exec();
      if (imageassid.first())
        _imageassid = imageassid.value("_imageass_id").toInt();
    }

    newImage.prepare( "INSERT INTO imageass "
                      "( imageass_id, imageass_source, imageass_source_id, imageass_purpose, imageass_image_id ) "
                      "VALUES "
                      "( :imageass_id, :imageass_source, :imageass_source_id, :imageass_purpose, :imageass_image_id );" );
  }

  if (_mode == cEdit)
    newImage.prepare( "UPDATE imageass "
                      "SET imageass_purpose=:imageass_purpose, imageass_image_id=:imageass_image_id "
                      "WHERE (imageass_id=:imageass_id);" );

  newImage.bindValue(":imageass_id", _imageassid);
  newImage.bindValue(":imageass_source", Documents::_documentMap[_source].ident);
  newImage.bindValue(":imageass_source_id", _sourceid);
  newImage.bindValue(":imageass_image_id", _image->id());
  newImage.bindValue(":imageass_purpose", purpose);

  newImage.exec();

  done(_imageassid);
}

void imageAssignment::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  imageview newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != QDialog::Rejected)
    sFillList();
}

void imageAssignment::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("image_id", _image->id());

  imageview newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void imageAssignment::sFillList()
{
  _image->populate( "SELECT image_id, image_name, firstLine(image_descrip) AS image_descrip "
                    "FROM image "
                    "ORDER BY image_name;" );
}

void imageAssignment::populate()
{
  XSqlQuery imageass;
  imageass.prepare( "SELECT imageass_purpose, imageass_image_id, imageass_source "
                     "FROM imageass "
                     "WHERE (imageass_id=:imageass_id);" );
  imageass.bindValue(":imageass_id", _imageassid);
  imageass.exec();
  if (imageass.first())
  {
    if (imageass.value("imageass_source").toString() == "I")
    {
      QString purpose = imageass.value("imageass_purpose").toString();

      if (purpose == "I")
        _purpose->setCurrentItem(0);
      else if (purpose == "P")
        _purpose->setCurrentItem(1);
      else if (purpose == "E")
        _purpose->setCurrentItem(2);
      else if (purpose == "M")
        _purpose->setCurrentItem(3);
    }
    else
    {
      _purpose->setCurrentIndex(3);
      _purpose->hide();
      _purposeLit->hide();
    }

    _image->setId(imageass.value("imageass_image_id").toInt());
  }
}

