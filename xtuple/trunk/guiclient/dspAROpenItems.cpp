/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspAROpenItems.h"

#include <QMenu>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include "mqlutil.h"

#include <openreports.h>

#include <currcluster.h>

#include "arOpenItem.h"
#include "dspInvoiceInformation.h"
#include "invoice.h"
#include "incident.h"

dspAROpenItems::dspAROpenItems(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_aropen, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));

  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _aropen->setRootIsDecorated(TRUE);
  _aropen->addColumn(tr("Doc. Type"),     _itemColumn, Qt::AlignLeft,  true, "doctype");
  _aropen->addColumn(tr("Doc. #"),       _orderColumn, Qt::AlignLeft,  true, "aropen_docnumber");
  _aropen->addColumn(tr("Cust./Incdt."),  _itemColumn, Qt::AlignLeft,  true, "cust_number");
  _aropen->addColumn(tr("Name/Desc."),    _itemColumn, Qt::AlignLeft,  true, "cust_name");
  _aropen->addColumn(tr("Order/AssignTo"),_itemColumn, Qt::AlignRight, true, "aropen_ordernumber");
  _aropen->addColumn(tr("Doc. Date"),     _dateColumn, Qt::AlignCenter,true, "aropen_docdate");
  _aropen->addColumn(tr("Due Date"),      _dateColumn, Qt::AlignCenter,true, "aropen_duedate");
  _aropen->addColumn(tr("Amount"),    _bigMoneyColumn, Qt::AlignRight, true, "aropen_amount");
  _aropen->addColumn(tr("Paid"),      _bigMoneyColumn, Qt::AlignRight, true, "aropen_paid");
  _aropen->addColumn(tr("Balance"),   _bigMoneyColumn, Qt::AlignRight, true, "balance");
  _aropen->addColumn(tr("Currency"),  _currencyColumn, Qt::AlignLeft,  true, "currAbbr");
  if(omfgThis->singleCurrency())
    _aropen->hideColumn("currAbbr");
}

dspAROpenItems::~dspAROpenItems()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspAROpenItems::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspAROpenItems::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("startDate", &valid);
  if (valid)
    _dates->setStartDate(param.toDate());

  param = pParams.value("endDate", &valid);
  if (valid)
    _dates->setEndDate(param.toDate());

  if (pParams.inList("run"))
  {
    sFillList();
    return NoError_Run;
  }

  return NoError;
}

void dspAROpenItems::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pItem)
{
  int menuItem;

  if (((XTreeWidgetItem *)pItem)->id() != -1)
  {
    menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEdit()), 0);
    if (!_privileges->check("EditAROpenItem"))
      pMenu->setItemEnabled(menuItem, FALSE);

    pMenu->insertItem(tr("View..."), this, SLOT(sView()), 0);
    
    XTreeWidgetItem* item = (XTreeWidgetItem*)pItem;
    {
      if (item->text(0) == "Invoice")
      {
        pMenu->insertItem(tr("View Invoice..."), this, SLOT(sViewInvoice()), 0);
        pMenu->insertItem(tr("View Invoice Details..."), this, SLOT(sViewInvoiceDetails()), 0);
      }
    }
    
   pMenu->insertSeparator();

    menuItem = pMenu->insertItem(tr("New Incident..."), this, SLOT(sIncident()), 0);
    if (!_privileges->check("AddIncidents"))
      pMenu->setItemEnabled(menuItem, FALSE);
  }
  else
  {
    pMenu->insertSeparator();
    
    menuItem = pMenu->insertItem(tr("Edit Incident..."), this, SLOT(sEditIncident()), 0);
    if (!_privileges->check("MaintainIncidents"))
      pMenu->setItemEnabled(menuItem, FALSE);

    pMenu->insertItem(tr("View Incident..."), this, SLOT(sViewIncident()), 0);
    if (!_privileges->check("ViewIncidents"))
      pMenu->setItemEnabled(menuItem, FALSE);
  }
}

void dspAROpenItems::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("aropen_id", _aropen->id());
  arOpenItem newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspAROpenItems::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("aropen_id", _aropen->id());
  arOpenItem newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspAROpenItems::sViewInvoice()
{
  q.prepare("SELECT aropen_docnumber FROM aropen WHERE (aropen_id=:aropen_id);");
  q.bindValue(":aropen_id", _aropen->id());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("invoiceNumber", q.value("aropen_docnumber"));
    dspInvoiceInformation* newdlg = new dspInvoiceInformation();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
}

void dspAROpenItems::sViewInvoiceDetails()
{
  q.prepare("SELECT invchead_id FROM aropen, invchead WHERE ((aropen_id=:aropen_id) AND (invchead_invcnumber=aropen_docnumber));");
  q.bindValue(":aropen_id", _aropen->id());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("invchead_id", q.value("invchead_id"));
    params.append("mode", "view");
    invoice* newdlg = new invoice();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
}

void dspAROpenItems::sIncident()
{
  q.prepare("SELECT crmacct_id, crmacct_cntct_id_1 "
            "FROM crmacct, aropen "
            "WHERE ((aropen_id=:aropen_id) "
            "AND (crmacct_cust_id=aropen_cust_id));");
  q.bindValue(":aropen_id", _aropen->id());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("mode", "new");
    params.append("aropen_id", _aropen->id());
    params.append("crmacct_id", q.value("crmacct_id"));
    params.append("cntct_id", q.value("crmacct_cntct_id_1"));
    incident newdlg(this, 0, TRUE);
    newdlg.set(params);

    if (newdlg.exec() == XDialog::Accepted)
      sFillList();
  }
}

void dspAROpenItems::sEditIncident()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("incdt_id", _aropen->altId());
  incident newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspAROpenItems::sViewIncident()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("incdt_id", _aropen->altId());
  incident newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

bool dspAROpenItems::setParams(ParameterList &params)
{
  if (_docDate->isChecked())
    _dates->appendValue(params);
  else
  {
    params.append("startDueDate", _dates->startDate());
    params.append("endDueDate", _dates->endDate());
  }
  params.append("invoice", tr("Invoice"));
  params.append("creditMemo", tr("Credit Memo"));
  params.append("debitMemo", tr("Debit Memo"));
  params.append("cashdeposit", tr("Customer Deposit"));
  if (_incidentsOnly->isChecked())
    params.append("incidentsOnly");

  return true;
}

void dspAROpenItems::sPrint()
{
  ParameterList params;
  if (! setParams(params))
    return;

  orReport report("AROpenItems", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspAROpenItems::sFillList()
{
  MetaSQLQuery mql = mqlLoad("arOpenItems", "detail");
  ParameterList params;
  if (! setParams(params))
    return;
  q = mql.toQuery(params);
  _aropen->populate(q, true);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
