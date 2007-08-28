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

#include "dspWoEffortByWorkOrder.h"

#include <qvariant.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qworkspace.h>
#include "math.h"
#include "rptWoEffortByWorkOrder.h"
#include "dspWoEffortByWorkOrder.h"
#include "wotc.h"
#include "xlistview.h"
#include "wocluster.h"

/*
 *  Constructs a dspWoEffortByWorkOrder as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspWoEffortByWorkOrder::dspWoEffortByWorkOrder(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_wotc, SIGNAL(populateMenu(Q3PopupMenu*,Q3ListViewItem*,int)), this, SLOT(sPopulateMenu(Q3PopupMenu*,Q3ListViewItem*)));
    connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(_wo, SIGNAL(valid(bool)), _print, SLOT(setEnabled(bool)));
    connect(_autoUpdate, SIGNAL(toggled(bool)), this, SLOT(sHandleAutoUpdate(bool)));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspWoEffortByWorkOrder::~dspWoEffortByWorkOrder()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspWoEffortByWorkOrder::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <Q3PopupMenu>
#include <QSqlError>
void dspWoEffortByWorkOrder::init()
{
  statusBar()->hide();

  //omfgThis->inputManager()->notify(cBCWorkOrder, this, _wo, SLOT(setId(int)));

  _wotc->addColumn(tr("User"),         _userColumn,     Qt::AlignLeft  );
  _wotc->addColumn(tr("Operation"),    -1,              Qt::AlignLeft  );
  _wotc->addColumn(tr("Time In"),      _timeDateColumn, Qt::AlignRight );
  _wotc->addColumn(tr("Time Out"),     _timeDateColumn, Qt::AlignRight );
  _wotc->addColumn(tr("Setup Time"),   _timeColumn,     Qt::AlignRight );
  _wotc->addColumn(tr("Run Time"),     _timeColumn,     Qt::AlignRight );
  _wotc->addColumn(tr("Effort"),       _timeColumn,     Qt::AlignRight );

  connect(omfgThis, SIGNAL(workOrdersUpdated(int,bool)),  this,SLOT(sFillList()));
}

enum SetResponse dspWoEffortByWorkOrder::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("wo_id", &valid);
  if (valid)
    _wo->setId(param.toInt());

  if (pParams.inList("run"))
  {
    sFillList();
    return NoError_Run;
  }

  return NoError;
}

void dspWoEffortByWorkOrder::sPrint()
{
  ParameterList params;
  params.append("wo_id", _wo->id());
  params.append("print");

  rptWoEffortByWorkOrder newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspWoEffortByWorkOrder::sPopulateMenu(Q3PopupMenu *pMenu, Q3ListViewItem *)
{
  int menuItem;

  if (_wotc->id() != -1)
  {
    menuItem = pMenu->insertItem(tr("New"), this, SLOT(sNew()), 0);
    pMenu->setItemEnabled(menuItem, _privleges->check("MaintainWoTimeClock"));
    
    menuItem = pMenu->insertItem(tr("Edit"), this, SLOT(sEdit()), 0);
    pMenu->setItemEnabled(menuItem, _privleges->check("MaintainWoTimeClock"));

    menuItem = pMenu->insertItem(tr("View"), this, SLOT(sView()), 0);

    menuItem = pMenu->insertItem(tr("Delete"), this, SLOT(sDelete()), 0);
    pMenu->setItemEnabled(menuItem, _privleges->check("MaintainWoTimeClock"));
  }
}


void dspWoEffortByWorkOrder::sNew()
{
  ParameterList params;
  params.append("wo_id", _wo->id());
  params.append("mode",  "new");

  wotc *newdlg = new wotc(this, "", true);
  newdlg->set(params);
  newdlg->exec();

  sFillList();
}

void dspWoEffortByWorkOrder::sEdit()
{
  ParameterList params;
  params.append("wotc_id", _wotc->id());
  params.append("mode",    "edit");

  wotc *newdlg = new wotc(this, "", true);
  newdlg->set(params);
  newdlg->exec();

  sFillList();
}

void dspWoEffortByWorkOrder::sView()
{
  ParameterList params;
  params.append("wotc_id", _wotc->id());
  params.append("mode",    "view");
  
  wotc *newdlg = new wotc(this, "", true);
  newdlg->set(params);
  newdlg->exec();

  sFillList();
}

void dspWoEffortByWorkOrder::sDelete()
{
  if (QMessageBox::question(this, tr("Delete Selected Entry?"),
			    tr("Are you sure that you want to delete the "
			       "selected time clock entry?"),
			    tr("&Yes"), tr("&No"), 0, 0, 1) == 0)
  {
    q.prepare("DELETE FROM wotc "
	      "WHERE (wotc_id=:wotc_id);");
    q.bindValue(":wotc_id", _wotc->id());
    q.exec();
    if (q.lastError().type() != QSqlError::NoError)
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);

    sFillList();
  }
}

void dspWoEffortByWorkOrder::sFillList()
{
  _wotc->clear();

  if (_wo->isValid())
  {
    q.prepare( "SELECT wotc_id, usr_username,"
	       "       wooper_seqnumber || ' - ' || wooper_descrip1 || ' - ' ||"
	       "                                    wooper_descrip2 AS wooper, "
	       "       formatDateTime(wotc_timein) AS timein,"
	       "       formatDateTime(wotc_timeout) AS timeout,"
	       "       NULL AS setup_time, NULL AS run_time,"
	       "       formatInterval(wotcTime(wotc_id)) AS wo_effort "
	       "FROM usr, wotc LEFT OUTER JOIN "
	       "     wooper ON (wotc_wooper_id=wooper_id) "
	       "WHERE ((wotc_wo_id=:wo_id)"
	       "  AND  (wotc_usr_id=usr_id) "
	       "  AND  (wotc_id NOT IN (SELECT DISTINCT wooperpost_wotc_id "
	       "                        FROM wooperpost"
	       "                        WHERE (wooperpost_wo_id=:wo_id)"
	       "                          AND (wooperpost_wotc_id IS NOT NULL)))) "
	       "UNION "
	       "SELECT wotc_id, usr_username,"
	       "       CAST(wooperpost_seqnumber AS TEXT) AS wooper, "
	       "       formatDateTime(wotc_timein) AS timein,"
	       "       formatDateTime(wotc_timeout) AS timeout,"
	       "       formatInterval(wooperpost_sutime) AS setup_time,"
	       "       formatInterval(wooperpost_rntime) AS run_time,"
	       "       formatInterval(wotcTime(wotc_id)) AS wo_effort "
	       "FROM usr, wotc, wooperpost "
	       "WHERE ((wotc_wo_id=:wo_id)"
	       "  AND  (wotc_usr_id=usr_id) "
	       "  AND  (wooperpost_wotc_id=wotc_id)) "
	       "ORDER BY timein, timeout;" );
    q.bindValue(":wo_id", _wo->id());
    q.exec();
    _wotc->populate(q);

    if (q.lastError().type() != QSqlError::NoError)
    {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return;
    }

    // we have to piece together the TOTAL line because one select won't work
    XListViewItem* lastLine = new XListViewItem(_wotc, _wotc->lastItem(),
		       _wo->id(), "", tr("Total"));

    q.prepare("SELECT formatDateTime(MIN(wotc_timein)) AS timein,"
	      "       formatDateTime(MAX(wotc_timeout)) AS timeout,"
	      "       formatInterval(woTimeByWo(wotc_wo_id)) AS wo_effort "
	      "FROM wotc "
	      "WHERE (wotc_wo_id=:wo_id) "
	      "GROUP BY wotc_wo_id;");
    q.bindValue(":wo_id", _wo->id());
    q.exec();
    if (q.first())
    {
      lastLine->setText(2, q.value("timein"));
      lastLine->setText(3, q.value("timeout"));
      lastLine->setText(6, q.value("wo_effort"));
    }
    else if (q.lastError().type() != QSqlError::NoError)
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);

    q.prepare("SELECT formatInterval(SUM(wooperpost_sutime)) AS setup_time,"
	      "       formatInterval(SUM(wooperpost_rntime)) AS run_time,"
	      "       SUM(wooperpost_sutime) + SUM(wooperpost_rntime) -"
	      "           intervalToMinutes(woTimeByWo(wooperpost_wo_id)) AS variance "
	      "FROM wooperpost "
	      "WHERE (wooperpost_wo_id=:wo_id) "
	      "GROUP BY wooperpost_wo_id;");
    q.bindValue(":wo_id", _wo->id());
    q.exec();
    if (q.first())
    {
      lastLine->setText(4, q.value("setup_time"));
      lastLine->setText(5, q.value("run_time"));
      if (fabs(q.value("variance").toDouble()) > 1.5)	// rounding errors that appear <= 1 min
	lastLine->setColor("red");
    }
    else if (q.lastError().type() != QSqlError::NoError)
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);

  }
}

void dspWoEffortByWorkOrder::sHandleAutoUpdate(bool pAutoUpdate)
{
  if (pAutoUpdate)
    connect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
  else
    disconnect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
}
