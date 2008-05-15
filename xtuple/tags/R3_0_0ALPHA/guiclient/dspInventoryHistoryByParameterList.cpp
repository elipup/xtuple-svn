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

#include "dspInventoryHistoryByParameterList.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include <openreports.h>

#include "adjustmentTrans.h"
#include "countTag.h"
#include "expenseTrans.h"
#include "materialReceiptTrans.h"
#include "mqlutil.h"
#include "scrapTrans.h"
#include "transactionInformation.h"
#include "transferTrans.h"
#include "workOrder.h"

dspInventoryHistoryByParameterList::dspInventoryHistoryByParameterList(QWidget* parent, const char* name, Qt::WFlags fl)
    : XMainWindow(parent, name, fl)
{
  setupUi(this);

  (void)statusBar();

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_invhist, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));

  _invhist->setRootIsDecorated(TRUE);
  _invhist->addColumn(tr("Transaction Time"),_timeDateColumn, Qt::AlignLeft  );
  _invhist->addColumn(tr("Created Time"),    _timeDateColumn, Qt::AlignLeft  );
  _invhist->addColumn(tr("User"),        _orderColumn,       Qt::AlignCenter );
  _invhist->addColumn(tr("Type"),        _transColumn,       Qt::AlignCenter );
  _invhist->addColumn(tr("Whs."),        _whsColumn,         Qt::AlignCenter );
  _invhist->addColumn(tr("Order #"),     _orderColumn,       Qt::AlignCenter );
  _invhist->addColumn(tr("Item Number"), -1,                 Qt::AlignLeft   );
  _invhist->addColumn(tr("UOM"),         _uomColumn,         Qt::AlignCenter );
  _invhist->addColumn(tr("Trans-Qty"),   _qtyColumn,         Qt::AlignRight  );
  _invhist->addColumn(tr("From Area"),   _orderColumn,       Qt::AlignLeft   );
  _invhist->addColumn(tr("QOH Before"),  _qtyColumn,         Qt::AlignRight  );
  _invhist->addColumn(tr("To Area"),     _orderColumn,       Qt::AlignLeft   );
  _invhist->addColumn(tr("QOH After"),   _qtyColumn,         Qt::AlignRight  );

  _transType->append(cTransAll,       tr("All Transactions")       );
  _transType->append(cTransReceipts,  tr("Receipts")               );
  _transType->append(cTransIssues,    tr("Issues")                 );
  _transType->append(cTransShipments, tr("Shipments")              );
  _transType->append(cTransAdjCounts, tr("Adjustments and Counts") );
  _transType->append(cTransTransfers, tr("Transfers")              );
  _transType->append(cTransScraps,    tr("Scraps")                 );
  _transType->setCurrentItem(0);
}

dspInventoryHistoryByParameterList::~dspInventoryHistoryByParameterList()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspInventoryHistoryByParameterList::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspInventoryHistoryByParameterList::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("classcode_id", &valid);
  if (valid)
  {
    _parameter->setType(ClassCode);
    _parameter->setId(param.toInt());
  }

  param = pParams.value("classcode_pattern", &valid);
  if (valid)
  {
    _parameter->setType(ClassCode);
    _parameter->setPattern(param.toString());
  }

  param = pParams.value("classcode", &valid);
  if (valid)
    _parameter->setType(ClassCode);

  param = pParams.value("plancode_id", &valid);
  if (valid)
  {
    _parameter->setType(PlannerCode);
    _parameter->setId(param.toInt());
  }

  param = pParams.value("plancode_pattern", &valid);
  if (valid)
  {
    _parameter->setType(PlannerCode);
    _parameter->setPattern(param.toString());
  }

  param = pParams.value("plancode", &valid);
  if (valid)
    _parameter->setType(PlannerCode);

  param = pParams.value("itemgrp_id", &valid);
  if (valid)
  {
    _parameter->setType(ItemGroup);
    _parameter->setId(param.toInt());
  }

  param = pParams.value("itemgrp_pattern", &valid);
  if (valid)
  {
    _parameter->setType(ItemGroup);
    _parameter->setPattern(param.toString());
  }

  param = pParams.value("itemgrp", &valid);
  if (valid)
    _parameter->setType(ItemGroup);

  param = pParams.value("warehous_id", &valid);
  if (valid)
    _warehouse->setId(param.toInt());

  param = pParams.value("startDate", &valid);
  if (valid)
    _dates->setStartDate(param.toDate());

  param = pParams.value("endDate", &valid);
  if (valid)
    _dates->setEndDate(param.toDate());

  param = pParams.value("transtype", &valid);
  if (valid)
  {
    QString transtype = param.toString();

    if (transtype == "R")
      _transType->setCurrentItem(1);
    else if (transtype == "I")
      _transType->setCurrentItem(2);
    else if (transtype == "S")
      _transType->setCurrentItem(3);
    else if (transtype == "A")
      _transType->setCurrentItem(4);
    else if (transtype == "T")
      _transType->setCurrentItem(5);
    else if (transtype == "SC")
      _transType->setCurrentItem(6);
  }

  if (pParams.inList("run"))
    sFillList();

  switch (_parameter->type())
  {
    case ClassCode:
      setCaption(tr("Inventory History by Class Code"));
      break;

    case PlannerCode:
      setCaption(tr("Inventory History by Planner Code"));
      break;

    case ItemGroup:
      setCaption(tr("Inventory History by Item Group"));
      break;

    default:
      break;
  }

  return NoError;
}

