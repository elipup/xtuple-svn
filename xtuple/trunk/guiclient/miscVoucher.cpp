/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "miscVoucher.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QVariant>

#include "voucherMiscDistrib.h"

miscVoucher::miscVoucher(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_amountDistributed, SIGNAL(valueChanged()), this, SLOT(sPopulateBalanceDue()));
  connect(_amountToDistribute, SIGNAL(valueChanged()), this, SLOT(sPopulateBalanceDue()));
  connect(_amountToDistribute, SIGNAL(effectiveChanged(const QDate&)), this, SLOT(sFillMiscList()));
  connect(_amountToDistribute, SIGNAL(idChanged(int)), this, SLOT(sFillMiscList()));
  connect(_amountToDistribute, SIGNAL(valueChanged()), this, SLOT(sPopulateBalanceDue()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDeleteMiscDistribution()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEditMiscDistribution()));
  connect(_invoiceDate, SIGNAL(newDate(const QDate&)), this, SLOT(sPopulateDistDate()));
  connect(_invoiceDate, SIGNAL(newDate(const QDate&)), this, SLOT(sPopulateDueDate()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNewMiscDistribution()));
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_voucherNumber, SIGNAL(lostFocus()), this, SLOT(sHandleVoucherNumber()));

  _terms->setType(XComboBox::APTerms);
  _vendor->setType(__activeVendors);

  _miscDistrib->addColumn(tr("Account"), -1,           Qt::AlignLeft,   true,  "account"  );
  _miscDistrib->addColumn(tr("Amount"),  _moneyColumn, Qt::AlignRight,  true,  "vodist_amount" );
}

miscVoucher::~miscVoucher()
{
  // no need to delete child widgets, Qt does it all for us
}

void miscVoucher::languageChange()
{
  retranslateUi(this);
}

enum SetResponse miscVoucher::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      q.exec("SELECT NEXTVAL('vohead_vohead_id_seq') AS vohead_id");
      if (q.first())
        _voheadid = q.value("vohead_id").toInt();
//  ToDo

      if (_metrics->value("VoucherNumberGeneration") == "A")
      {
        populateNumber();
        _vendor->setFocus();
      }
      else
        _voucherNumber->setFocus();

      connect(_vendor, SIGNAL(newId(int)), this, SLOT(sPopulateVendorInfo(int)));
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      _voucherNumber->setEnabled(FALSE);
      _vendor->setEnabled(FALSE);

      _save->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _voucherNumber->setEnabled(FALSE);
      _vendor->setReadOnly(TRUE);
	  _taxzone->setEnabled(FALSE);
      _amountToDistribute->setEnabled(FALSE);
      _distributionDate->setEnabled(FALSE);
      _invoiceDate->setEnabled(FALSE);
      _dueDate->setEnabled(FALSE);
      _terms->setEnabled(FALSE);
      _terms->setType(XComboBox::Terms);
      _vendor->setType(__allVendors);
      _invoiceNum->setEnabled(FALSE);
      _reference->setEnabled(FALSE);
      _miscDistrib->setEnabled(FALSE);
      _new->setEnabled(FALSE);
      _flagFor1099->setEnabled(FALSE);
      _notes->setEnabled(false);
      _close->setText(tr("&Close"));
      _save->hide();

      _close->setFocus();
    }
  }

  param = pParams.value("vohead_id", &valid);
  if (valid)
  {
    _voheadid = param.toInt();
    populate();
  }

  return NoError;
}

