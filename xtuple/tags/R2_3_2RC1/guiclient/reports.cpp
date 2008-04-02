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

#include "reports.h"

#include <QVariant>
#include <QMessageBox>
#include <QStatusBar>
#include <QWorkspace>
#include <openreports.h>
#include <reporthandler.h>

/*
 *  Constructs a reports as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
reports::reports(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
  setupUi(this);

  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_report, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
  connect(_report, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
  connect(_report, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));

  statusBar()->hide();

  _report->addColumn( tr("Name"),       200, Qt::AlignLeft );
  _report->addColumn( tr("Grade"),       50, Qt::AlignRight);
  _report->addColumn( tr("Description"), -1, Qt::AlignLeft | Qt::AlignTop);

  connect(omfgThis, SIGNAL(reportsChanged(int, bool)), this, SLOT(sFillList()));

  sFillList();
}

/*
 *  Destroys the object and frees any allocated resources
 */
reports::~reports()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void reports::languageChange()
{
    retranslateUi(this);
}

void reports::sFillList()
{
  _report->populate( "SELECT report_id, report_name, report_grade, report_descrip "
                     "FROM report "
                     "ORDER BY report_name, report_grade" );
}

void reports::sPrint()
{
  orReport report("ReportsMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void reports::sNew()
{
  if (!omfgThis->_reportHandler)
  {
    omfgThis->_reportHandler = new ReportHandler(omfgThis, "report handler");
    if(_preferences->value("InterfaceWindowOption") != "TopLevel")
      omfgThis->_reportHandler->setParentWindow(omfgThis->workspace());
    omfgThis->_reportHandler->setAllowDBConnect(FALSE);
    omfgThis->_reportHandler->setPlaceMenusOnWindows(TRUE);
    omfgThis->_reportHandler->setPlaceToolbarsOnWindows(TRUE);
    connect(omfgThis->_reportHandler, SIGNAL(reportsChanged(int, bool)), omfgThis, SLOT(sReportsChanged(int, bool)));
  }

  omfgThis->_reportHandler->fileNew();
}

void reports::sEdit()
{
  if (!omfgThis->_reportHandler)
  {
    omfgThis->_reportHandler = new ReportHandler(omfgThis, "report handler");
    if(_preferences->value("InterfaceWindowOption") != "TopLevel")
      omfgThis->_reportHandler->setParentWindow(omfgThis->workspace());
    omfgThis->_reportHandler->setAllowDBConnect(FALSE);
    omfgThis->_reportHandler->setPlaceMenusOnWindows(TRUE);
    omfgThis->_reportHandler->setPlaceToolbarsOnWindows(TRUE);
    connect(omfgThis->_reportHandler, SIGNAL(reportsChanged(int, bool)), omfgThis, SLOT(sReportsChanged(int, bool)));
  }

  q.prepare( "SELECT report_name, report_grade, report_source "
             "  FROM report "
             " WHERE (report_id=:report_id); " );
  q.bindValue(":report_id", _report->id());
  q.exec();
  if (q.first())
  {
    QDomDocument doc;
    QString errorMessage;
    int errorLine;
    int errorColumn;

    if (doc.setContent(q.value("report_source").toString(), &errorMessage, &errorLine, &errorColumn))
      omfgThis->_reportHandler->fileOpen(doc, q.value("report_name").toString(), q.value("report_grade").toInt());
    else
      QMessageBox::warning( this, tr("Error Loading Report"),
                            tr( "ERROR parsing content:\n"
                                "\t%1 (Line %2 Column %3" )
                            .arg(errorMessage)
                            .arg(errorLine)
                            .arg(errorColumn) );
  }
}

void reports::sDelete()
{
  q.prepare( "DELETE FROM report "
             "WHERE (report_id=:report_id);" );
  q.bindValue(":report_id", _report->id());
  q.exec();

  sFillList();
}
