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

#include "scripts.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <parameter.h>
#include "scriptEditor.h"
#include "guiclient.h"

scripts::scripts(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));

  _script->addColumn(tr("Name"),   _itemColumn, Qt::AlignLeft,  true, "script_name");
  _script->addColumn(tr("Description"),     -1, Qt::AlignLeft,  true, "script_notes");
  _script->addColumn(tr("Order"),    _ynColumn, Qt::AlignCenter,true, "script_order");
  _script->addColumn(tr("Enabled"),  _ynColumn, Qt::AlignCenter,true, "script_enabled");
  _script->addColumn(tr("Package"),_itemColumn, Qt::AlignLeft,  false,"nspname");

  sFillList();
}

scripts::~scripts()
{
  // no need to delete child widgets, Qt does it all for us
}

void scripts::languageChange()
{
  retranslateUi(this);
}

void scripts::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  scriptEditor newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void scripts::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("script_id", _script->id());

  scriptEditor newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void scripts::sDelete()
{
  if ( QMessageBox::warning(this, tr("Delete Script?"),
                            tr("<p>Are you sure that you want to completely "
			       "delete the selected script?"),
			    QMessageBox::Yes,
			    QMessageBox::No | QMessageBox::Default) == QMessageBox::Yes)
  {
    q.prepare( "DELETE FROM script "
               "WHERE (script_id=:script_id);" );
    q.bindValue(":script_id", _script->id());
    q.exec();
    if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    sFillList();
  }
}

void scripts::sFillList()
{
  q.exec("SELECT script.*,"
         "       CASE WHEN nspname='public' THEN ''"
         "            ELSE nspname END AS nspname"
         "  FROM script, pg_class, pg_namespace"
         "  WHERE ((script.tableoid=pg_class.oid)"
         "    AND  (relnamespace=pg_namespace.oid))"
         " ORDER BY script_name, script_order, script_id;" );
  _script->populate(q);
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