void miscVoucher::sSave()
{
  if (!_invoiceDate->isValid())
  {
    QMessageBox::critical( this, tr("Cannot Save Voucher"),
                           tr("You must enter an Invoice Date before you may save this Voucher.") );
    _invoiceDate->setFocus();
    return;
  }

  if (!_dueDate->isValid())
  {
    QMessageBox::critical( this, tr("Cannot Save Voucher"),
                           tr("You must enter a Due Date before you may save this Voucher.") );
    _dueDate->setFocus();
    return;
  }

  if (!_distributionDate->isValid())
  {
    QMessageBox::critical( this, tr("Cannot Save Voucher"),
                           tr("You must enter a Distribution Date before you may save this Voucher.") );
    _distributionDate->setFocus();
    return;
  }

  if (_invoiceNum->text().trimmed().length() == 0)
  {
    QMessageBox::critical( this, tr("Cannot Save Voucher"),
                           tr("You must enter a Vendor Invoice Number before you may save this Voucher.") );
    _invoiceNum->setFocus();
    return;
  }

  q.prepare( "SELECT vohead_id "
             "FROM vohead "
             "WHERE ( (vohead_invcnumber=:vohead_invcnumber)"
             " AND (vohead_vend_id=:vend_id)"
             " AND (vohead_id<>:vohead_id) );" );
  q.bindValue(":vohead_invcnumber", _invoiceNum->text().trimmed());
  q.bindValue(":vend_id", _vendor->id());
  q.bindValue(":vohead_id", _voheadid);
  q.exec();
  if (q.first())
  {
      if (QMessageBox::question( this, windowTitle(),
                             tr( "A Voucher for this Vendor has already been entered with the same Vendor Invoice Number. "
                                 "Are you sure you want to use this number again?" ),
                              QMessageBox::Yes,
                              QMessageBox::No | QMessageBox::Default) == QMessageBox::No)
      {
        _invoiceNum->setFocus();
        return;
      }
  }

  if (_mode == cNew)
  {
    q.prepare( "INSERT INTO vohead "
               "( vohead_id, vohead_number, vohead_pohead_id, vohead_vend_id, vohead_taxzone_id, "
               "  vohead_terms_id, vohead_distdate, vohead_docdate, vohead_duedate,"
               "  vohead_invcnumber, vohead_reference,"
               "  vohead_amount, vohead_1099, vohead_posted, vohead_curr_id, vohead_misc, vohead_notes ) "
               "VALUES "
               "( :vohead_id, :vohead_number, -1, :vohead_vend_id, :vohead_taxzone_id, "
               "  :vohead_terms_id, :vohead_distdate, :vohead_docdate, :vohead_duedate,"
               "  :vohead_invcnumber, :vohead_reference,"
               "  :vohead_amount, :vohead_1099, FALSE, :vohead_curr_id, TRUE, :vohead_notes );" );
    q.bindValue(":vohead_number", _voucherNumber->text().toInt());
    q.bindValue(":vohead_vend_id", _vendor->id());
  }
  else
    q.prepare( "UPDATE vohead "
               "SET vohead_distdate=:vohead_distdate, vohead_docdate=:vohead_docdate, vohead_duedate=:vohead_duedate,"
               "    vohead_terms_id=:vohead_terms_id, vohead_taxzone_id=:vohead_taxzone_id, "
               "    vohead_invcnumber=:vohead_invcnumber, vohead_reference=:vohead_reference,"
               "    vohead_amount=:vohead_amount, vohead_1099=:vohead_1099, "
	       "    vohead_curr_id=:vohead_curr_id, vohead_notes=:vohead_notes "
               "WHERE (vohead_id=:vohead_id);" );

  q.bindValue(":vohead_id", _voheadid);
  q.bindValue(":vohead_terms_id", _terms->id());
  q.bindValue(":vohead_taxzone_id", _taxzone->id());
  q.bindValue(":vohead_distdate", _distributionDate->date());
  q.bindValue(":vohead_docdate", _invoiceDate->date());
  q.bindValue(":vohead_duedate", _dueDate->date());
  q.bindValue(":vohead_invcnumber", _invoiceNum->text().trimmed());
  q.bindValue(":vohead_reference", _reference->text().trimmed());
  q.bindValue(":vohead_amount", _amountToDistribute->localValue());
  q.bindValue(":vohead_1099", QVariant(_flagFor1099->isChecked()));
  q.bindValue(":vohead_curr_id", _amountToDistribute->id());
  q.bindValue(":vohead_notes", _notes->text());
  q.exec();

  omfgThis->sVouchersUpdated();

  _voheadid = -1;

  if(cNew != _mode)
  {
    close();
    return;
  }

  _voucherNumber->clear();
  _vendor->setId(-1);
  _amountToDistribute->clear();
  _amountDistributed->clear();
  _balance->clear();
  _invoiceDate->setNull();
  _distributionDate->setNull();
  _dueDate->setNull();
  _invoiceNum->clear();
  _reference->clear();
  _flagFor1099->setChecked(false);
  _miscDistrib->clear();
  _notes->setText("");
  _cachedAmountDistributed = 0.0;

  ParameterList params;
  params.append("mode", "new");
  set(params);
}

