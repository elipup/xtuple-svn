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

#include "hotkeys.h"

#include <qvariant.h>
#include "hotkey.h"

/*
 *  Constructs a hotkeys as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
hotkeys::hotkeys(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
    connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
    connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
    connect(_hotkey, SIGNAL(populateMenu(Q3PopupMenu*,Q3ListViewItem*,int)), this, SLOT(sPopulateMenu(Q3PopupMenu*)));
    connect(_close, SIGNAL(clicked()), this, SLOT(sClose()));
    connect(_hotkey, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_hotkey, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_hotkey, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
hotkeys::~hotkeys()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void hotkeys::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <Q3PopupMenu>

void hotkeys::init()
{
  _dirty = FALSE;

  _hotkey->addColumn(tr("Keystroke"), _itemColumn, Qt::AlignLeft );
  _hotkey->addColumn(tr("Action"),    -1,          Qt::AlignLeft );
  _hotkey->addColumn("key",           0,           Qt::AlignLeft );

  sFillList();
}

enum SetResponse hotkeys::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("username", &valid);
  if (valid)
  {
    _username->setText(param.toString());
    _currentUser = FALSE;
  }

  param = pParams.value("currentUser", &valid);
  if (valid)
  {
    _username->setText(omfgThis->username());
    _currentUser = TRUE;
  }

  sFillList();

  return NoError;
}

void hotkeys::sFillList()
{
  _hotkey->clear();

  QString hotkey;
  QString action;
  char    key;

  if (_currentUser)
  {
    for (key = '1'; key <= '9'; key++)
    {
      hotkey = QString("F%1").arg(key);
      action = _preferences->value(hotkey);
      if (!action.isNull())
        new XListViewItem(_hotkey, _hotkey->lastItem(), -1, QVariant(tr("F%1").arg(key)), action, hotkey);
    }

    for (key = 'A'; key <= 'Z'; key++)
    {
      hotkey = QString("C%1").arg(key);
      action = _preferences->value(hotkey);
      if (!action.isNull())
        new XListViewItem(_hotkey, _hotkey->lastItem(), -1, QVariant(QString("Ctrl+%1").arg(key)), action, hotkey);
    }

    for (key = '0'; key <= '9'; key++)
    {
      hotkey = QString("C%1").arg(key);
      action = _preferences->value(hotkey);
      if (!action.isNull())
        new XListViewItem(_hotkey, _hotkey->lastItem(), -1, QVariant(QString("Ctrl+%1").arg(key)), action, hotkey);
    }
  }
  else
  {
    Preferences pref(_username->text());

    for (key = '1'; key <= '9'; key++)
    {
      hotkey = QString("F%1").arg(key);
      action = pref.value(hotkey);
      if (!action.isNull())
        new XListViewItem(_hotkey, _hotkey->lastItem(), -1, QVariant(tr("F%1").arg(key)), action, hotkey);
    }

    for (key = 'A'; key <= 'Z'; key++)
    {
      hotkey = QString("C%1").arg(key);
      action = pref.value(hotkey);
      if (!action.isNull())
        new XListViewItem(_hotkey, _hotkey->lastItem(), -1,QVariant( QString("Ctrl+%1").arg(key)), action, hotkey);
    }

    for (key = '0'; key <= '9'; key++)
    {
      hotkey = QString("C%1").arg(key);
      action = pref.value(hotkey);
      if (!action.isNull())
        new XListViewItem(_hotkey, _hotkey->lastItem(), -1, QVariant(QString("Ctrl+%1").arg(key)), action, hotkey);
    }
  }
}

void hotkeys::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  if (_currentUser)
    params.append("currentUser");
  else
    params.append("username", _username->text());

  hotkey newdlg(this, "", TRUE);
  newdlg.set(params);
  
  if (newdlg.exec() != QDialog::Rejected)
  {
    _dirty = TRUE;
    sFillList();
  }
}

void hotkeys::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("hotkey", _hotkey->currentItem()->text(2));

  if (_currentUser)
    params.append("currentUser");
  else
    params.append("username", _username->text());

  hotkey newdlg(this, "", TRUE);
  newdlg.set(params);
  
  if (newdlg.exec() != QDialog::Rejected)
  {
    _dirty = TRUE;
    sFillList();
  }
}

void hotkeys::sDelete()
{
  if (_currentUser)
  {
    _preferences->remove(_hotkey->currentItem()->text(2));
    _preferences->load();
  }
  else
  {
    q.prepare("SELECT deleteUserPreference(:username, :name) AS _result;");
    q.bindValue(":username", _username->text());
    q.bindValue(":name", _hotkey->currentItem()->text(2));
    q.exec();
  }

  _dirty = TRUE;
  sFillList();
}

void hotkeys::sPopulateMenu(Q3PopupMenu *)
{
}

void hotkeys::sClose()
{
  if (_dirty)
    accept();
  else
    reject();
}

