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

#include "incidentSeverities.h"

#include <QMenu>
#include <QVariant>
#include <QMessageBox>
#include <QSqlError>
//#include <QStatusBar>
#include <openreports.h>
#include "incidentSeverity.h"

/*
 *  Constructs an incidentSeverities as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
incidentSeverities::incidentSeverities(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  // signals and slots connections
  connect(_close,  SIGNAL(clicked()), this, SLOT(close()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_edit,   SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_incidentSeverities, SIGNAL(populateMenu(QMenu *, QTreeWidgetItem *, int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_incidentSeverities, SIGNAL(valid(bool)), _view, SLOT(setEnabled(bool)));
  connect(_new,   SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_view,  SIGNAL(clicked()), this, SLOT(sView()));

//  statusBar()->hide();
  
  _incidentSeverities->addColumn(tr("Order"),  _seqColumn, Qt::AlignRight, true, "incdtseverity_order");
  _incidentSeverities->addColumn(tr("Severity"),      100, Qt::AlignLeft, true, "incdtseverity_name" );
  _incidentSeverities->addColumn(tr("Description"),    -1, Qt::AlignLeft, true, "incdtseverity_descrip" );

  if (_privileges->check("MaintainIncidentSeverities"))
  {
    connect(_incidentSeverities, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_incidentSeverities, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_incidentSeverities, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_incidentSeverities, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  sFillList();
}

incidentSeverities::~incidentSeverities()
{
    // no need to delete child widgets, Qt does it all for us
}

void incidentSeverities::languageChange()
{
    retranslateUi(this);
}

void incidentSeverities::sFillList()
{
  q.prepare( "SELECT incdtseverity_id, incdtseverity_order, "
	     "       incdtseverity_name, firstLine(incdtseverity_descrip) AS incdtseverity_descrip "
             "FROM incdtseverity "
             "ORDER BY incdtseverity_order, incdtseverity_name;" );
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  _incidentSeverities->populate(q);
}

void incidentSeverities::sDelete()
{
  q.prepare( "DELETE FROM incdtseverity "
             "WHERE (incdtseverity_id=:incdtseverity_id);" );
  q.bindValue(":incdtseverity_id", _incidentSeverities->id());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void incidentSeverities::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  incidentSeverity newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void incidentSeverities::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("incdtseverity_id", _incidentSeverities->id());

  incidentSeverity newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void incidentSeverities::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("incdtseverity_id", _incidentSeverities->id());

  incidentSeverity newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void incidentSeverities::sPopulateMenu( QMenu *pMenu)
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEdit()), 0);
  if (!_privileges->check("MaintainIncidentSeverities"))
    pMenu->setItemEnabled(menuItem, FALSE);

  pMenu->insertItem(tr("View..."), this, SLOT(sView()), 0);

  menuItem = pMenu->insertItem(tr("Delete"), this, SLOT(sDelete()), 0);
  if (!_privileges->check("MaintainIncidentSeverities"))
    pMenu->setItemEnabled(menuItem, FALSE);
}

void incidentSeverities::sPrint()
{
  orReport report("IncidentSeveritiesList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}
