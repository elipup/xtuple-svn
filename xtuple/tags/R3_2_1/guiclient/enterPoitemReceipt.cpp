/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "enterPoitemReceipt.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

#include <metasql.h>

#include "distributeInventory.h"
#include "itemSite.h"
#include "mqlutil.h"
#include "storedProcErrorLookup.h"

enterPoitemReceipt::enterPoitemReceipt(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_receive, SIGNAL(clicked()), this, SLOT(sReceive()));

  _invVendorUOMRatio->setPrecision(omfgThis->ratioVal());
  _ordered->setPrecision(omfgThis->qtyVal());
  _received->setPrecision(omfgThis->qtyVal());
  _returned->setPrecision(omfgThis->qtyVal());

  _toReceive->setValidator(omfgThis->qtyVal());
  _toReceive->setFocus();
  _receiptDate->setDate(QDate::currentDate());

  _mode		= cView;
  _orderitemid	= -1;
  _ordertype	= "";
  _receivable	= 0.0;
  _recvid	= -1;
}

enterPoitemReceipt::~enterPoitemReceipt()
{
  // no need to delete child widgets, Qt does it all for us
}

void enterPoitemReceipt::languageChange()
{
  retranslateUi(this);
}

bool enterPoitemReceipt::correctReceipt(int pRecvid, QWidget *pParent)
{
  //Validate - Split receipts may not be corrected
  q.prepare("SELECT (count(*) > 0) AS result "
            "FROM recv "
            "WHERE (((recv_id=:recvid) "
            "  AND (recv_splitfrom_id IS NOT NULL)) "
            "  OR (recv_splitfrom_id=:recvid)); ");
  q.bindValue(":recvid", pRecvid);
  q.exec();
  if (q.first())
  {
    if (q.value("result").toBool())
    {
        QMessageBox::critical( pParent, tr("Cannot Correct"),
                         tr("<p>Receipt has been split.  The received quantity may not be changed."));
        return XDialog::Rejected;
    }
    else
    {
      ParameterList params;
      params.append("mode", "edit");
      params.append("recv_id", pRecvid);

      enterPoitemReceipt newdlg(pParent, "", TRUE);
      newdlg.set(params);

      if (newdlg.exec() != XDialog::Rejected)
        return true;
    }
  }
  return false;
}

enum SetResponse enterPoitemReceipt::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
      _mode = cNew;
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      _toReceiveLit->setText(tr("Correct Qty. to:"));
      _freightLit->setText(tr("Correct Freight to:"));
      _receive->setText(tr("Co&rrect"));
      _item->setEnabled(false);
      setWindowTitle(tr("Correct Item Receipt"));
    }
  }

  param = pParams.value("order_type", &valid);
  if (valid)
    _ordertype = param.toString();

  param = pParams.value("lineitem_id", &valid);
  if (valid)
  {
    _orderitemid = param.toInt();
    populate();
  }

  param = pParams.value("porecv_id", &valid);	// deprecated
  if (valid)
  {
    _recvid = param.toInt();
    populate();
  }

  param = pParams.value("recv_id", &valid);
  if (valid)
  {
    _recvid = param.toInt();
    populate();
  }

  return NoError;
}

