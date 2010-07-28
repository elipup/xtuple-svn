/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "configureGL.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>

#include "configureEncryption.h"
#include "guiclient.h"

configureGL::configureGL(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  // AP
  _nextAPMemoNumber->setValidator(omfgThis->orderVal());
  q.exec("SELECT currentAPMemoNumber() AS result;");
  if (q.first())
    _nextAPMemoNumber->setText(q.value("result"));

  _achGroup->setVisible(_metrics->boolean("ACHSupported"));
  if (_metrics->boolean("ACHSupported"))
  {
    _achGroup->setChecked(_metrics->boolean("ACHEnabled"));
    _nextACHBatchNumber->setValidator(omfgThis->orderVal());
    if (! _metrics->value("ACHCompanyId").trimmed().isEmpty())
      _companyId->setText(_metrics->value("ACHCompanyId"));
    if (! _metrics->value("ACHCompanyIdType").trimmed().isEmpty())
    {
      if (_metrics->value("ACHCompanyIdType").trimmed() == "D")
        _companyIdIsDUNS->setChecked(true);
      else if (_metrics->value("ACHCompanyIdType").trimmed() == "E")
        _companyIdIsEIN->setChecked(true);
      else if (_metrics->value("ACHCompanyIdType").trimmed() == "O")
        _companyIdIsOther->setChecked(true);
    }
    if (! _metrics->value("ACHCompanyName").trimmed().isEmpty())
      _companyName->setText(_metrics->value("ACHCompanyName"));

    QString eftsuffix = _metrics->value("ACHDefaultSuffix").trimmed();
    QString eftRregex = _metrics->value("EFTRoutingRegex").trimmed();
    QString eftAregex = _metrics->value("EFTAccountRegex").trimmed();
    QString eftproc   = _metrics->value("EFTFunction").trimmed();
    if (eftsuffix.isEmpty())
      _eftAch->setChecked(true);
    else if (eftsuffix == ".ach" &&
             eftRregex == _eftAchRoutingRegex->text() &&
             eftAregex == _eftAchAccountRegex->text() &&
             eftproc   == _eftAchFunction->text())
      _eftAch->setChecked(true);
    else if (eftsuffix == ".aba" &&
             eftRregex == _eftAbaRoutingRegex->text() &&
             eftAregex == _eftAbaAccountRegex->text() &&
             eftproc   == _eftAbaFunction->text())
      _eftAba->setChecked(true);
    else
    {
      _eftCustom->setChecked(true);
      _eftCustomRoutingRegex->setText(eftRregex);
      _eftCustomAccountRegex->setText(eftAregex);
      _eftCustomFunction->setText(eftproc);

       int suffixidx = _eftCustomSuffix->findText(_metrics->value("ACHDefaultSuffix"));
      if (suffixidx < 0)
      {
        _eftCustomSuffix->insertItem(0, _metrics->value("ACHDefaultSuffix"));
        _eftCustomSuffix->setCurrentIndex(0);
      }
      else
        _eftCustomSuffix->setCurrentIndex(suffixidx);
    }

    q.exec("SELECT currentNumber('ACHBatch') AS result;");
    if (q.first())
      _nextACHBatchNumber->setText(q.value("result"));
  }
  _reqInvoiceReg->setChecked(_metrics->boolean("ReqInvRegVoucher"));
  _reqInvoiceMisc->setChecked(_metrics->boolean("ReqInvMiscVoucher"));
    
  // AR
  _nextARMemoNumber->setValidator(omfgThis->orderVal());
  _nextCashRcptNumber->setValidator(omfgThis->orderVal());

  q.exec("SELECT currentARMemoNumber() AS result;");
  if (q.first())
    _nextARMemoNumber->setText(q.value("result"));
  else if (q.lastError().type() != QSqlError::NoError)
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);

  q.exec("SELECT currentCashRcptNumber() AS result;");
  if (q.first())
    _nextCashRcptNumber->setText(q.value("result"));
  else if (q.lastError().type() != QSqlError::NoError)
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);

  _hideApplyto->setChecked(_metrics->boolean("HideApplyToBalance"));
  _customerDeposits->setChecked(_metrics->boolean("EnableCustomerDeposits"));
  
  _name->setText(_metrics->value("remitto_name"));
  _address->setLine1(_metrics->value("remitto_address1"));
  _address->setLine2(_metrics->value("remitto_address2"));
  _address->setLine3(_metrics->value("remitto_address3"));
  _address->setCity(_metrics->value("remitto_city"));
  _address->setState(_metrics->value("remitto_state"));
  _address->setPostalCode(_metrics->value("remitto_zipcode"));
  _address->setCountry(_metrics->value("remitto_country"));
  _phone->setText(_metrics->value("remitto_phone"));

  _warnLate->setChecked(_metrics->boolean("AutoCreditWarnLateCustomers"));
  if(!_metrics->value("DefaultAutoCreditWarnGraceDays").isEmpty())
    _graceDays->setValue(_metrics->value("DefaultAutoCreditWarnGraceDays").toInt());
  _incdtCategory->setId(_metrics->value("DefaultARIncidentStatus").toInt());
  _closeARIncdt->setChecked(_metrics->boolean("AutoCloseARIncident"));
  
  // GL
  _mainSize->setValue(_metrics->value("GLMainSize").toInt());

  bool extConsolAllowed = _metrics->value("Application") != "PostBooks";
  _externalConsolidation->setVisible(extConsolAllowed);
  if (_metrics->value("GLCompanySize").toInt() == 0)
  {
    _useCompanySegment->setChecked(FALSE);
    _externalConsolidation->setChecked(FALSE);
  }
  else
  {
    _useCompanySegment->setChecked(TRUE);
    _companySegmentSize->setValue(_metrics->value("GLCompanySize").toInt());

    _externalConsolidation->setChecked(_metrics->boolean("MultiCompanyFinancialConsolidation") &&
                                       extConsolAllowed);
  }

  if (_metrics->value("GLProfitSize").toInt() == 0)
    _useProfitCenters->setChecked(FALSE);
  else
  {
    _useProfitCenters->setChecked(TRUE);
    _profitCenterSize->setValue(_metrics->value("GLProfitSize").toInt());
    _ffProfitCenters->setChecked(_metrics->boolean("GLFFProfitCenters"));
  }

  if (_metrics->value("GLSubaccountSize").toInt() == 0)
    _useSubaccounts->setChecked(FALSE);
  else
  {
    _useSubaccounts->setChecked(TRUE);
    _subaccountSize->setValue(_metrics->value("GLSubaccountSize").toInt());
    _ffSubaccounts->setChecked(_metrics->boolean("GLFFSubaccounts"));
  }

  _yearend->setId(_metrics->value("YearEndEquityAccount").toInt());

  _gainLoss->setId(_metrics->value("CurrencyGainLossAccount").toInt());
  switch(_metrics->value("CurrencyExchangeSense").toInt())
  {
    case 1:
      _localToBase->setChecked(TRUE);
      break;
    case 0:
    default:
      _baseToLocal->setChecked(TRUE);
  }

  _discrepancy->setId(_metrics->value("GLSeriesDiscrepancyAccount").toInt());

  _mandatoryNotes->setChecked(_metrics->boolean("MandatoryGLEntryNotes"));
  _manualFwdUpdate->setChecked(_metrics->boolean("ManualForwardUpdate"));
  _taxauth->setId(_metrics->value("DefaultTaxAuthority").toInt());

  _recurringBuffer->setValue(_metrics->value("RecurringInvoiceBuffer").toInt());

  if (_metrics->boolean("UseSubLedger"))
  {
    _subLedger->setChecked(true);
    XSqlQuery qry;
    qry.exec("SELECT count(sltrans_id) > 0 AS result FROM sltrans WHERE (NOT sltrans_posted);");
    qry.first();
    if (qry.value("result").toBool())
      _postGroup->setEnabled(false);
  }
  else
    _generalLedger->setChecked(true);
  
  adjustSize();
}