void miscVoucher::sHandleVoucherNumber()
{
  if (_voucherNumber->text().length() == 0)
  {
    if ((_metrics->value("VoucherNumberGeneration") == "A") || (_metrics->value("VoucherNumberGeneration") == "O"))
      populateNumber();
    else
    {
      QMessageBox::critical( this, tr("Enter Voucher Number"),
                             tr("You must enter a valid Voucher Number before continuing") );

      _voucherNumber->setFocus();
      return;
    }
  }
  else
  {
    q.prepare( "SELECT vohead_id "
               "FROM vohead "
               "WHERE (vohead_number=:vohead_number);" );
    q.bindValue(":vohead_number", _voucherNumber->text().toInt());
    q.exec();
    if (q.first())
    {
      _voheadid = q.value("vohead_id").toInt();

      _voucherNumber->setEnabled(FALSE);
      _vendor->setEnabled(FALSE);

      _mode = cEdit;
      populate();

      return;
    }
  }
}

void miscVoucher::sPopulateVendorInfo(int pVendid)
{
  q.prepare( "SELECT vend_terms_id, vend_1099, vend_curr_id, vend_taxzone_id "
             "FROM vend "
             "WHERE (vend_id=:vend_id);" );
  q.bindValue(":vend_id", pVendid);
  q.exec();
  if (q.first())
  {
    _terms->setId(q.value("vend_terms_id").toInt());
    _flagFor1099->setChecked(q.value("vend_1099").toBool());
    _amountToDistribute->setId(q.value("vend_curr_id").toInt());
	_taxzone->setId(q.value("vend_taxzone_id").toInt());
  }
}

void miscVoucher::sNewMiscDistribution()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("vohead_id", _voheadid);
  params.append("curr_id", _amountToDistribute->id());
  params.append("curr_effective", _amountToDistribute->effective());
  params.append("amount", _balance->localValue());
  if (_taxzone->isValid())
   params.append("taxzone_id", _taxzone->id());

  voucherMiscDistrib newdlg(this, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
  {
    sFillMiscList();
    sPopulateDistributed();
  }
}

void miscVoucher::sEditMiscDistribution()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("vodist_id", _miscDistrib->id());
  params.append("curr_id", _amountToDistribute->id());
  params.append("curr_effective", _amountToDistribute->effective());
  if (_taxzone->isValid())
   params.append("taxzone_id", _taxzone->id());

  voucherMiscDistrib newdlg(this, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
  {
    sFillMiscList();
    sPopulateDistributed();
  }
}

void miscVoucher::sDeleteMiscDistribution()
{
  q.prepare( "DELETE FROM vodist "
             "WHERE (vodist_id=:vodist_id);" );
  q.bindValue(":vodist_id", _miscDistrib->id());
  q.exec();
  sFillMiscList();
  sPopulateDistributed();
}

void miscVoucher::sFillMiscList()
{
  q.prepare( "SELECT vodist_id, (formatGLAccount(accnt_id) || ' - ' || accnt_descrip) AS account,"
             "       vodist_amount, 'curr' AS vodist_amount_xtnumericrole "
             "FROM vodist, accnt "
             "WHERE ( (vodist_poitem_id=-1)"
             " AND (vodist_accnt_id=accnt_id)"
             " AND (vodist_vohead_id=:vohead_id) ) "
             "UNION ALL "
             "SELECT vodist_id, (expcat_code || ' - ' || expcat_descrip) AS account,"
             "       vodist_amount, 'curr' AS vodist_amount_xtnumericrole "
             "  FROM vodist, expcat "
             " WHERE ( (vodist_poitem_id=-1)"
             "   AND   (vodist_expcat_id=expcat_id)"
             "   AND   (vodist_vohead_id=:vohead_id) ) "
			 "UNION ALL "
             "SELECT vodist_id, (tax_code || ' - ' || tax_descrip) AS account,"
             "       vodist_amount, 'curr' AS vodist_amount_xtnumericrole "
             "  FROM vodist, tax "
             " WHERE ( (vodist_poitem_id=-1)"
             "   AND   (vodist_tax_id=tax_id)"
             "   AND   (vodist_vohead_id=:vohead_id) ) " 
             "ORDER BY account;" );
  q.bindValue(":vohead_id", _voheadid);
  q.exec();
  _miscDistrib->populate(q);
}

