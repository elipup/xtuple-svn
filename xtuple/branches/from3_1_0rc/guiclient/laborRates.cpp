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

#include "laborRates.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include <qstatusbar.h>
#include <parameter.h>
#include <openreports.h>
#include "laborRate.h"

/*
 *  Constructs a laborRates as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
laborRates::laborRates(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
    connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
    connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
    connect(_lbrrate, SIGNAL(valid(bool)), _view, SLOT(setEnabled(bool)));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
laborRates::~laborRates()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void laborRates::languageChange()
{
    retranslateUi(this);
}


void laborRates::init()
{
  statusBar()->hide();
  
  _lbrrate->addColumn(tr("Labor Rate"),  _itemColumn, Qt::AlignLeft,   true,  "lbrrate_code"  );
  _lbrrate->addColumn(tr("Description"), -1,          Qt::AlignLeft,   true,  "lbrrate_descrip"  );
  _lbrrate->addColumn(tr("Rate"),        _costColumn, Qt::AlignRight,  true,  "lbrrate_rate" );

  if (_privileges->check("MaintainLaborRates"))
  {
    connect(_lbrrate, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_lbrrate, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_lbrrate, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    connect(_lbrrate, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));

    _new->setEnabled(FALSE);
  }

  sFillList();
}

void laborRates::sPrint()
{
  orReport report("StdLaborRatesMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void laborRates::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  laborRate newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void laborRates::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("lbrrate_id", _lbrrate->id());

  laborRate newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void laborRates::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("lbrrate_id", _lbrrate->id());

  laborRate newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void laborRates::sDelete()
{
  q.prepare( "SELECT wrkcnt_id "
             "FROM wrkcnt "
             "WHERE ( (wrkcnt_run_lbrrate_id=:lbrrate_id)"
             " OR (wrkcnt_setup_lbrrate_id=:lbrrate_id) ) "
             "LIMIT 1;" );
  q.bindValue(":lbrrate_id", _lbrrate->id());
  q.exec();
  if (q.first())
  {
    QMessageBox::information( this, tr("Cannot Delete Standard Labor Rate"),
                              tr( "The selected Standard Labor Rate cannot be deleted as it is in use at one or more Work Centers.\n"
                                  "You must first remove all references of the selected Standard Labor Rate before you may delete it.\n" ));
    return;
  }

  q.prepare( "DELETE FROM lbrrate "
             "WHERE (lbrrate_id=:lbrrate_id);" );
  q.bindValue(":lbrrate_id", _lbrrate->id());
  q.exec();

  sFillList();
}

void laborRates::sFillList()
{
  _lbrrate->populate( "SELECT lbrrate_id, lbrrate_code, lbrrate_descrip,"
                      "       lbrrate_rate, 'curr' AS lbrrate_rate_xtnumericrole "
                      "FROM lbrrate "
                      "ORDER BY lbrrate_code" );
}