void enterPoitemReceipt::populate()
{
  ParameterList params;

  if (_metrics->boolean("MultiWhs"))
    params.append("MultiWhs");

  // NOTE: this crashes if popm is defined and toQuery() is called outside the blocks
  if (_mode == cNew)
  {
    MetaSQLQuery popm = mqlLoad("itemReceipt", "populateNew");

    params.append("ordertype",    _ordertype);
    params.append("orderitem_id", _orderitemid);

    q = popm.toQuery(params);
  }
  else if (_mode == cEdit)
  {
    MetaSQLQuery popm = mqlLoad("itemReceipt", "populateEdit");
    params.append("recv_id", _recvid);
    q = popm.toQuery(params);
  }
  else
  {
    systemError(this, tr("<p>Incomplete Parameter List: "
			 "_orderitem_id=%1, _ordertype=%2, _mode=%3.")
                       .arg(_orderitemid)
                       .arg(_ordertype)
                       .arg(_mode) );
    return;
  }

  if (q.first())
  {
    _orderNumber->setText(q.value("order_number").toString());
    _lineNumber->setText(q.value("orderitem_linenumber").toString());
    _vendorItemNumber->setText(q.value("vend_item_number").toString());
    _vendorDescrip->setText(q.value("vend_item_descrip").toString());
    _vendorUOM->setText(q.value("vend_uom").toString());
    _invVendorUOMRatio->setDouble(q.value("orderitem_qty_invuomratio").toDouble());
    _dueDate->setDate(q.value("duedate").toDate());
    _ordered->setDouble(q.value("orderitem_qty_ordered").toDouble());
    _received->setDouble(q.value("qtyreceived").toDouble());
    _returned->setDouble(q.value("qtyreturned").toDouble());
    _receivable = q.value("receivable").toDouble();
    _notes->setText(q.value("notes").toString());
    _receiptDate->setDate(q.value("effective").toDate());
    _freight->setId(q.value("curr_id").toInt());
    _freight->setLocalValue(q.value("recv_freight").toDouble());

    if (_ordertype.isEmpty())
      _ordertype = q.value("recv_order_type").toString();
    if (_ordertype == "PO")
      _orderType->setText(tr("P/O"));
    else if (_ordertype == "TO")
      _orderType->setText(tr("T/O"));
    else if (_ordertype == "RA")
      _orderType->setText(tr("R/A"));

    int itemsiteid = q.value("itemsiteid").toInt();
    if (itemsiteid > 0)
      _item->setItemsiteid(itemsiteid);
    _item->setEnabled(false);

    if (_mode == cNew)
      _toReceive->setText(q.value("f_qtytoreceive").toString());

    if (q.value("inventoryitem").toBool() && itemsiteid <= 0)
    {
      MetaSQLQuery ism = mqlLoad("itemReceipt", "sourceItemSite");
      XSqlQuery isq = ism.toQuery(params);
      if (isq.first())
      {
        itemsiteid = itemSite::createItemSite(this,
                      isq.value("itemsite_id").toInt(),
                      isq.value("warehous_id").toInt(),
                      true);
        if (itemsiteid < 0)
          return;
        _item->setItemsiteid(itemsiteid);
      }
      else if (isq.lastError().type() != QSqlError::NoError)
      {
        systemError(this, isq.lastError().databaseText(), __FILE__, __LINE__);
        return;
      }
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void enterPoitemReceipt::sReceive()
{
  XSqlQuery rollback;
  rollback.prepare("ROLLBACK;");
    
  if(_metrics->boolean("DisallowReceiptExcessQty") && _receivable < _toReceive->toDouble())
  {
    QMessageBox::critical( this, tr("Cannot Receive"),
                           tr("<p>Cannot receive more quantity than ordered."));
    return;
  }

  if(_ordertype == "RA" && _receivable < _toReceive->toDouble())
  {
    QMessageBox::critical( this, tr("Cannot Receive"),
                           tr("<p>Cannot receive more quantity than authorized."));
    return;
  }

  double tolerance = _metrics->value("ReceiptQtyTolerancePct").toDouble() / 100.0;
  if(_metrics->boolean("WarnIfReceiptQtyDiffers") &&
      (_receivable < _toReceive->toDouble() * (1.0 - tolerance) ||
       _receivable > _toReceive->toDouble() * (1.0 + tolerance)))
  {
    if(QMessageBox::question( this, tr("Receipt Qty. Differs"),
      tr("<p>The Qty entered does not match the receivable Qty for this order. "
         "Do you wish to continue anyway?"),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel)
      return;
  }

  int result = 0;
  QString storedProc;
  if (_mode == cNew)
  {
    q.prepare("SELECT enterReceipt(:ordertype, :poitem_id, :qty, :freight, :notes, "
		  ":curr_id, :effective) AS result;");
    q.bindValue(":poitem_id",	_orderitemid);
    q.bindValue(":ordertype",	_ordertype);
    storedProc = "enterReceipt";
  }
  else if (_mode == cEdit)
  {
    q.exec("BEGIN;");	// because of possible lot, serial, or location distribution cancelations
    q.prepare("UPDATE recv SET recv_notes = :notes WHERE (recv_id=:recv_id);" );
    q.bindValue(":notes",	_notes->toPlainText());
    q.bindValue(":recv_id",	_recvid);
    q.exec();
    if (q.lastError().type() != QSqlError::NoError)
    {
      rollback.exec();
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
    
    q.prepare("SELECT correctReceipt(:recv_id, :qty, :freight, 0, "
		":curr_id, :effective) AS result;");
    q.bindValue(":recv_id", _recvid);
    storedProc = "correctReceipt";
  }

  q.bindValue(":qty",		_toReceive->toDouble());
  q.bindValue(":freight",	_freight->localValue());
  q.bindValue(":notes",		_notes->toPlainText());
  q.bindValue(":curr_id",	_freight->id());
  q.bindValue(":effective",	_receiptDate->date());
  q.exec();
  if (q.first())
  {
    result = q.value("result").toInt();
    if (result < 0)
    {
      rollback.exec();
      systemError(this, storedProcErrorLookup(storedProc, result),
		  __FILE__, __LINE__);
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
      rollback.exec();
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
  }

  if(cEdit == _mode)
  {
    if (distributeInventory::SeriesAdjust(result, this) == XDialog::Rejected)
    {
      rollback.exec();
      QMessageBox::information( this, tr("Enterp PO Receipt"), tr("Transaction Canceled") );
      return;
    }

    q.exec("COMMIT;");
  }

  omfgThis->sPurchaseOrderReceiptsUpdated();
  accept();
}
