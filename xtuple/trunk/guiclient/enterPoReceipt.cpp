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

// rename to enterReceipt
#include "enterPoReceipt.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>

#include "distributeInventory.h"
#include "enterPoitemReceipt.h"
#include "getLotInfo.h"
#include "mqlutil.h"
#include "printLabelsByPo.h"
#include "storedProcErrorLookup.h"

enterPoReceipt::enterPoReceipt(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
  setupUi(this);

  connect(_all,		SIGNAL(clicked()),	this, SLOT(sReceiveAll()));
  connect(_enter,	SIGNAL(clicked()),	this, SLOT(sEnter()));
  connect(_order,	SIGNAL(valid(bool)),	this, SLOT(sFillList()));
  connect(_post,	SIGNAL(clicked()),	this, SLOT(sPost()));
  connect(_print,	SIGNAL(clicked()),	this, SLOT(sPrint()));
  connect(_save,	SIGNAL(clicked()),	this, SLOT(sSave()));

  _order->setAllowedStatuses(OrderLineEdit::Open);
  _order->setAllowedTypes(OrderLineEdit::Purchase |
			  OrderLineEdit::Return |
			  OrderLineEdit::Transfer);
  if (_metrics->boolean("EnableReturnAuth"))
  {
      _order->setExtraClause("RA", "(SELECT SUM(raitem_qtyauthorized) > 0 "
                       "  FROM raitem"
                       "  WHERE ((raitem_rahead_id=orderhead_id)"
                       "     AND (orderhead_type = 'RA'))) "
                       " AND "
                       "(SELECT TRUE "
                       " FROM raitem"
                       " WHERE ((raitem_rahead_id=orderhead_id)"
                       "   AND  (raitem_disposition IN ('R','P','V')) "
                       "   AND  (orderhead_type = 'RA')) "
                       " LIMIT 1)");
  }
  _order->setFocus();

  _orderitem->addColumn(tr("#"),            _whsColumn,  Qt::AlignCenter );
  _orderitem->addColumn(tr("Due Date"),     _dateColumn, Qt::AlignLeft   );
  _orderitem->addColumn(tr("Item Number"),  _itemColumn, Qt::AlignLeft   );
  _orderitem->addColumn(tr("UOM"),          _uomColumn,  Qt::AlignCenter );
  _orderitem->addColumn(tr("Vend. Item #"), -1,          Qt::AlignLeft   );
  _orderitem->addColumn(tr("UOM"),          _uomColumn,  Qt::AlignCenter );
  _orderitem->addColumn(tr("Ordered"),      _qtyColumn,  Qt::AlignRight  );
  _orderitem->addColumn(tr("Received"),     _qtyColumn,  Qt::AlignRight  );
  _orderitem->addColumn(tr("To Receive"),   _qtyColumn,  Qt::AlignRight  );

  _captive = FALSE;
}

enterPoReceipt::~enterPoReceipt()
{
  // no need to delete child widgets, Qt does it all for us
}

void enterPoReceipt::languageChange()
{
  retranslateUi(this);
}

enum SetResponse enterPoReceipt::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("pohead_id", &valid);
  if (valid)
  {
    _captive = TRUE;
    _order->setId(param.toInt(), "PO");
    _orderitem->setFocus();
  }

  param = pParams.value("tohead_id", &valid);
  if (valid)
  {
    _captive = TRUE;
    _order->setId(param.toInt(), "TO");
    _orderitem->setFocus();
  }

  param = pParams.value("rahead_id", &valid);
  if (valid)
  {
    _captive = TRUE;
    _order->setId(param.toInt(), "RA");
    _orderitem->setFocus();
  }

  return NoError;
}

void enterPoReceipt::setParams(ParameterList & params)
{
  if (_metrics->boolean("MultiWhs"))
    params.append("MultiWhs");

  if (_order->isValid())
  {
    if (_order->isPO())
      params.append("pohead_id", _order->id());
    else if (_order->isRA())
      params.append("rahead_id", _order->id());
    else if (_order->isTO())
      params.append("tohead_id", _order->id());

    params.append("orderid",   _order->id());
    params.append("ordertype", _order->type());
  }

  params.append("nonInventory",	tr("Non-Inventory"));
  params.append("na",		tr("N/A"));
}

