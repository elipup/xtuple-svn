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

#include "uiforms.h"

#include <QVariant>
#include <QMessageBox>
#include <QStatusBar>
#include <QBuffer>
#include <QUiLoader>
#include <parameter.h>
#include "uiform.h"
#include "guiclient.h"

/*
 *  Constructs a uiforms as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
uiforms::uiforms(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  (void)statusBar();

  // signals and slots connections
  connect(_uiform, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
  connect(_uiform, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  connect(_uiform, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_test, SIGNAL(clicked()), this, SLOT(sTest()));

  statusBar()->hide();
  
  _uiform->addColumn(tr("Name"),        _itemColumn, Qt::AlignLeft );
  _uiform->addColumn(tr("Description"), -1,          Qt::AlignLeft );
  _uiform->addColumn(tr("Order"),       _ynColumn,   Qt::AlignCenter );
  _uiform->addColumn(tr("Enabled"),     _ynColumn,   Qt::AlignCenter );

  sFillList();
}

/*
 *  Destroys the object and frees any allocated resources
 */
uiforms::~uiforms()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void uiforms::languageChange()
{
  retranslateUi(this);
}

void uiforms::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  uiform newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void uiforms::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("uiform_id", _uiform->id());

  uiform newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void uiforms::sDelete()
{
  q.prepare( "DELETE FROM uiform "
             "WHERE (uiform_id=:uiform_id);" );
  q.bindValue(":uiform_id", _uiform->id());
  q.exec();

  sFillList();
}

void uiforms::sFillList()
{
  _uiform->populate( "SELECT uiform_id, uiform_name, uiform_notes, uiform_order, formatBoolYN(uiform_enabled) "
                   "  FROM uiform "
                   " ORDER BY uiform_name, uiform_order, uiform_id;" );
}

void uiforms::sTest()
{
  q.prepare("SELECT *"
            "  FROM uiform "
            " WHERE(uiform_id=:uiform_id);");
  q.bindValue(":uiform_id", _uiform->id());
  q.exec();
  if(!q.first())
    return;
  
  XMainWindow * wnd = new XMainWindow();
  wnd->setObjectName(q.value("uiform_name").toString());

  QUiLoader loader;
  QByteArray ba = q.value("uiform_source").toByteArray();
  QBuffer uiFile(&ba);
  if(!uiFile.open(QIODevice::ReadOnly))
  {
    QMessageBox::critical(this, tr("Could not load file"),
        tr("There was an error loading the UI Form from the database."));
    return;
  }
  QWidget *ui = loader.load(&uiFile);
  wnd->setWindowTitle(ui->windowTitle());
  uiFile.close();
  wnd->setCentralWidget(ui);

  omfgThis->handleNewWindow(wnd);
}
