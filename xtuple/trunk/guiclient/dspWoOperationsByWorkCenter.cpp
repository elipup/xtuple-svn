/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspWoOperationsByWorkCenter.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>
//#include <QStatusBar>
#include <QVariant>

#include <openreports.h>
#include "woOperation.h"
#include "postOperations.h"
#include "postProduction.h"
#include "dspRunningAvailability.h"
#include "dspMPSDetail.h"

/*
 *  Constructs a dspWoOperationsByWorkCenter as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspWoOperationsByWorkCenter::dspWoOperationsByWorkCenter(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_wooper, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*, QTreeWidgetItem*)));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));

  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _wrkcnt->populate( "SELECT wrkcnt_id, wrkcnt_code "
                     "FROM wrkcnt "
                     "ORDER BY wrkcnt_code;" );

  _wooper->addColumn(tr("Source"),        _orderColumn, Qt::AlignLeft,   true,  "source"   );
  _wooper->addColumn(tr("W/O #"),         _orderColumn, Qt::AlignLeft,   true,  "wonumber"   );
  _wooper->addColumn(tr("Status"),        _seqColumn,   Qt::AlignCenter, true,  "wo_status" );
  _wooper->addColumn(tr("Due Date"),      _dateColumn,  Qt::AlignCenter, true,  "scheduled" );
  _wooper->addColumn(tr("Item Number"),   _itemColumn,  Qt::AlignLeft,   true,  "item_number"   );
  _wooper->addColumn(tr("Seq #"),         _seqColumn,   Qt::AlignCenter, true,  "wooper_seqnumber" );
  _wooper->addColumn(tr("Std. Oper."),    _itemColumn,  Qt::AlignLeft,   true,  "stdoper"   );
  _wooper->addColumn(tr("Description"),   -1,           Qt::AlignLeft,   true,  "descrip"   );
  _wooper->addColumn(tr("Setup Remain."), _itemColumn,  Qt::AlignRight,  true,  "setup"  );
  _wooper->addColumn(tr("Run Remain."),   _itemColumn,  Qt::AlignRight,  true,  "run"  );
  _wooper->addColumn(tr("Qty. Remain."),  _qtyColumn,   Qt::AlignRight,  true,  "qtyremain"  );
  
  if (_preferences->boolean("XCheckBox/forgetful"))
    _loadOnly->setChecked(true);

  sFillList();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspWoOperationsByWorkCenter::~dspWoOperationsByWorkCenter()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspWoOperationsByWorkCenter::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspWoOperationsByWorkCenter::set(const ParameterList &pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("wrkcnt_id", &valid);
  if (valid)
    _wrkcnt->setId(param.toInt());

  param = pParams.value("startDate", &valid);
  if (valid)
    _dates->setStartDate(param.toDate());

  param = pParams.value("endDate", &valid);
  if (valid)
    _dates->setEndDate(param.toDate());

  _loadOnly->setChecked(pParams.inList("loadOnly"));

  if (pParams.inList("run"))
  {
    sFillList();
    return NoError_Run;
  }

  return NoError;
}

void dspWoOperationsByWorkCenter::sPrint()
{
  ParameterList params;
  _dates->appendValue(params);
  params.append("wrkcnt_id", _wrkcnt->id());

  if(_loadOnly->isChecked())
    params.append("loadOnly");

  orReport report("WOOperationsByWorkCenter", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspWoOperationsByWorkCenter::sPopulateMenu(QMenu *pMenu,  QTreeWidgetItem *selected)
{
  QString status(selected->text(2));
  int menuItem;
  bool multi = false;

  int cnt = _wooper->selectedItems().size();
  multi = (cnt > 1);

  menuItem = pMenu->insertItem(tr("View Operation..."), this, SLOT(sViewOperation()), 0);
  if (multi || ((!_privileges->check("ViewWoOperations")) && (!_privileges->check("MaintainWoOperations"))))
    pMenu->setItemEnabled(menuItem, FALSE);

  if ( (status == "E") || (status == "I") || (status == "R") )
  {
    menuItem = pMenu->insertItem(tr("Edit Operation..."), this, SLOT(sEditOperation()), 0);
    if (multi || !_privileges->check("MaintainWoOperations"))
      pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem(tr("Delete Operation..."), this, SLOT(sDeleteOperation()), 0);
    if (multi || !_privileges->check("MaintainWoOperations"))
      pMenu->setItemEnabled(menuItem, FALSE);

    pMenu->insertSeparator();

    menuItem = pMenu->insertItem(tr("Print Pick List(s)..."), this, SLOT(sPrintPickLists()), 0);
    if (!_privileges->check("PrintWorkOrderPaperWork"))
      pMenu->setItemEnabled(menuItem, FALSE);

    pMenu->insertSeparator();

    menuItem = pMenu->insertItem(tr("Post Production..."), this, SLOT(sPostProduction()), 0);
    if (multi || !_privileges->check("PostProduction"))
      pMenu->setItemEnabled(menuItem, FALSE);

    menuItem = pMenu->insertItem(tr("Post Operations..."), this, SLOT(sPostOperations()), 0);
    if (multi || !_privileges->check("PostWoOperations"))
      pMenu->setItemEnabled(menuItem, FALSE);

    pMenu->insertSeparator();
  }

  menuItem = pMenu->insertItem(tr("Running Availability..."), this, SLOT(sRunningAvailability()), 0);
  if (!_privileges->check("ViewInventoryAvailability"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("MPS Detail..."), this, SLOT(sMPSDetail()), 0);
  if (!_privileges->check("ViewMPS"))
    pMenu->setItemEnabled(menuItem, FALSE);
}

void dspWoOperationsByWorkCenter::sViewOperation()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("wooper_id", _wooper->id());

  woOperation newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspWoOperationsByWorkCenter::sEditOperation()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("wooper_id", _wooper->id());

  woOperation newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspWoOperationsByWorkCenter::sDeleteOperation()
{
  if (QMessageBox::critical( this, tr("Delete W/O Operation"),
                             tr( "If you Delete the selected W/O Operation\n"
                                 "you will not be able to post Labor to this Operation\n"
                                 "to this W/O.  Are you sure that you want to delete the\n"
                                 "selected W/O Operation?"),
                                 tr("&Yes"), tr("&No"), QString::null, 0, 1) == 0)
  {
    q.prepare( "DELETE FROM wooper "
               "WHERE (wooper_id=:wooper_id);" );
    q.bindValue(":wooper_id", _wooper->id());
    q.exec();
    if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    sFillList();
  }
}

void dspWoOperationsByWorkCenter::sRunningAvailability()
{
  q.prepare("SELECT wo_itemsite_id"
            "  FROM wo"
            " WHERE (wo_id=:wo_id);");
  q.bindValue(":wo_id", _wooper->altId());
  q.exec();
  if(q.first())
  {
    ParameterList params;
    params.append("itemsite_id", q.value("wo_itemsite_id").toInt());

    dspRunningAvailability * newdlg = new dspRunningAvailability();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void dspWoOperationsByWorkCenter::sMPSDetail()
{
  q.prepare("SELECT wo_itemsite_id"
            "  FROM wo"
            " WHERE (wo_id=:wo_id);");
  q.bindValue(":wo_id", _wooper->altId());
  q.exec();
  if(q.first())
  {
    ParameterList params;
    params.append("itemsite_id", q.value("wo_itemsite_id").toInt());

    dspMPSDetail * newdlg = new dspMPSDetail();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}


void dspWoOperationsByWorkCenter::sFillList()
{
  _wooper->clear();

  q.prepare( "SELECT wrkcnt_descrip, warehous_code "
             "FROM wrkcnt, warehous "
             "WHERE ( (wrkcnt_warehous_id=warehous_id)"
             " AND (wrkcnt_id=:wrkcnt_id) );" );
  q.bindValue(":wrkcnt_id", _wrkcnt->id());
  q.exec();
  if (q.first())
  {
    _description->setText(q.value("wrkcnt_descrip").toString());
    _warehouse->setText(q.value("warehous_code").toString());
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  QString sql( "SELECT wooper.*, formatWoNumber(wo_id) AS wonumber, item_number,"
               "       CAST(wooper_scheduled AS DATE) AS scheduled,"
               "       CASE WHEN (wooper_stdopn_id <> -1) THEN ( SELECT stdopn_number FROM stdopn WHERE (stdopn_id=wooper_stdopn_id) )"
               "            ELSE ''"
               "       END AS stdoper,"
               "       (wooper_descrip1 || ' ' || wooper_descrip2) AS descrip, wo_status,"
               "       CASE WHEN (wooper_sucomplete) THEN 0"
               "            ELSE noNeg(wooper_sutime - wooper_suconsumed)"
               "       END AS setup,"
               "       CASE WHEN (wooper_rncomplete) THEN 0"
               "            ELSE noNeg(wooper_rntime - wooper_rnconsumed)"
               "       END AS run,"
               "       noNeg(wo_qtyord - wooper_qtyrcv) AS qtyremain, uom_name,"
               "       CASE WHEN(wo_ordtype='M') THEN :mrp"
               "            WHEN(wo_ordtype='P') THEN :mps"
               "            WHEN(wo_ordtype='S') THEN (:so||'-'||formatSoNumber(wo_ordid))"
               "            WHEN(wo_ordtype='W') THEN (:wo||'-'||formatWoNumber(wo_ordid))"
               "            WHEN(wo_ordtype IS NULL OR wo_ordtype='') THEN :manual"
               "            ELSE wo_ordtype"
               "       END AS source,"
               "       CASE WHEN (date(wooper_scheduled) < CURRENT_DATE) THEN 'error' END AS wooper_scheduled_qtforegroundrole,"
               "       '1' AS setup_xtnumericrole,"
               "       '1' AS run_xtnumericrole,"
               "       CASE WHEN (wooper_sucomplete) THEN :complete END AS setup_qtdisplayrole,"
               "       CASE WHEN (wooper_rncomplete) THEN :complete END AS run_qtdisplayrole,"
               "       'qty' AS qtyremain_xtnumericrole "
               "FROM wooper, wo, itemsite, item, uom "
               "WHERE ( (wooper_wo_id=wo_id)"
               " AND (wo_itemsite_id=itemsite_id)"
               " AND (itemsite_item_id=item_id)"
               " AND (item_inv_uom_id=uom_id)"
               " AND (DATE(wooper_scheduled) BETWEEN :startDate AND :endDate)"
               " AND (wooper_wrkcnt_id=:wrkcnt_id)" );

  if (_loadOnly->isChecked())
    sql += " AND ( ((wooper_sutime - wooper_suconsumed) > 0) OR ((wooper_rntime - wooper_rnconsumed) > 0) )";

  sql += ") "
         "ORDER BY wooper_scheduled, wo_number, wo_subnumber, wooper_seqnumber;";

  q.prepare(sql);
  _dates->bindValue(q);
  q.bindValue(":complete", tr("Complete"));
  q.bindValue(":wrkcnt_id", _wrkcnt->id());
  q.bindValue(":mrp", tr("MRP"));
  q.bindValue(":mps", tr("MPS"));
  q.bindValue(":so", tr("SO"));
  q.bindValue(":wo", tr("WO"));
  q.bindValue(":manual", tr("Manual"));
  q.exec();
  _wooper->populate(q, true);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void dspWoOperationsByWorkCenter::sPostProduction()
{
  ParameterList params;
  params.append("wo_id", _wooper->altId());

  postProduction newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspWoOperationsByWorkCenter::sPostOperations()
{
  ParameterList params;
  params.append("wooper_id", _wooper->id());

  postOperations newdlg(this, "", TRUE);
  if(newdlg.set(params) != UndefinedError)
    newdlg.exec();
}

void dspWoOperationsByWorkCenter::sPrintPickLists()
{
  QPrinter printer(QPrinter::HighResolution);
  int counter = 0;
  bool userCanceled = false;
  if (orReport::beginMultiPrint(&printer, userCanceled) == false)
  {
    if(!userCanceled)
      systemError(this, tr("Could not initialize printing system for multiple reports."));
    return;
  }
  QList<XTreeWidgetItem*> selected = _wooper->selectedItems();
  for (int i = 0; i < selected.size(); i++)
  {
    ParameterList params;
    params.append("wo_id", ((XTreeWidgetItem*)selected[i])->altId());

    orReport report("PickList", params);
    if (! (report.isValid() && report.print(&printer, (counter == 0))) )
    {
      report.reportError(this);
      orReport::endMultiPrint(&printer);
      return;
    }

    counter++;
  }
  orReport::endMultiPrint(&printer);
}
