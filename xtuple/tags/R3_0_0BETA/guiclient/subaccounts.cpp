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

#include "subaccounts.h"

#include <QVariant>
#include <QMessageBox>
#include <QStatusBar>
#include <parameter.h>
#include <QWorkspace>
#include "subaccount.h"

/*
 *  Constructs a subaccounts as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
subaccounts::subaccounts(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
    connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
    connect(_subaccnt, SIGNAL(populateMenu(QMenu *, QTreeWidgetItem *, int)), this, SLOT(sPopulateMenu(QMenu*)));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
    connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
    connect(_subaccnt, SIGNAL(valid(bool)), _view, SLOT(setEnabled(bool)));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
subaccounts::~subaccounts()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void subaccounts::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <QMenu>

void subaccounts::init()
{
  statusBar()->hide();
  
  if (_privileges->check("MaintainChartOfAccounts"))
  {
    connect(_subaccnt, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_subaccnt, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_subaccnt, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_subaccnt, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  _subaccnt->addColumn(tr("Number"),      _itemColumn,  Qt::AlignCenter );
  _subaccnt->addColumn(tr("Description"), -1,           Qt::AlignLeft   );

  sFillList();
}

void subaccounts::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  subaccount *newdlg = new subaccount(this, "", TRUE);
  newdlg->set(params);
  if (newdlg->exec() != XDialog::Rejected)
    sFillList();
}

void subaccounts::sEdit()
{
  ParameterList params;
  params.append("subaccnt_id", _subaccnt->id());
  params.append("mode", "edit");

  subaccount *newdlg = new subaccount(this, "", TRUE);
  newdlg->set(params);
  if (newdlg->exec() != XDialog::Rejected)
    sFillList();
}

void subaccounts::sView()
{
  ParameterList params;
  params.append("subaccnt_id", _subaccnt->id());
  params.append("mode", "view");

  subaccount *newdlg = new subaccount(this, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
}

void subaccounts::sDelete()
{
  q.prepare( "SELECT accnt_id "
             "FROM accnt "
             "WHERE (accnt_sub=:subaccnt);" );
  q.bindValue(":subaccnt", _subaccnt->currentItem()->text(0));
  q.exec();
  if (q.first())
  {
    QMessageBox::warning( this, tr("Cannot Delete Subaccount"),
                          tr( "The selected Subaccount cannot be deleted as it is in use by existing Account.\n"
                              "You must reclass these Accounts before you may delete the selected Subaccount." ) );
    return;
  }

  q.prepare( "DELETE FROM subaccnt "
             "WHERE (subaccnt_id=:subaccnt_id);" );
  q.bindValue(":subaccnt_id", _subaccnt->id());
  q.exec();

  sFillList();
}

void subaccounts::sPopulateMenu(QMenu *pMenu)
{
  int menuItem;

  pMenu->insertItem("View...", this, SLOT(sView()), 0);

  menuItem = pMenu->insertItem("Edit...", this, SLOT(sEdit()), 0);
  if (!_privileges->check("MaintainChartOfAccounts"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem("Delete...", this, SLOT(sDelete()), 0);
  if (!_privileges->check("MaintainChartOfAccounts"))
    pMenu->setItemEnabled(menuItem, FALSE);
}

void subaccounts::sFillList()
{
  q.prepare( "SELECT subaccnt_id,"
             "       subaccnt_number, subaccnt_descrip "
             "FROM subaccnt "
             "ORDER BY subaccnt_number;" );
  q.exec();
  _subaccnt->populate(q);
}

