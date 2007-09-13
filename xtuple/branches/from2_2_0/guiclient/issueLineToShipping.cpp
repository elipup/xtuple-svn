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

#include "issueLineToShipping.h"

#include <QApplication>
#include <QSqlError>
#include <QVariant>
#include <QValidator>

#include <metasql.h>

#include "xmessagebox.h"
#include "distributeInventory.h"
#include "storedProcErrorLookup.h"

issueLineToShipping::issueLineToShipping(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_issue, SIGNAL(clicked()), this, SLOT(sIssue()));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));

  _requireInventory = false;
  _snooze = false;
  _item->setReadOnly(TRUE);

  _qtyToIssue->setValidator(omfgThis->qtyVal());
}

issueLineToShipping::~issueLineToShipping()
{
  // no need to delete child widgets, Qt does it all for us
}

void issueLineToShipping::languageChange()
{
  retranslateUi(this);
}

enum SetResponse issueLineToShipping::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("soitem_id", &valid);
  if (valid)
  {
    _itemid = param.toInt();
    _ordertype = "SO";
    _orderNumberLit->setText(tr("Sales Order #:"));
    populate();
  }

  param = pParams.value("toitem_id", &valid);
  if (valid)
  {
    _itemid = param.toInt();
    _ordertype = "TO";
    _orderNumberLit->setText(tr("Transfer Order #:"));
    populate();
  }

  if (pParams.inList("requireInventory"))
    _requireInventory = true;

  param = pParams.value("qty", &valid);
  if (valid)
    _qtyToIssue->setText(param.toString());

  _snooze = pParams.inList("snooze");

  if(pParams.inList("issue"))
    sIssue();

  return NoError;
}