void enterPoReceipt::sPrint()
{
  ParameterList params;
  setParams(params);

  printLabelsByPo newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void enterPoReceipt::post(const QString pType, const int pId)
{
  enterPoReceipt w(0, "enterPoReceipt");
  w.setWindowModality(Qt::WindowModal);
  ParameterList params;
  if (pType == "PO")
    params.append("pohead_id", pId);
  else if (pType == "RA")
    params.append("rahead_id", pId);
  else if (pType == "TO")
    params.append("tohead_id", pId);
  w.set(params);
  w.sPost();
}

void enterPoReceipt::sPost()
{
  ParameterList params;	// shared by several queries
  setParams(params);
  QList<QVariant> itemlocSeriesList;
  QString lotnum = QString::null;
  QDate expdate = omfgThis->startOfTime();

 // Check that there is something to receive
  QString checks = "SELECT SUM(qtyToReceive(recv_order_type,"
                   "                        recv_orderitem_id)) AS qtyToRecv "
                   "FROM recv, orderitem "
                   "WHERE ((recv_order_type=<? value(\"ordertype\") ?>)"
                   "  AND  (recv_orderitem_id=orderitem_id)"
                   "  AND  (orderitem_orderhead_id=<? value(\"orderid\") ?>));";
  MetaSQLQuery checkm(checks);
  q = checkm.toQuery(params);
  if (q.first())
  {
    if (q.value("qtyToRecv").toDouble() == 0)
    {
      QMessageBox::critical(this, tr("Nothing selected for Receipt"),
			    tr("<p>No Line Items have been selected "
			       "for receipt. Select at least one Line Item and "
			       "enter a Receipt before trying to Post."));
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  // Handle single lot distribution option
  if ( _singleLot->isChecked() )
  {
    // first find out if we have any lot controlled items that need distribution
    QString itemlocSerial (  "SELECT COUNT(*) "
                             "FROM recv, orderitem, itemsite "
                             "WHERE ((recv_orderitem_id=orderitem_id) "
                             "AND  (orderitem_orderhead_id=<? value(\"orderid\") ?>) "
                             "AND  (orderitem_orderhead_type=pordertype) "
                             "AND  (NOT recv_posted) "
                             "AND  (itemsite_loccntrl) "
                             "AND  (itemsite_id=recv_itemsite_id) "
                             "AND  (recv_order_type=<? value (\"ordertype\") ?>)); " );
    MetaSQLQuery distm(itemlocSerial);
    q = distm.toQuery(params);
    if (q.first())
    {
      getLotInfo newdlg(this, "", TRUE);
       
      // if we have distribution then ask for a lot# and optionally expiration date.
      itemlocSerial =  "SELECT COUNT(*) "
                       "FROM recv, orderitem, itemsite "
                       "WHERE ((recv_orderitem_id=orderitem_id) "
                       "AND  (orderitem_orderhead_id=<? value(\"orderid\") ?>) "
                       "AND  (orderitem_orderhead_type=pordertype) "
                       "AND  (NOT recv_posted) "
                       "AND  (itemsite_perishable) "
                       "AND  (itemsite_loccntrl) "
                       "AND  (itemsite_id=recv_itemsite_id) "
                       "AND  (recv_order_type=<? value (\"ordertype\") ?>)); " ;
      distm.setQuery(itemlocSerial);
      q = distm.toQuery(params);
      if (q.first())                    
        newdlg.enableExpiration(TRUE);  
      else if (q.lastError().type() != QSqlError::None)
      {
        systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
        return;
      }

      if(newdlg.exec() == QDialog::Accepted)
      {
        lotnum = newdlg.lot();
        expdate = newdlg.expiration();
      }
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }

  XSqlQuery rollback;
  rollback.prepare("ROLLBACK;");
  q.exec("BEGIN;");	// because of possible insertgltransaction failures
  
  //Fetch receipts and loop through them
  QString receipts (   "SELECT recv_id, recv_itemsite_id, recv_qty * orderitem_qty_invuomratio as recv_qty, "
                       "( (itemsite_loccntrl) OR (itemsite_controlmethod IN ('L','S')) ) AS dist "
                       "FROM recv, orderitem, itemsite "
                       "WHERE ((recv_orderitem_id=orderitem_id) "
                       "AND  (orderitem_orderhead_id=<? value(\"orderid\") ?>) "
                       "AND  (orderitem_orderhead_type=<? value (\"ordertype\") ?>) "
                       "AND  (NOT recv_posted) "
                       "AND  (itemsite_id=recv_itemsite_id) "
                       "AND  (recv_order_type=<? value (\"ordertype\") ?>)); " );
  MetaSQLQuery receiptm(receipts);
  XSqlQuery r;
  r = receiptm.toQuery(params);
  if (q.lastError().type() != QSqlError::None)
  {
    rollback.exec();
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  while (r.next())
  {
    //Prepare transaction
    QString posts ( "SELECT postReceipt(<? value(\"recv_id\") ?>,  "
                    "<? if exists(\"itemlocSeriesList\") ?> "
                    "  ARRAY[ "
                    "  <? foreach(\"itemlocSeriesList\") ?> "
                    "    <? if not isfirst(\"itemlocSeriesList\") ?> "
                    "    ,"
                    "    <? endif ?> "
                    "   <? value(\"itemlocSeriesList\") ?> "
                    "  <? endforeach ?> ] "
                    "<? else ?> "
                    " NULL::INTEGER[] "
                    "<? endif ?> "
                    ") AS result;" );
    ParameterList rparams;
    rparams.append("recv_id", r.value("recv_id").toInt());
    rparams.append("recv_qty", r.value("recv_qty").toInt());
    
    if (r.value("dist").toBool())  //Need to request distribution info
    {
      itemlocSeriesList = distributeInventory::SeriesAdjust(r.value("recv_itemsite_id").toInt(), r.value("recv_qty").toDouble(), this, lotnum, expdate);
      if (itemlocSeriesList.at(0) == -1)  // If true, distribution was canceled
      {
        QMessageBox::information(  this, tr("Enter Receipt"), tr(  "Post canceled."  ));
        rollback.exec();
        return;
      }
      else
        rparams.append("itemlocSeriesList", itemlocSeriesList);
    }
    
    MetaSQLQuery postm(posts);
    q = postm.toQuery(rparams);
    if (q.lastError().type() != QSqlError::None)
    {
      rollback.exec();
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  // Looks like everything is okay, so let's commit
  q.exec("COMMIT;");
  
  if (_captive)
  {
    _orderitem->clear();
    close();
  }
  else
  {
    _order->setId(-1);
    _close->setText(tr("&Close"));
  }
    
  omfgThis->sPurchaseOrderReceiptsUpdated();
}

void enterPoReceipt::sSave()
{
  // most of the work is done in enterPoitemReceipt
  // don't call this->close() 'cause that deletes the recv records
  QMainWindow::close();
}

void enterPoReceipt::sEnter()
{
  ParameterList params;
  params.append("lineitem_id", _orderitem->id());
  params.append("order_type", _order->type());
  params.append("mode", "new");

  enterPoitemReceipt newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != QDialog::Rejected)
    sFillList();
}

void enterPoReceipt::sFillList()
{
  _orderitem->clear();

  disconnect(_order,	SIGNAL(valid(bool)),	this, SLOT(sFillList()));
  if (_order->isValid())
  {
    ParameterList params;
    setParams(params);
    MetaSQLQuery fillm = mqlLoad(":/sr/enterReceipt/FillListDetail.mql");
    q = fillm.toQuery(params);
    _orderitem->populate(q);
    if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      connect(_order,	SIGNAL(valid(bool)),	this, SLOT(sFillList()));
      return;
    }
  }
  connect(_order,	SIGNAL(valid(bool)),	this, SLOT(sFillList()));
}

void enterPoReceipt::close()
{
  QList<QTreeWidgetItem*> zeroItems = _orderitem->findItems("^[0.]*$", Qt::MatchRegExp, 8);
  if (_order->isValid() &&
      zeroItems.size() != _orderitem->topLevelItemCount())
  {
    if (QMessageBox::question(this, tr("Cancel Receipts?"),
			      tr("<p>Are you sure you want to cancel all "
				 "unposted Receipts for this Order?"),
			      QMessageBox::Yes,
			      QMessageBox::No | QMessageBox::Default) == QMessageBox::Yes)
    {
      ParameterList params;
      setParams(params);
      QString dels = "SELECT deleteRecvForOrder(<? value (\"ordertype\") ?>,"
		     "                   <? value(\"orderid\") ?>) AS result;" ;
      MetaSQLQuery delm(dels);
      q = delm.toQuery(params);
      if (q.first())
      {
        int result = q.value("result").toInt();
        if (result < 0)
        {
          systemError(this, storedProcErrorLookup("deleteRecvForOrder", result), __FILE__, __LINE__);
          return;
        }
      }
      else if (q.lastError().type() != QSqlError::None)
      {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return;
      }
      omfgThis->sPurchaseOrderReceiptsUpdated();
    }
    else
      return;
  }
  QMainWindow::close();
}

void enterPoReceipt::sReceiveAll()
{
  ParameterList params;
  setParams(params);
  if (_metrics->boolean("EnableReturnAuth"))
    params.append("EnableReturnAuth", TRUE);
  MetaSQLQuery recvm = mqlLoad(":/sr/enterReceipt/ReceiveAll.mql");
  q = recvm.toQuery(params);

  while (q.next())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("enterReceipt", result),
		  __FILE__, __LINE__);
      return;
    }
    omfgThis->sPurchaseOrderReceiptsUpdated();
  }
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}