void miscVoucher::sPopulateDistributed()
{
  q.prepare( "SELECT COALESCE(SUM(vodist_amount), 0) AS distrib "
             "FROM vodist "
             "WHERE (vodist_vohead_id=:vohead_id);" );
  q.bindValue(":vohead_id", _voheadid);
  q.exec();
  if (q.first())
  {
    _amountDistributed->setLocalValue(q.value("distrib").toDouble());
    sPopulateBalanceDue();
  }
}

void miscVoucher::sPopulateBalanceDue()
{
  _balance->setLocalValue(_amountToDistribute->localValue() - _amountDistributed->localValue());
  if (_balance->isZero())
    _balance->setPaletteForegroundColor(QColor("black"));
  else
    _balance->setPaletteForegroundColor(QColor("red"));
}

void miscVoucher::populateNumber()
{
  q.exec("SELECT fetchVoNumber() AS vouchernumber;");
  if (q.first())
  {
    _voucherNumber->setText(q.value("vouchernumber").toString());
    _voucherNumber->setEnabled(FALSE);
  }
}

void miscVoucher::populate()
{
  XSqlQuery vohead;
  vohead.prepare( "SELECT vohead_number, vohead_vend_id, vohead_taxzone_id, vohead_terms_id,"
                  "       vohead_distdate, vohead_docdate, vohead_duedate,"
                  "       vohead_invcnumber, vohead_reference,"
                  "       vohead_1099, vohead_amount, vohead_curr_id, vohead_notes "
                  "FROM vohead "
                  "WHERE (vohead_id=:vohead_id);" );
  vohead.bindValue(":vohead_id", _voheadid);
  vohead.exec();
  if (vohead.first())
  {
    _voucherNumber->setText(vohead.value("vohead_number").toString());
    _vendor->setId(vohead.value("vohead_vend_id").toInt());
	_taxzone->setId(vohead.value("vohead_taxzone_id").toInt());
    _amountToDistribute->set(vohead.value("vohead_amount").toDouble(),
			     vohead.value("vohead_curr_id").toInt(),
			     vohead.value("vohead_docdate").toDate(), false);
    _terms->setId(vohead.value("vohead_terms_id").toInt());

    _distributionDate->setDate(vohead.value("vohead_distdate").toDate(), true);
    _invoiceDate->setDate(vohead.value("vohead_docdate").toDate());
    _dueDate->setDate(vohead.value("vohead_duedate").toDate());
    _invoiceNum->setText(vohead.value("vohead_invcnumber").toString());
    _reference->setText(vohead.value("vohead_reference").toString());
    _flagFor1099->setChecked(vohead.value("vohead_1099").toBool());
    _notes->setText(vohead.value("vohead_notes").toString());

    sFillMiscList();
    sPopulateDistributed();
  }
}

void miscVoucher::clear()
{
}

void miscVoucher::closeEvent(QCloseEvent *pEvent)
{
  if ( (_mode == cNew) && (_voheadid != -1) )
  {
    q.prepare( "DELETE FROM vohead "
               "WHERE (vohead_id=:vohead_id);"

               "DELETE FROM vodist "
               "WHERE (vodist_vohead_id=:vohead_id);"

               "SELECT releaseVoNumber(:voucherNumber);" );
    q.bindValue(":vohead_id", _voheadid);
    q.bindValue(":voucherNumber", _voucherNumber->text().toInt());
    q.exec();
  }

  pEvent->accept();
}

void miscVoucher::sPopulateDistDate()
{
  if ( (_invoiceDate->isValid()) && (!_distributionDate->isValid()) )
  {
    _distributionDate->setDate(_invoiceDate->date(), true);
    sPopulateDueDate();
  }
}

void miscVoucher::sPopulateDueDate()
{
  if ( (_invoiceDate->isValid()) && (!_dueDate->isValid()) )
  {
    q.prepare("SELECT determineDueDate(:terms_id, :invoiceDate) AS duedate;");
    q.bindValue(":terms_id", _terms->id());
    q.bindValue(":invoiceDate", _invoiceDate->date());
    q.exec();
    if (q.first())
      _dueDate->setDate(q.value("duedate").toDate());
  }
}
 
void miscVoucher::keyPressEvent( QKeyEvent * e )
{
  if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
  {
    _save->animateClick();
    e->accept();
  }
  else
    e->ignore();
}

