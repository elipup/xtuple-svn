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

#include "deletePlannedOrdersByPlannerCode.h"

#include <qvariant.h>
#include <qmessagebox.h>

/*
 *  Constructs a deletePlannedOrdersByPlannerCode as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
deletePlannedOrdersByPlannerCode::deletePlannedOrdersByPlannerCode(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
deletePlannedOrdersByPlannerCode::~deletePlannedOrdersByPlannerCode()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void deletePlannedOrdersByPlannerCode::languageChange()
{
    retranslateUi(this);
}


void deletePlannedOrdersByPlannerCode::init()
{
  _captive = FALSE;

  _plannerCode->setType(PlannerCode);
}

void deletePlannedOrdersByPlannerCode::sDelete()
{
  if (!_cutoffDate->isValid())
  {
    QMessageBox::critical( this, tr("Enter Cut Off Date"),
                           tr( "You must enter a cut off date for the Planned Orders\n"
                               "to be deleted." ) );
    _cutoffDate->setFocus();
    return;
  }

  if (!(_both->isChecked() || _mps->isChecked() || _mrp->isChecked()))
  {
    QMessageBox::critical( this, tr("Select Planning System"),
                           tr("You must select which Planning System(s) to delete from.") );
    return;
  }

  QString sql = "SELECT deletePlannedOrder(planord_id, :deleteChildren) "
                "FROM planord, itemsite "
                "WHERE ( (planord_itemsite_id=itemsite_id)"
                " AND (planord_startdate<=:cutOffDate)";

  if (!_deleteFirmed->isChecked())
    sql += " AND (NOT planord_firm)";

  if (_plannerCode->isSelected())
    sql += " AND (itemsite_plancode_id=:plancode_id)";
  else if (_plannerCode->isPattern())
    sql + " AND (itemsite_plancode_id IN (SELECT plancode_id FROM plancode WHERE (plancode_code ~ :plancode_pattern)))";

  if (_warehouse->isSelected())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  if (_mps->isChecked())
    sql += " AND (planord_mps)";
  else if (_mrp->isChecked())
    sql += " AND (NOT planord_mps)";
  
  sql += ");";

  q.prepare(sql);
  q.bindValue(":deleteChildren", QVariant(_deleteChildren->isChecked(), 0));
  q.bindValue(":cutOffDate", _cutoffDate->date());
  _warehouse->bindValue(q);
  _plannerCode->bindValue(q);
  q.exec();

  accept();
}