configureGL::~configureGL()
{
  // no need to delete child widgets, Qt does it all for us
}

void configureGL::languageChange()
{
  retranslateUi(this);
}

void configureGL::sSave()
{
  emit saving();

  if (_metrics->boolean("ACHSupported"))
  {
    QString tmpCompanyId = _companyId->text();
    struct {
      bool    condition;
      QString msg;
      QWidget *widget;
    } error[] = {
      { _achGroup->isChecked() && _companyId->text().isEmpty(),
        tr("Please enter a default Company Id if you are going to create "
           "ACH files."),
        _companyId },
      { _achGroup->isChecked() &&
        (_companyIdIsEIN->isChecked() || _companyIdIsDUNS->isChecked()) && 
        tmpCompanyId.remove("-").size() != 9,
        tr("EIN, TIN, and DUNS numbers are all 9 digit numbers. Other "
           "characters (except dashes for readability) are not allowed."),
        _companyId },
      { _achGroup->isChecked() &&
        _companyIdIsOther->isChecked() && _companyId->text().size() > 10,
        tr("Company Ids must be 10 characters or shorter (not counting dashes "
           "in EIN's, TIN's, and DUNS numbers)."),
        _companyId },
      { _achGroup->isChecked() &&
        ! (_companyIdIsEIN->isChecked() || _companyIdIsDUNS->isChecked() ||
           _companyIdIsOther->isChecked()),
        tr("Please mark whether the Company Id is an EIN, TIN, DUNS number, "
           "or Other."),
        _companyIdIsEIN }
    };
    for (unsigned int i = 0; i < sizeof(error) / sizeof(error[0]); i++)
      if (error[i].condition)
      {
        QMessageBox::critical(this, tr("Cannot Save Accounting Configuration"),
                              error[i].msg);
        error[i].widget->setFocus();
        return;
      }
  }

  // AP
  q.prepare("SELECT setNextAPMemoNumber(:armemo_number) AS result;");
  q.bindValue(":armemo_number", _nextAPMemoNumber->text().toInt());
  q.exec();

  if (_metrics->boolean("ACHSupported"))
  {
    _metrics->set("ACHEnabled",           _achGroup->isChecked());
    if (_achGroup->isChecked())
    {
      _metrics->set("ACHCompanyId",     _companyId->text().trimmed());
      if (_companyId->text().trimmed().length() > 0)
      {
        if (_companyIdIsDUNS->isChecked())
          _metrics->set("ACHCompanyIdType", QString("D"));
        else if (_companyIdIsEIN->isChecked())
          _metrics->set("ACHCompanyIdType", QString("E"));
        else if (_companyIdIsOther->isChecked())
          _metrics->set("ACHCompanyIdType", QString("O"));
      }
      _metrics->set("ACHCompanyName",   _companyName->text().trimmed());

      if (_eftAch->isChecked())
      {
        _metrics->set("ACHDefaultSuffix", _eftAchSuffix->text().trimmed());
        _metrics->set("EFTRoutingRegex",  _eftAchRoutingRegex->text());
        _metrics->set("EFTAccountRegex",  _eftAchAccountRegex->text());
        _metrics->set("EFTFunction",      _eftAchFunction->text());
      }
      else if (_eftAba->isChecked())
      {
        _metrics->set("ACHDefaultSuffix", _eftAbaSuffix->text().trimmed());
        _metrics->set("EFTRoutingRegex",  _eftAbaRoutingRegex->text());
        _metrics->set("EFTAccountRegex",  _eftAbaAccountRegex->text());
        _metrics->set("EFTFunction",      _eftAbaFunction->text());
      }
      else
      {
        _metrics->set("ACHDefaultSuffix", _eftCustomSuffix->currentText().trimmed());
        _metrics->set("EFTRoutingRegex",  _eftCustomRoutingRegex->text().trimmed());
        _metrics->set("EFTAccountRegex",  _eftCustomAccountRegex->text().trimmed());
        _metrics->set("EFTFunction",      _eftCustomFunction->text().trimmed());
      }

      q.prepare("SELECT setNextNumber('ACHBatch', :number) AS result;");
      q.bindValue(":number", _nextACHBatchNumber->text().toInt());
      q.exec();
    }
  }
  _metrics->set("ReqInvRegVoucher", _reqInvoiceReg->isChecked());
  _metrics->set("ReqInvMiscVoucher", _reqInvoiceMisc->isChecked());
  
  // AR
  q.prepare("SELECT setNextARMemoNumber(:armemo_number) AS result;");
  q.bindValue(":armemo_number", _nextARMemoNumber->text().toInt());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  q.prepare("SELECT setNextCashRcptNumber(:cashrcpt_number) AS result;");
  q.bindValue(":cashrcpt_number", _nextCashRcptNumber->text().toInt());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  _metrics->set("HideApplyToBalance", _hideApplyto->isChecked());
  _metrics->set("EnableCustomerDeposits", _customerDeposits->isChecked());

  _metrics->set("remitto_name", 	_name->text().trimmed());
  _metrics->set("remitto_address1",	_address->line1().trimmed());
  _metrics->set("remitto_address2",	_address->line2().trimmed());
  _metrics->set("remitto_address3",	_address->line3().trimmed());
  _metrics->set("remitto_city",		_address->city().trimmed());
  _metrics->set("remitto_state",	_address->state().trimmed());
  _metrics->set("remitto_zipcode",	_address->postalCode().trimmed());
  _metrics->set("remitto_country",	_address->country().trimmed());
  _metrics->set("remitto_phone",	_phone->text().trimmed());
  
  _metrics->set("AutoCreditWarnLateCustomers", _warnLate->isChecked());
  if(_warnLate->isChecked())
    _metrics->set("DefaultAutoCreditWarnGraceDays", _graceDays->value());

  _metrics->set("RecurringInvoiceBuffer", _recurringBuffer->value());
  _metrics->set("DefaultARIncidentStatus", _incdtCategory->id());
  _metrics->set("AutoCloseARIncident", _closeARIncdt->isChecked());
  
  // GL
  QAction *profitcenter = omfgThis->findChild<QAction*>("gl.profitCenterNumber");
  QAction *subaccounts  = omfgThis->findChild<QAction*>("gl.subaccountNumbers");
  QAction *companyseg   = omfgThis->findChild<QAction*>("gl.companies");

  _metrics->set("GLMainSize", _mainSize->value());

  if (_useCompanySegment->isChecked())
  {
    _metrics->set("GLCompanySize", _companySegmentSize->value());
    _metrics->set("MultiCompanyFinancialConsolidation", _externalConsolidation->isChecked());
  }
  else
  {
    _metrics->set("GLCompanySize", 0);
    _metrics->set("MultiCompanyFinancialConsolidation", 0);
  }
  if(companyseg)
    companyseg->setEnabled(_useCompanySegment->isChecked());

  if (_useProfitCenters->isChecked())
  {
    _metrics->set("GLProfitSize", _profitCenterSize->value());
    _metrics->set("GLFFProfitCenters", _ffProfitCenters->isChecked());
    if(profitcenter)
      profitcenter->setEnabled(_privileges->check("MaintainChartOfAccounts"));
  }
  else
  {
    _metrics->set("GLProfitSize", 0);
    _metrics->set("GLFFProfitCenters", FALSE);
    if(profitcenter)
      profitcenter->setEnabled(FALSE);
  }

  if (_useSubaccounts->isChecked())
  {
    _metrics->set("GLSubaccountSize", _subaccountSize->value());
    _metrics->set("GLFFSubaccounts", _ffSubaccounts->isChecked());
    if(subaccounts)
      subaccounts->setEnabled(_privileges->check("MaintainChartOfAccounts"));

  }
  else
  {
    _metrics->set("GLSubaccountSize", 0);
    _metrics->set("GLFFSubaccounts", FALSE);
    if(subaccounts)
      subaccounts->setEnabled(FALSE);
  }

  _metrics->set("UseSubLedger", _subLedger->isChecked());
  _metrics->set("YearEndEquityAccount", _yearend->id());

  //if (! omfgThis->singleCurrency())
  //{
      _metrics->set("CurrencyGainLossAccount", _gainLoss->id());
      if(_localToBase->isChecked())
        _metrics->set("CurrencyExchangeSense", 1);
      else // if(_baseToLocal->isChecked())
        _metrics->set("CurrencyExchangeSense", 0);
  //}

  _metrics->set("GLSeriesDiscrepancyAccount", _discrepancy->id());
  _metrics->set("MandatoryGLEntryNotes", _mandatoryNotes->isChecked());
  _metrics->set("ManualForwardUpdate", _manualFwdUpdate->isChecked());
  _metrics->set("DefaultTaxAuthority", _taxauth->id());

  omfgThis->sConfigureGLUpdated();

  if (_metrics->boolean("ACHSupported") && _metrics->boolean("ACHEnabled") &&
      omfgThis->_key.isEmpty())
  {
    if (_privileges->check("ConfigureEncryption"))
    {
      if (QMessageBox::question(this, tr("Set Encryption?"),
                                tr("Your encryption key is not set. You will "
                                   "not be able to configure electronic "
                                   "checking information for Vendors until you "
                                   "configure encryption. Would you like to do "
                                   "this now?"),
                                    QMessageBox::Yes | QMessageBox::Default,
                                    QMessageBox::No ) == QMessageBox::Yes)
        configureEncryption(this, "", TRUE).exec();
    }
    else
      QMessageBox::question(this, tr("Set Encryption?"),
                            tr("Your encryption key is not set. You will "
                               "not be able to configure electronic "
                               "checking information for Vendors until the "
                               "system is configured to perform encryption."));
  }
}
