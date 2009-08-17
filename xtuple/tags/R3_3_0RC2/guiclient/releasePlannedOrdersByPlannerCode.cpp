/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "releasePlannedOrdersByPlannerCode.h"

#include <QVariant>
#include <QMessageBox>

#include "submitAction.h"

/*
 *  Constructs a releasePlannedOrdersByPlannerCode as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
releasePlannedOrdersByPlannerCode::releasePlannedOrdersByPlannerCode(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);


  // signals and slots connections
  connect(_release, SIGNAL(clicked()), this, SLOT(sRelease()));
  connect(_submit, SIGNAL(clicked()), this, SLOT(sSubmit()));

  if (!_metrics->boolean("EnableBatchManager"))
    _submit->hide();

  _plannerCode->setType(ParameterGroup::PlannerCode);
  
// TODO Always append to existing unreleased Transfer Orders
  _appendTransferOrder->hide();
  _appendTransferOrder->setChecked(TRUE);
}

/*
 *  Destroys the object and frees any allocated resources
 */
releasePlannedOrdersByPlannerCode::~releasePlannedOrdersByPlannerCode()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void releasePlannedOrdersByPlannerCode::languageChange()
{
  retranslateUi(this);
}

void releasePlannedOrdersByPlannerCode::sRelease()
{
  if (!_cutoffDate->isValid())
  {
    QMessageBox::critical( this, tr("Enter Cutoff Date"),
                           tr("Please enter a valid Cutoff Date.") );
    _cutoffDate->setFocus();
    return;
  }

  QString sql( "SELECT releasePlannedOrder(planord_id, TRUE, :appendTransferOrder) "
               "FROM planord, itemsite "
               "WHERE ( (planord_itemsite_id=itemsite_id)"
               " AND (planord_startdate<=:cutOffDate)" );

  if (_warehouse->isChecked())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  if (_firmedOnly->isChecked())
    sql += " AND (planord_firm)";

  if (_plannerCode->isSelected())
    sql += " AND (itemsite_plancode_id=:plancode_id)";
  else if (_plannerCode->isPattern())
    sql += " AND (itemsite_plancode_id IN (SELECT plancode_id FROM plancode WHERE (plancode_code ~ :plancode_pattern)))";

  sql += ");";

  q.prepare(sql);
  q.bindValue(":cutOffDate", _cutoffDate->date());
  q.bindValue(":appendTransferOrder",	QVariant(_appendTransferOrder->isChecked()));
  _warehouse->bindValue(q);
  _plannerCode->bindValue(q);
  q.exec();

  accept();
}

void releasePlannedOrdersByPlannerCode::sSubmit()
{
  if (!_cutoffDate->isValid())
  {
    QMessageBox::warning( this, tr("Enter Cut Off Date"),
                          tr( "You must enter a valid Cut Off Date before\n"
                              "submitting this job." ));
    _cutoffDate->setFocus();
    return;
  }

  ParameterList params;

  params.append("action_name", "ReleasePlannedOrders");

  _plannerCode->appendValue(params);
  _warehouse->appendValue(params);

  if(_firmedOnly->isChecked())
    params.append("firmedOnly", QVariant(true,0));

  params.append("cutoff_offset", QDate::currentDate().daysTo(_cutoffDate->date()));

  submitAction newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() == XDialog::Accepted)
    accept();
}

