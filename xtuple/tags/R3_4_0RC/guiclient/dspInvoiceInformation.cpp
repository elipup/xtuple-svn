/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspInvoiceInformation.h"

#include <QSqlError>
#include <QVariant>

#include "invoice.h"

#include <openreports.h>
#include <invoiceList.h>

dspInvoiceInformation::dspInvoiceInformation(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_invoiceNumber, SIGNAL(newInvoiceNumber(QString)), this, SLOT(sParseInvoiceNumber()));
  connect(_invoiceList, SIGNAL(clicked()), this, SLOT(sInvoiceList()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sViewDetails()));

  _invoiceAmount->setPrecision(omfgThis->moneyVal());

#ifndef Q_WS_MAC
  _invoiceList->setMaximumWidth(25);
#endif

  _cust->setReadOnly(TRUE);

  _arapply->addColumn(tr("Type"),            _dateColumn, Qt::AlignCenter,true, "doctype");
  _arapply->addColumn(tr("Doc./Ref. #"),              -1, Qt::AlignLeft,  true, "docnumber");
  _arapply->addColumn(tr("Apply Date"),      _dateColumn, Qt::AlignCenter,true, "arapply_postdate");
  _arapply->addColumn(tr("Amount"),         _moneyColumn, Qt::AlignRight, true, "arapply_applied");
  _arapply->addColumn(tr("Currency"),    _currencyColumn, Qt::AlignLeft,  true, "currabbr");
  _arapply->addColumn(tr("Base Amount"), _bigMoneyColumn, Qt::AlignRight, true, "baseapplied");

  if (omfgThis->singleCurrency())
      _arapply->hideColumn("currabbr");

  _invcheadid = -1;
}

dspInvoiceInformation::~dspInvoiceInformation()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspInvoiceInformation::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspInvoiceInformation::set(const ParameterList &pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("invoiceNumber", &valid);
  if (valid)
  {
    _invoiceNumber->setInvoiceNumber(param.toString());
    _invoiceNumber->setEnabled(FALSE);
    _invoiceList->hide();
  }

  return NoError;
}

