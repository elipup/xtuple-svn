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

#include "deleteWoMaterialItem.h"

#include <QVariant>
#include <QMessageBox>
#include "returnWoMaterialItem.h"

/*
 *  Constructs a deleteWoMaterialItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
deleteWoMaterialItem::deleteWoMaterialItem(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);


  // signals and slots connections
  connect(_womatl, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_wo, SIGNAL(newId(int)), _womatl, SLOT(setWoid(int)));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));

  _captive = FALSE;

  _wo->setType(cWoExploded | cWoIssued | cWoReleased);
}

/*
 *  Destroys the object and frees any allocated resources
 */
deleteWoMaterialItem::~deleteWoMaterialItem()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void deleteWoMaterialItem::languageChange()
{
    retranslateUi(this);
}

enum SetResponse deleteWoMaterialItem::set(const ParameterList &pParams)
{
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("womatl_id", &valid);
  if (valid)
    _womatl->setId(param.toInt());

  return NoError;
}

void deleteWoMaterialItem::sDelete()
{
  if (_womatl->qtyIssued() > 0)
  {
    if(_privleges->check("ReturnWoMaterials"))
    {
      if (  QMessageBox::critical(  this, tr("W/O Material Requirement cannot be Deleted"),
                                    tr( "This W/O Material Requirement cannot be deleted as it has has material issued to it.\n"
                                        "You must return this material to stock before you can delete this Material Requirement.\n"
                                        "Would you like to return this material to stock now?"  ),
                                    tr("&Yes"), tr("&No"), 0, 0, 1 ) == 0)
      {
        ParameterList params;
        params.append("womatl_id", _womatl->id());
  
        returnWoMaterialItem newdlg(omfgThis, "", TRUE);
        newdlg.set(params);
  
        if (newdlg.exec())
          _womatl->setId(_womatl->id());
  
        if (_womatl->qtyIssued() > 0)
          return;
      }
      else
        return;
    }
    else
    {
      QMessageBox::critical( this, tr("W/O Material Requirement cannot be Deleted"),
                             tr( "This W/O Material Requirement cannot be deleted as it has material issued to it.\n"
                                 "You must return this material to stock before you can delete this Material Requirement.\n" ) );
      return;
    }
  }

  q.prepare("SELECT deleteWoMaterial(:womatl_id);");
  q.bindValue(":womatl_id", _womatl->id());
  q.exec();

  _womatl->sRefresh();
}