void issueLineToShipping::sIssue()
{
  if (_qtyToIssue->toDouble() <= 0)
  {
    XMessageBox::message( (isShown() ? this : parentWidget()), QMessageBox::Warning, tr("Invalid Quantity to Issue to Shipping"),
                          tr(  "<p>Please enter a non-negative, non-zero value to indicate the amount "
                               "of Stock you wish to Issue to Shipping for this Order Line." ),
                          QString::null, QString::null, _snooze );
    _qtyToIssue->setFocus();
    return;
  }

  if(_requireInventory)
  {
    q.prepare("SELECT sufficientInventoryToShipItem(:ordertype, :orderitemid) AS result;");
    q.bindValue(":ordertype",   _ordertype);
    q.bindValue(":orderitemid", _itemid);
    q.exec();
    if (q.first())
    {
      int result = q.value("result").toInt();
      if (result < 0)
      {
	ParameterList errp;
	if (_ordertype == "SO")
	  errp.append("soitem_id", _itemid);
	else if (_ordertype == "TO")
	  errp.append("toitem_id", _itemid);

	QString errs = "<? if exists(\"soitem_id\") ?>"
		  "SELECT item_number, warehous_code "
		  "  FROM coitem, item, itemsite, whsinfo "
		  " WHERE ((coitem_itemsite_id=itemsite_id)"
		  "   AND  (itemsite_item_id=item_id)"
		  "   AND  (itemsite_warehous_id=warehous_id)"
		  "   AND  (coitem_id=<? value(\"soitem_id\") ?>));"
		  "<? elseif exists(\"toitem_id\")?>"
		  "SELECT item_number, tohead_srcname AS warehous_code "
		  "  FROM toitem, tohead, item "
		  " WHERE ((toitem_item_id=item_id)"
		  "   AND  (toitem_tohead_id=tohead_id)"
		  "   AND  (toitem_id=<? value(\"toitem_id\") ?>));"
		  "<? endif ?>" ;
	MetaSQLQuery errm(errs);
	q = errm.toQuery(errp);
	if (! q.first() && q.lastError().type() != QSqlError::None)
	    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	systemError(this,
		    storedProcErrorLookup("sufficientInventoryToShipItem",
					  result)
		    .arg(q.value("item_number").toString())
		    .arg(q.value("warehous_code").toString()), __FILE__, __LINE__);
	return;
      }
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }

  // check to see if we are over issuing
  ParameterList params;
  if (_ordertype == "SO")
    params.append("soitem_id", _itemid);
  else if (_ordertype == "TO")
    params.append("toitem_id", _itemid);
  params.append("qty", _qtyToIssue->toDouble());

  QString sql = "<? if exists(\"soitem_id\") ?>"
	    "SELECT (noNeg(coitem_qtyord - coitem_qtyshipped + coitem_qtyreturned) <"
	    "           (COALESCE(SUM(shipitem_qty), 0) + <? value(\"qty\") ?>)) AS overship"
            "  FROM coitem LEFT OUTER JOIN"
            "        ( shipitem JOIN shiphead"
            "          ON ( (shipitem_shiphead_id=shiphead_id) AND (NOT shiphead_shipped) )"
            "        ) ON  (shipitem_orderitem_id=coitem_id)"
            " WHERE (coitem_id=<? value(\"soitem_id\") ?>)"
            " GROUP BY coitem_qtyord, coitem_qtyshipped, coitem_qtyreturned;"
	    "<? elseif exists(\"toitem_id\") ?>"
	    "SELECT (noNeg(toitem_qty_ordered - toitem_qty_shipped) <"
	    "           (COALESCE(SUM(shipitem_qty), 0) + <? value(\"qty\") ?>)) AS overship"
            "  FROM toitem LEFT OUTER JOIN"
            "        ( shipitem JOIN shiphead"
            "          ON ( (shipitem_shiphead_id=shiphead_id) AND (NOT shiphead_shipped) )"
            "        ) ON  (shipitem_orderitem_id=toitem_id)"
            " WHERE (toitem_id=<? value(\"toitem_id\") ?>)"
            " GROUP BY toitem_qty_ordered, toitem_qty_shipped;"
	    "<? endif ?>"
	    ;
  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  if (q.next() && q.value("overship").toBool())
  {
    if(XMessageBox::message( (isShown() ? this : parentWidget()) , QMessageBox::Question, tr("Inventory Overshipped"),
        tr("<p>You have selected to ship more inventory than required. Do you want to continue?"),
        tr("Yes"), tr("No"), _snooze, 0, 1) == 1)
      return;
  }
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  XSqlQuery issue;
  issue.prepare("SELECT issueToShipping(:ordertype, :lineitem_id, :qty, 0, CURRENT_TIMESTAMP) AS result;");
  issue.bindValue(":ordertype", _ordertype);
  issue.bindValue(":lineitem_id", _itemid);
  issue.bindValue(":qty", _qtyToIssue->toDouble());
  issue.exec();

  if (issue.first())
  {
    int result = issue.value("result").toInt();
    if (result < 0)
    {
      systemError( this, storedProcErrorLookup("issueToShipping", result),
		  __FILE__, __LINE__);
      return;
    }
    else
    {
      distributeInventory::SeriesAdjust(result, this);
      accept();
    }
  }
  else if (issue.lastError().type() != QSqlError::None)
  {
    systemError(this, issue.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void issueLineToShipping::populate()
{
  ParameterList itemp;
  if (_ordertype == "SO")
    itemp.append("soitem_id", _itemid);
  else if (_ordertype == "TO")
    itemp.append("toitem_id", _itemid);
  itemp.append("ordertype", _ordertype);

  QString sql = "<? if exists(\"soitem_id\") ?>"
		"SELECT cohead_number AS order_number,"
		"       itemsite_item_id AS item_id,"
		"       warehous_code,"
		"       formatQty(coitem_qtyord) AS qtyordered,"
		"       formatQty(coitem_qtyshipped) AS qtyshipped,"
		"       formatQty(coitem_qtyreturned) AS qtyreturned,"
		"       formatQty(noNeg(coitem_qtyord - coitem_qtyshipped +"
		"                       coitem_qtyreturned)) AS balance "
		"FROM cohead, coitem, itemsite, item, warehous "
		"WHERE ((coitem_cohead_id=cohead_id)"
		"  AND  (coitem_itemsite_id=itemsite_id)"
		"  AND  (coitem_status <> 'X')"
		"  AND  (itemsite_item_id=item_id)"
		"  AND  (itemsite_warehous_id=warehous_id)"
		"  AND  (coitem_id=<? value(\"soitem_id\") ?>) );"
		"<? elseif exists(\"toitem_id\") ?>"
		"SELECT tohead_number AS order_number,"
		"       toitem_item_id AS item_id,"
		"       warehous_code,"
		"       formatQty(toitem_qty_ordered) AS qtyordered,"
		"       formatQty(toitem_qty_shipped) AS qtyshipped,"
		"       0 AS qtyreturned,"
		"       formatQty(noNeg(toitem_qty_ordered -"
		"                       toitem_qty_shipped)) AS balance "
		"FROM tohead, toitem, warehous "
		"WHERE ((toitem_tohead_id=tohead_id)"
		"  AND  (toitem_status <> 'X')"
		"  AND  (tohead_src_warehous_id=warehous_id)"
		"  AND  (toitem_id=<? value(\"toitem_id\") ?>) );"
		"<? endif ?>";

  MetaSQLQuery itemm(sql);
  XSqlQuery itemq = itemm.toQuery(itemp);

  if (itemq.first())
  {
    _orderNumber->setText(itemq.value("order_number").toString());
    _item->setId(itemq.value("item_id").toInt());
    _warehouse->setText(itemq.value("warehous_code").toString());
    _qtyOrdered->setText(itemq.value("qtyordered").toString());
    _qtyShipped->setText(itemq.value("qtyshipped").toString());
    _qtyReturned->setText(itemq.value("qtyreturned").toString());
    _balance->setText(itemq.value("balance").toString());
  }
  else if (itemq.lastError().type() != QSqlError::None)
  {
    systemError(this, itemq.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  ParameterList shipp;
  shipp.append("ordertype", _ordertype);
  shipp.append("orderitem_id", _itemid);

  sql = "SELECT shiphead_id AS misc_id,"
	"       formatQty(SUM(shipitem_qty)) AS qtyatship "
	"FROM shiphead, shipitem "
	"WHERE ((shipitem_shiphead_id=shiphead_id)"
	"  AND  (NOT shiphead_shipped)"
	"  AND  (shiphead_order_type=<? value(\"ordertype\") ?>)"
	"  AND  (shipitem_orderitem_id=<? value(\"orderitem_id\") ?>) ) "
	"GROUP BY shiphead_id;" ;

  MetaSQLQuery shipm(sql);
  XSqlQuery shipq = shipm.toQuery(shipp);

  if (shipq.first())
  {
    _shipment->setType(_ordertype);
    _shipment->setId(shipq.value("misc_id").toInt());
    _qtyAtShip->setText(shipq.value("qtyatship").toString());
  }
  else if (shipq.lastError().type() != QSqlError::None)
  {
    systemError( this, shipq.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  if (_qtyAtShip->text().toDouble() == 0.0)
    _qtyToIssue->setText(itemq.value("balance").toString());
}