void dspInvoiceInformation::sParseInvoiceNumber()
{
  q.prepare( "SELECT invchead_id, invchead_cust_id, invchead_ponumber,"
             "       invchead_shipdate, invchead_invcdate,"
             "       (invchead_misc_amount + invchead_freight + invchead_tax + SUM(COALESCE(round((invcitem_billed * invcitem_qty_invuomratio) * (invcitem_price / COALESCE(invcitem_price_invuomratio, 1)),2),0))) AS amount,"
             "       invchead_billto_name, invchead_billto_address1,"
             "       invchead_billto_address2, invchead_billto_address3,"
             "       invchead_billto_city, invchead_billto_state, invchead_billto_zipcode,"
             "       invchead_shipto_name, invchead_shipto_address1,"
             "       invchead_shipto_address2, invchead_shipto_address3,"
             "       invchead_shipto_city, invchead_shipto_state, invchead_shipto_zipcode,"
             "       invchead_notes "
             "FROM invchead LEFT OUTER JOIN"
             "     ( invcitem LEFT OUTER JOIN item"
             "       ON (invcitem_item_id=item_id) )"
             "     ON (invcitem_invchead_id=invchead_id) "
             "WHERE (invchead_invcnumber=:invoiceNumber) "
             "GROUP BY invchead_id, invchead_cust_id, invchead_ponumber,"
             "         invchead_shipdate, invchead_invcdate,"
             "         invchead_misc_amount, invchead_freight, invchead_tax,"
             "         invchead_billto_name, invchead_billto_address1,"
             "         invchead_billto_address2, invchead_billto_address3,"
             "         invchead_billto_city, invchead_billto_state, invchead_billto_zipcode,"
             "         invchead_shipto_name, invchead_shipto_address1,"
             "         invchead_shipto_address2, invchead_shipto_address3,"
             "         invchead_shipto_city, invchead_shipto_state, invchead_shipto_zipcode,"
             "         invchead_notes;" );
  q.bindValue(":invoiceNumber", _invoiceNumber->invoiceNumber());
  q.exec();
  if (q.first())
  {
    _print->setEnabled(TRUE);
    _view->setEnabled(TRUE);

    _invcheadid = q.value("invchead_id").toInt();

    _custPoNumber->setText(q.value("invchead_ponumber").toString());
    _cust->setId(q.value("invchead_cust_id").toInt());
    _invoiceDate->setDate(q.value("invchead_invcdate").toDate());
    _shipDate->setDate(q.value("invchead_shipdate").toDate());
    _invoiceAmount->setDouble(q.value("amount").toDouble());

    _billToName->setText(q.value("invchead_billto_name"));
    _billToAddress1->setText(q.value("invchead_billto_address1"));
    _billToAddress2->setText(q.value("invchead_billto_address2"));
    _billToAddress3->setText(q.value("invchead_billto_address3"));
    _billToCity->setText(q.value("invchead_billto_city"));
    _billToState->setText(q.value("invchead_billto_state"));
    _billToZip->setText(q.value("invchead_billto_zipcode"));

    _shipToName->setText(q.value("invchead_shipto_name"));
    _shipToAddress1->setText(q.value("invchead_shipto_address1"));
    _shipToAddress2->setText(q.value("invchead_shipto_address2"));
    _shipToAddress3->setText(q.value("invchead_shipto_address3"));
    _shipToCity->setText(q.value("invchead_shipto_city"));
    _shipToState->setText(q.value("invchead_shipto_state"));
    _shipToZip->setText(q.value("invchead_shipto_zipcode"));

    _notes->setText(q.value("invchead_notes").toString());

    q.prepare( "SELECT arapply_id,"
               "       CASE WHEN (arapply_source_doctype = 'C') THEN :creditMemo"
               "            WHEN (arapply_source_doctype = 'R') THEN :cashdeposit"
               "            WHEN (arapply_fundstype='C') THEN :check"
               "            WHEN (arapply_fundstype='T') THEN :certifiedCheck"
               "            WHEN (arapply_fundstype='M') THEN :masterCard"
               "            WHEN (arapply_fundstype='V') THEN :visa"
               "            WHEN (arapply_fundstype='A') THEN :americanExpress"
               "            WHEN (arapply_fundstype='D') THEN :discoverCard"
               "            WHEN (arapply_fundstype='R') THEN :otherCreditCard"
               "            WHEN (arapply_fundstype='K') THEN :cash"
               "            WHEN (arapply_fundstype='W') THEN :wireTransfer"
               "            WHEN (arapply_fundstype='O') THEN :other"
               "       END AS doctype,"
               "       CASE WHEN (arapply_source_doctype IN ('C','R')) THEN arapply_source_docnumber"
               "            WHEN (arapply_source_doctype = 'K') THEN arapply_refnumber"
               "            ELSE :error"
               "       END AS docnumber,"
               "       arapply_postdate, arapply_applied,"
               "       currConcat(arapply_curr_id) AS currabbr,"
               "       currToBase(arapply_curr_id, arapply_applied, arapply_postdate) AS baseapplied,"
               "       'curr' AS arapply_applied_xtnumericrole,"
               "       'curr' AS baseapplied_xtnumericrole "
               "FROM arapply "
               "WHERE ( (arapply_target_doctype='I') "
               " AND (arapply_target_docnumber=:aropen_docnumber) ) "
               "ORDER BY arapply_postdate;" );

    q.bindValue(":creditMemo", tr("C/M"));
    q.bindValue(":cashdeposit", tr("Cash Deposit"));
    q.bindValue(":error", tr("Error"));
    q.bindValue(":check", tr("Check"));
    q.bindValue(":certifiedCheck", tr("Certified Check"));
    q.bindValue(":masterCard", tr("Master Card"));
    q.bindValue(":visa", tr("Visa"));
    q.bindValue(":americanExpress", tr("American Express"));
    q.bindValue(":discoverCard", tr("Discover Card"));
    q.bindValue(":otherCreditCard", tr("Other Credit Card"));
    q.bindValue(":cash", tr("Cash"));
    q.bindValue(":wireTransfer", tr("Wire Transfer"));
    q.bindValue(":other", tr("Other"));
    q.bindValue(":aropen_docnumber", _invoiceNumber->invoiceNumber());
    q.exec();
    _arapply->clear();
    _arapply->populate(q);
    if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else
  {
    if (q.lastError().type() != QSqlError::NoError)
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    _print->setEnabled(FALSE);
    _view->setEnabled(FALSE);
    _invoiceNumber->clear();
    _arapply->clear();
    _invcheadid = -1;
  }
}

void dspInvoiceInformation::sPrint()
{
  ParameterList params;
  params.append("invchead_id", _invcheadid);

  orReport report("InvoiceInformation", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspInvoiceInformation::sViewDetails()
{
  invoice::viewInvoice(_invcheadid);
}

void dspInvoiceInformation::sInvoiceList()
{
  ParameterList params;
  params.append("invoiceNumber", _invoiceNumber->invoiceNumber());

  invoiceList newdlg(this, "", TRUE);
  newdlg.set(params);
  int invoiceid = newdlg.exec();

  if (invoiceid > 0)
    _invoiceNumber->setId(invoiceid);
}

