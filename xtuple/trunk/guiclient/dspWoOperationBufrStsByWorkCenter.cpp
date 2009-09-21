/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspWoOperationBufrStsByWorkCenter.h"

#include <QMenu>
#include <QVariant>
#include <QMessageBox>
//#include <QStatusBar>
#include <openreports.h>
#include "woOperation.h"
#include "submitReport.h"

/*
 *  Constructs a dspWoOperationBufrStsByWorkCenter as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspWoOperationBufrStsByWorkCenter::dspWoOperationBufrStsByWorkCenter(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_wooper, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_wrkcnt, SIGNAL(valid(bool)), _query, SLOT(setEnabled(bool)));
  connect(_autoUpdate, SIGNAL(toggled(bool)), this, SLOT(sHandleAutoUpdate(bool)));
  connect(_submit, SIGNAL(clicked()), this, SLOT(sSubmit()));

  _wrkcnt->populate( "SELECT wrkcnt_id, wrkcnt_code "
                     "FROM wrkcnt "
                     "ORDER BY wrkcnt_code;" );

  _wooper->addColumn(tr("W/O #"),         _orderColumn,  Qt::AlignLeft,   true,  "wonumber"   );
  _wooper->addColumn(tr("Status"),        _statusColumn, Qt::AlignCenter, true,  "bufrsts_status" );
  _wooper->addColumn(tr("Type"),          _uomColumn,    Qt::AlignLeft,   true,  "bufrststype");
  _wooper->addColumn(tr("Item Number"),   _itemColumn,   Qt::AlignLeft,   true,  "item_number"   );
  _wooper->addColumn(tr("Seq #"),         _seqColumn,    Qt::AlignCenter, true,  "wooper_seqnumber" );
  _wooper->addColumn(tr("Std. Oper."),    _itemColumn,   Qt::AlignLeft,   true,  "stdoper"   );
  _wooper->addColumn(tr("Description"),   -1,            Qt::AlignLeft,   true,  "wooperdescrip"   );
  _wooper->addColumn(tr("Setup Remain."), _itemColumn,   Qt::AlignRight,  true,  "setup"  );
  _wooper->addColumn(tr("Run Remain."),   _itemColumn,   Qt::AlignRight,  true,  "run"  );
  _wooper->addColumn(tr("Qty. Remain."),  _qtyColumn,    Qt::AlignRight,  true,  "qtyremain"  );
  _wooper->addColumn(tr("UOM"),           _uomColumn,    Qt::AlignCenter, true,  "uom_name" );
  
  if (_preferences->boolean("XCheckBox/forgetful"))
    _QtyAvailOnly->setChecked(true);

  if (!_metrics->boolean("EnableBatchManager"))
    _submit->hide();

  sFillList();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspWoOperationBufrStsByWorkCenter::~dspWoOperationBufrStsByWorkCenter()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspWoOperationBufrStsByWorkCenter::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspWoOperationBufrStsByWorkCenter::set(const ParameterList &pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("wrkcnt_id", &valid);
  if (valid)
    _wrkcnt->setId(param.toInt());

  if (pParams.inList("run"))
  {
    sFillList();
    return NoError_Run;
  }

  return NoError;
}

void dspWoOperationBufrStsByWorkCenter::sPrint()
{
  ParameterList params(buildParameters());

  orReport report("WOOperationBufrStsByWorkCenter", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspWoOperationBufrStsByWorkCenter::sPopulateMenu(QMenu *pMenu)
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("View Operation..."), this, SLOT(sViewOperation()), 0);
  if ((!_privileges->check("ViewWoOperations")) && (!_privileges->check("MaintainWoOperations")))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Edit Operation..."), this, SLOT(sEditOperation()), 0);
  if (!_privileges->check("MaintainWoOperations"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Delete Operation..."), this, SLOT(sDeleteOperation()), 0);
  if (!_privileges->check("MaintainWoOperations"))
    pMenu->setItemEnabled(menuItem, FALSE);
}

void dspWoOperationBufrStsByWorkCenter::sViewOperation()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("wooper_id", _wooper->id());

  woOperation newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspWoOperationBufrStsByWorkCenter::sEditOperation()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("wooper_id", _wooper->id());

  woOperation newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspWoOperationBufrStsByWorkCenter::sDeleteOperation()
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

    sFillList();
  }
}

void dspWoOperationBufrStsByWorkCenter::sHandleAutoUpdate(bool pAutoUpdate)
{
  if (pAutoUpdate)
    connect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
  else
    disconnect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
}

void dspWoOperationBufrStsByWorkCenter::sFillList()
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

  QString sql( "SELECT wooper_id, formatWoNumber(wo_id) AS wonumber, bufrsts_status,"
               "       CASE WHEN (bufrsts_type='T') THEN 'Time'"
               "           ELSE 'Stock'"
               "       END AS bufrststype,"
               "       item_number, wooper_seqnumber,"
               "       CASE WHEN (wooper_stdopn_id <> -1) THEN ( SELECT stdopn_number"
               "                                                   FROM stdopn"
               "                                                  WHERE (stdopn_id=wooper_stdopn_id) )"
               "            ELSE ''"
               "       END AS stdoper,"
               "       (wooper_descrip1 || ' ' || wooper_descrip2) AS wooperdescrip,"
               "       CASE WHEN (wooper_sucomplete) THEN 0"
               "            ELSE noNeg(wooper_sutime - wooper_suconsumed)"
               "       END AS setup,"
               "       CASE WHEN (wooper_rncomplete) THEN 0"
               "            ELSE noNeg(wooper_rntime - wooper_rnconsumed)"
               "       END AS run,"
               "       noNeg(wo_qtyord - wooper_qtyrcv) AS qtyremain, uom_name,"
               "       CASE WHEN (bufrsts_status > 65) THEN 'error' END AS bufrsts_status_qtforegroundrole,"
               "       '1' AS setup_xtnumericrole,"
               "       '1' AS run_xtnumericrole,"
               "       CASE WHEN (wooper_sucomplete) THEN :complete END AS setup_qtdisplayrole,"
               "       CASE WHEN (wooper_rncomplete) THEN :complete END AS run_qtdisplayrole,"
               "       'qty' AS qtyremain_xtnumericrole "
               "  FROM wooper, wo, itemsite, item, uom, bufrsts "
               " WHERE ( (wooper_wo_id=wo_id)"
               "   AND   (wo_itemsite_id=itemsite_id)"
               "   AND   (wo_status <> 'C')"
               "   AND   (itemsite_item_id=item_id)"
               "   AND   (item_inv_uom_id=uom_id)"
               "   AND   (bufrsts_target_type='W')"
               "   AND   (bufrsts_target_id=wo_id)"
               "   AND   (bufrsts_date=current_date)"
               "   AND   (wooper_rncomplete=False)"
               "   AND   (wooper_wrkcnt_id=:wrkcnt_id)" );

  if (_QtyAvailOnly->isChecked())
    sql += " AND ((wooperqtyavail(wooper_id) > 0))";

  sql += ") "
         "ORDER BY bufrsts_status DESC, wo_number, wo_subnumber, wooper_seqnumber;";

  q.prepare(sql);
  q.bindValue(":complete", tr("Complete"));
  q.bindValue(":wrkcnt_id", _wrkcnt->id());
  q.exec();
  _wooper->populate(q);
}

void dspWoOperationBufrStsByWorkCenter::sSubmit()
{
  ParameterList params(buildParameters());
  params.append("report_name","WOOperationBufrStsByWorkCenter");

  submitReport newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.check() == cNoReportDefinition)
    QMessageBox::critical( this, tr("Report Definition Not Found"),
                           tr( "The report defintions for this report, \"WOOperationBufrStsByWorkCenter\" cannot be found.\n"
                               "Please contact your Systems Administrator and report this issue." ) );
  else
    newdlg.exec();
}

ParameterList dspWoOperationBufrStsByWorkCenter::buildParameters()
{
  ParameterList params;
  params.append("wrkcnt_id", _wrkcnt->id());

  if(_QtyAvailOnly->isChecked())
    params.append("QtyAvailOnly");

  return params;

}

