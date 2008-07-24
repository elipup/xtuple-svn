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

#include "expenseCategories.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include <qstatusbar.h>
#include <openreports.h>
#include "expenseCategory.h"

/*
 *  Constructs a expenseCategories as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
expenseCategories::expenseCategories(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
    connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
    connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
    connect(_copy, SIGNAL(clicked()), this, SLOT(sCopy()));
    connect(_expcat, SIGNAL(valid(bool)), _view, SLOT(setEnabled(bool)));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
expenseCategories::~expenseCategories()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void expenseCategories::languageChange()
{
    retranslateUi(this);
}


void expenseCategories::init()
{
  statusBar()->hide();
  
  _expcat->addColumn(tr("Category"),    _itemColumn, Qt::AlignLeft   );
  _expcat->addColumn(tr("Description"), -1,          Qt::AlignLeft   );

  if (_privileges->check("MaintainExpenseCategories"))
  {
    connect(_expcat, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_expcat, SIGNAL(valid(bool)), _copy, SLOT(setEnabled(bool)));
    connect(_expcat, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_expcat, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_expcat, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  sFillList();
}

void expenseCategories::sDelete()
{
  q.prepare( "SELECT poitem_status, COUNT(*) "
             "FROM poitem "
             "WHERE (poitem_expcat_id=:expcat_id) "
             "GROUP BY poitem_status;" );
  q.bindValue(":expcat_id", _expcat->id());
  q.exec();
  if (q.first())
  {
    if (q.findFirst("poitem_status", "U") != -1)
      QMessageBox::warning( this, tr("Cannot Delete Expense Category"),
                            tr( "<p>The selected Expense Category cannot be deleted as there are unposted P/O Lines assigned to it. "
                                "You must reassign these P/O Lines before you may delete the selected Expense Category.</p>" ) );
    else if (q.findFirst("poitem_status", "O") != -1)
      QMessageBox::warning( this, tr("Cannot Delete Expense Category"),
                            tr( "<p>The selected Expense Category cannot be deleted as there are open P/O Lines assigned to it. "
                                "You must close these P/O Lines before you may delete the selected Expense Category.</p>" ) );
    else if (q.findFirst("poitem_status", "C") != -1)
    {
      int result = QMessageBox::warning( this, tr("Cannot Delete Expense Category"),
                                         tr( "<p>The selected Expense Category cannot be deleted as there are closed P/O Lines assigned to it. "
                                             "Would you like to mark the selected Expense Category as inactive instead?</p>" ),
                                         tr("&Yes"), tr("&No"), QString::null );
      if (result == 0)
      {
        q.prepare( "UPDATE expcat "
                   "SET expcat_active=FALSE "
                   "WHERE (expcat_id=:expcat_id);" );
        q.bindValue(":expcat_id", _expcat->id());
        q.exec();
	sFillList();
      }
    }
  }
  else
  {
    q.prepare( "DELETE FROM expcat "
               "WHERE (expcat_id=:expcat_id); ");
    q.bindValue(":expcat_id", _expcat->id());
    q.exec();
    sFillList();
  }
}

void expenseCategories::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  expenseCategory newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void expenseCategories::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("expcat_id", _expcat->id());

  expenseCategory newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void expenseCategories::sCopy()
{
  ParameterList params;
  params.append("mode", "copy");
  params.append("expcat_id", _expcat->id());

  expenseCategory newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void expenseCategories::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("expcat_id", _expcat->id());

  expenseCategory newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void expenseCategories::sPrint()
{
  orReport report("ExpenseCategoriesMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void expenseCategories::sFillList()
{
  _expcat->populate( "SELECT expcat_id, expcat_code, expcat_descrip "
                     "FROM expcat "
                    "ORDER BY expcat_code;" );
}