void dspInventoryHistoryByParameterList::setParams(ParameterList & params)
{
  _warehouse->appendValue(params);
  _parameter->appendValue(params);
  _dates->appendValue(params);
  params.append("transType", _transType->id());

  if (_parameter->type() == ItemGroup)
    params.append("itemgrp");
  else if(_parameter->type() == PlannerCode)
    params.append("plancode");
  else
    params.append("classcode");
}

void dspInventoryHistoryByParameterList::sPrint()
{
  if (!_dates->allValid())
  {
    QMessageBox::warning( this, tr("Enter a Valid Start Date and End Date"),
                          tr("You must enter a valid Start Date and End Date for this report.") );
    _dates->setFocus();
    return;
  }

  ParameterList params;
  _warehouse->appendValue(params);
  _parameter->appendValue(params);
  _dates->appendValue(params);
  params.append("transType", _transType->id());

  if (_parameter->isAll())
  {
    switch(_parameter->type())
    {
    case ItemGroup:
      params.append("itemgrp");
      break;
    case ClassCode:
      params.append("classcode");
      break;
    case PlannerCode:
      params.append("plancode");
      break;
    default:
      break;
    }
  }

  orReport report("InventoryHistoryByParameterList", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspInventoryHistoryByParameterList::sViewTransInfo()
{
  QString transType(((XTreeWidgetItem *)_invhist->currentItem())->text(2));

  ParameterList params;
  params.append("mode", "view");
  params.append("invhist_id", _invhist->id());

  if (transType == "AD")
  {
    adjustmentTrans *newdlg = new adjustmentTrans();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
  else if (transType == "TW")
  {
    transferTrans *newdlg = new transferTrans();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
  else if (transType == "SI")
  {
    scrapTrans *newdlg = new scrapTrans();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
  else if (transType == "EX")
  {
    expenseTrans *newdlg = new expenseTrans();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
  else if (transType == "RX")
  {
    materialReceiptTrans *newdlg = new materialReceiptTrans();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
  else if (transType == "CC")
  {
    countTag newdlg(this, "", TRUE);
    newdlg.set(params);
    newdlg.exec();
  }
  else
  {
    transactionInformation newdlg(this, "", TRUE);
    newdlg.set(params);
    newdlg.exec();
  }
}

void dspInventoryHistoryByParameterList::sEditTransInfo()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("invhist_id", _invhist->id());

  transactionInformation newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void dspInventoryHistoryByParameterList::sViewWOInfo()
{
  QString orderNumber = _invhist->currentItem()->text(4);
  int sep1            = orderNumber.find('-');
  int sep2            = orderNumber.find('-', (sep1 + 1));
  int mainNumber      = orderNumber.mid((sep1 + 1), ((sep2 - sep1) - 1)).toInt();
  int subNumber       = orderNumber.right((orderNumber.length() - sep2) - 1).toInt();

  q.prepare( "SELECT wo_id "
             "FROM wo "
             "WHERE ( (wo_number=:wo_number)"
             " AND (wo_subnumber=:wo_subnumber) );" );
  q.bindValue(":wo_number", mainNumber);
  q.bindValue(":wo_subnumber", subNumber);
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("mode", "view");
    params.append("wo_id", q.value("wo_id"));

    workOrder *newdlg = new workOrder();
    newdlg->set(params);
    omfgThis->handleNewWindow(newdlg);
  }
}

void dspInventoryHistoryByParameterList::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pItem)
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("View Transaction Information..."), this, SLOT(sViewTransInfo()), 0);
  menuItem = pMenu->insertItem(tr("Edit Transaction Information..."), this, SLOT(sEditTransInfo()), 0);

  if ( (pItem->text(3).length()) &&
       ( (pItem->text(2) == "RM") || (pItem->text(2) == "IM") ) )
  {
    QString orderNumber = _invhist->currentItem()->text(4);
    int sep1            = orderNumber.find('-');
    int sep2            = orderNumber.find('-', (sep1 + 1));
    int mainNumber      = orderNumber.mid((sep1 + 1), ((sep2 - sep1) - 1)).toInt();
    int subNumber       = orderNumber.right((orderNumber.length() - sep2) - 1).toInt();

    if ( (mainNumber) && (subNumber) )
    {
      q.prepare( "SELECT wo_id "
                 "FROM wo "
                 "WHERE ( (wo_number=:wo_number)"
                 " AND (wo_subnumber=:wo_subnumber) );" );
      q.bindValue(":wo_number", mainNumber);
      q.bindValue(":wo_subnumber", subNumber);
      q.exec();
      if (q.first())
      {
          menuItem = pMenu->insertItem(tr("View Work Order Information..."), this, SLOT(sViewWOInfo()), 0);
          if ((!_privileges->check("MaintainWorkOrders")) && (!_privileges->check("ViewWorkOrders")))
            pMenu->setItemEnabled(menuItem, FALSE);
      }
    }
  }
}

void dspInventoryHistoryByParameterList::sFillList()
{
  _invhist->clear();

  if (!_dates->startDate().isValid())
  {
    QMessageBox::critical( this, tr("Enter Start Date"),
                           tr("Please enter a valid Start Date.") );
    _dates->setFocus();
    return;
  }

  if (!_dates->endDate().isValid())
  {
    QMessageBox::critical( this, tr("Enter End Date"),
                           tr("Please enter a valid End Date.") );
    _dates->setFocus();
    return;
  }

  ParameterList params;
  setParams(params);
  MetaSQLQuery mql = mqlLoad(":/im/displays/InventoryHistoryByParameterList/FillListDetail.mql");
  q = mql.toQuery(params);

  if (q.first())
  {
    XTreeWidgetItem *parentItem = NULL;
    XTreeWidgetItem *child      = NULL;
    int             invhistid   = 0;

    do
    {
      if (q.value("invhist_id").toInt() != invhistid)
      {
        invhistid = q.value("invhist_id").toInt();

        parentItem = new XTreeWidgetItem( _invhist, parentItem,
					  q.value("invhist_id").toInt(),
					  q.value("invdetail_id").toInt(),
					  q.value("invhist_transdate"),
					  q.value("invhist_created"),
					  q.value("invhist_user"),
					  q.value("invhist_transtype"),
					  q.value("warehous_code"),
					  q.value("ordernumber"),
					  q.value("item_number"),
					  q.value("invhist_invuom"),
					  q.value("transqty") );

        if (q.value("invhist_posted").toBool())
        {
          parentItem->setText( 9, q.value("locfrom").toString());
          parentItem->setText(10, q.value("qohbefore").toString());
          parentItem->setText(11, q.value("locto").toString());
          parentItem->setText(12, q.value("qohafter").toString());
        }
        else
          parentItem->setTextColor("orange");
      }

      if (q.value("invdetail_id").toInt())
      {
        child = new XTreeWidgetItem(parentItem, q.value("invhist_id").toInt(),
				    q.value("invdetail_id").toInt(),
				    "", "", "", "", "",
				    q.value("locationname"),
				    "", q.value("detailqty") );

        if (q.value("invhist_posted").toBool())
        {
          child->setText(10, q.value("locationqtybefore").toString());
          child->setText(12, q.value("locationqtyafter").toString());
        }
        else
          child->setTextColor("orange");
      }
    }
    while (q.next());
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
