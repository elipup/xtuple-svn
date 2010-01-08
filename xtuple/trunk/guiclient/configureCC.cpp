/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "configureCC.h"

#include <QMessageBox>

#include "configureEncryption.h"
#include "creditcardprocessor.h"

// Change these definitions to match the indices in the _ccCompany combo box
#define ANINDEX 0
#define YPINDEX 1
#define EXTINDEX 2

configureCC::configureCC(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_anDuplicateWindow, SIGNAL(valueChanged(int)), this, SLOT(sDuplicateWindow(int)));
  connect(_ccCompany, SIGNAL(currentIndexChanged(int)), this, SLOT(sCCCompanyChanged(int)));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_save,  SIGNAL(clicked()), this, SLOT(sSave()));

  _enableChargePreauth->setVisible(false);
  _enableCredit->setVisible(false);

  if (_metricsenc == 0)
  {
    QMessageBox::critical( this, tr("Cannot Read Configuration"),
		    tr("<p>Cannot read encrypted information from database."));
  }

  _ccDefaultBank->setType(XComboBox::ARBankAccounts);

  _ccAccept->setChecked(_metrics->boolean("CCAccept"));
  _ccTest->setChecked(_metrics->boolean("CCTest"));
  _ccValidDays->setValue(_metrics->value("CCValidDays").toInt());
  
  _ccCompany->setCurrentIndex(_ccCompany->findText(_metrics->value("CCCompany")));
  _ccWidgetStack->setCurrentIndex(_ccCompany->currentIndex());
  sCCCompanyChanged(_ccCompany->currentIndex());

  _ccServer->setText(_metrics->value("CCServer"));
  _ccPort->setText(_metrics->value("CCPort"));

  _ccUseProxyServer->setChecked(_metrics->boolean("CCUseProxyServer"));
  _ccProxyServer->setText(_metrics->value("CCProxyServer"));
  _ccProxyPort->setText(_metrics->value("CCProxyPort"));

  _ccDefaultBank->setId(_metrics->value("CCDefaultBank").toInt());

  if (_metrics->value("CCANVer").isEmpty())
    _anVersion->setCurrentText("3.1");
  else
    _anVersion->setCurrentText(_metrics->value("CCANVer"));
  _anDelim->setText(_metrics->value("CCANDelim"));
  _anEncap->setText(_metrics->value("CCANEncap"));
  _anDuplicateWindow->setValue(_metrics->value("CCANDuplicateWindow").toInt());

  _anMD5Hash->setText(_metrics->value("CCANMD5Hash"));
  _anMD5HashSetOnGateway->setChecked(_metrics->boolean("CCANMD5HashSetOnGateway"));
  _anMD5HashWarn->setChecked(_metrics->value("CCANMD5HashAction") == "W");
  _anMD5HashFail->setChecked(_metrics->value("CCANMD5HashAction") == "F");
  
  if (_metrics->value("CCANCurrency") == "TRANS")
    _anCurrTransaction->setChecked(true);
  else if (! _metrics->value("CCANCurrency").isEmpty())
  {
    _anCurrFixed->setChecked(true);
    _anCurrFixedValue->setId(_metrics->value("CCANCurrency").toInt());
  }

  _anUsingWellsFargoSecureSource->setChecked(_metrics->boolean("CCANWellsFargoSecureSource"));

  _ccYPWinPathPEM->setText(_metrics->value("CCYPWinPathPEM"));
  _ccYPLinPathPEM->setText(_metrics->value("CCYPLinPathPEM"));
  _ccYPMacPathPEM->setText(_metrics->value("CCYPMacPathPEM"));
  _ccYPLinkShield->setChecked(_metrics->boolean("CCYPLinkShield"));
  _ccYPLinkShieldMax->setValue(_metrics->value("CCYPLinkShieldMax").toInt());

  _confirmPreauth->setChecked(_metrics->boolean("CCConfirmPreauth"));
  _confirmCharge->setChecked(_metrics->boolean("CCConfirmCharge"));
  _confirmChargePreauth->setChecked(_metrics->boolean("CCConfirmChargePreauth"));
  _confirmCredit->setChecked(_metrics->boolean("CCConfirmCredit"));

  _enablePreauth->setChecked(_metrics->boolean("CCEnablePreauth"));
  _enableCharge->setChecked(_metrics->boolean("CCEnableCharge"));
  _enableChargePreauth->setChecked(_metrics->boolean("CCEnableChargePreauth"));
  _enableCredit->setChecked(_metrics->boolean("CCEnableCredit"));

  _printReceipt->setChecked(_metrics->boolean("CCPrintReceipt"));

  _cvvRequired->setChecked(_metrics->boolean("CCRequireCVV"));
  QString str = _metrics->value("CCCVVCheck");
  if (str == "F")
    _cvvReject->setChecked(true);
  else if (str == "W")
    _cvvWarn->setChecked(true);
  else // if (str == "X")
    _cvvNone->setChecked(true);

  str = _metrics->value("CCCVVErrors");
  _cvvNotMatch->setChecked(str.contains("N"));
  _cvvNotProcessed->setChecked(str.contains("P"));
  _cvvNotOnCard->setChecked(str.contains("S"));
  _cvvInvalidIssuer->setChecked(str.contains("U"));

  str = _metrics->value("CCAvsCheck");
  if (str == "F")
    _avsReject->setChecked(true);
  else if (str == "W")
    _avsWarn->setChecked(true);
  else // if (str == "X")
    _avsNone->setChecked(true);

  str = _metrics->value("CCAvsAddr");
  _avsAddrNotMatch->setChecked(str.contains("N"));
  _avsAddrNotAvail->setChecked(str.contains("X"));

  str = _metrics->value("CCAvsZIP");
  _avsZIPNotMatch->setChecked(str.contains("N"));
  _avsZIPNotAvail->setChecked(str.contains("X"));

  str = _metrics->value("CCTestResult");
  if (str == "F")
    _testsAllFail->setChecked(true);
  else if (str == "S")
    _testsSomeFail->setChecked(true);
  else // if (str == "P")
    _testsAllPass->setChecked(true);

  if (0 != _metricsenc)
  {
    _ccLogin->setText(_metricsenc->value("CCLogin"));
    _ccPassword->setText(_metricsenc->value("CCPassword"));
    _ccProxyLogin->setText(_metricsenc->value("CCProxyLogin"));
    _ccProxyPassword->setText(_metricsenc->value("CCProxyPassword"));
    _ccYPStoreNum->setText(_metricsenc->value("CCYPStoreNum"));
    _anMD5Hash->setText(_metricsenc->value("CCANMD5Hash"));
  }
  else
  {
    _ccLogin->setEnabled(false);
    _ccPassword->setEnabled(false);
    _ccProxyLogin->setEnabled(false);
    _ccProxyPassword->setEnabled(false);
    _ccYPStoreNum->setEnabled(false);
    _anMD5Hash->setEnabled(false);
  }

  sDuplicateWindow(_anDuplicateWindow->value());

  QWidget *encryption = new configureEncryption(this);
  encryption->setName("_encryption");
  _keyPage->layout()->addWidget(encryption);
  QPushButton *encbutton = encryption->findChild<QPushButton*>("_save");
  if (encbutton)
    encbutton->hide();
  encbutton = encryption->findChild<QPushButton*>("_close");
  if (encbutton)
    encbutton->hide();
  encryption->show();
}

configureCC::~configureCC()
{
  // no need to delete child widgets, Qt does it all for us
}

void configureCC::languageChange()
{
  retranslateUi(this);
}

void configureCC::sSave()
{
  _metrics->set("CCAccept",          _ccAccept->isChecked());
  _metrics->set("CCTest",            _ccTest->isChecked());
  _metrics->set("CCValidDays",       _ccValidDays->value());
  _metrics->set("CCCompany",         _ccCompany->currentText());
  _metrics->set("CCServer",          _ccServer->text());
  _metrics->set("CCPort",            _ccPort->text());
  _metrics->set("CCUseProxyServer",  _ccUseProxyServer->isChecked());
  _metrics->set("CCProxyServer",     _ccProxyServer->text());
  _metrics->set("CCProxyPort",       _ccProxyPort->text());
  _metrics->set("CCDefaultBank",     _ccDefaultBank->id());

  _metrics->set("CCANVer",               _anVersion->currentText());
  _metrics->set("CCANDelim",             _anDelim->text());
  _metrics->set("CCANEncap",             _anEncap->text());
  _metrics->set("CCANDuplicateWindow",   _anDuplicateWindow->cleanText());
  _metrics->set("CCANMD5HashSetOnGateway", _anMD5HashSetOnGateway->isChecked());
  if (_anMD5HashWarn->isChecked())
    _metrics->set("CCANMD5HashAction", QString("W"));
  else if (_anMD5HashFail->isChecked())
    _metrics->set("CCANMD5HashAction", QString("F"));

  if (_anCurrFixed->isChecked())
    _metrics->set("CCANCurrency", _anCurrFixedValue->id());
  else // if (_anCurrTransaction->isChecked())
    _metrics->set("CCANCurrency", QString("TRANS"));
  _metrics->set("CCANWellsFargoSecureSource", _anUsingWellsFargoSecureSource->isChecked());

  _metrics->set("CCYPWinPathPEM",    _ccYPWinPathPEM->text());
  _metrics->set("CCYPLinPathPEM",    _ccYPLinPathPEM->text());
  _metrics->set("CCYPMacPathPEM",    _ccYPMacPathPEM->text());
  _metrics->set("CCYPLinkShield",    _ccYPLinkShield->isChecked());
  _metrics->set("CCYPLinkShieldMax", _ccYPLinkShieldMax->text());

  _metrics->set("CCConfirmPreauth",       _confirmPreauth->isChecked());
  _metrics->set("CCConfirmCharge",        _confirmCharge->isChecked());
  _metrics->set("CCConfirmChargePreauth", _confirmChargePreauth->isChecked());
  _metrics->set("CCConfirmCredit",        _confirmCredit->isChecked());

  _metrics->set("CCEnablePreauth",       _enablePreauth->isChecked());
  _metrics->set("CCEnableCharge",        _enableCharge->isChecked());
  _metrics->set("CCEnableChargePreauth", _enableChargePreauth->isChecked());
  _metrics->set("CCEnableCredit",        _enableCredit->isChecked());

  _metrics->set("CCPrintReceipt",        _printReceipt->isChecked());

  _metrics->set("CCRequireCVV", _cvvRequired->isChecked());
  if(_cvvNone->isChecked())
    _metrics->set("CCCVVCheck", QString("X"));
  else if(_cvvWarn->isChecked())
    _metrics->set("CCCVVCheck", QString("W"));
  else if(_cvvReject->isChecked())
    _metrics->set("CCCVVCheck", QString("F"));

  if(_avsNone->isChecked())
    _metrics->set("CCAvsCheck", QString("X"));
  else if(_avsWarn->isChecked())
    _metrics->set("CCAvsCheck", QString("W"));
  else if(_avsReject->isChecked())
    _metrics->set("CCAvsCheck", QString("F"));

  QString str;
  if (_cvvNotMatch->isChecked())
    str += "N";
  if (_cvvNotProcessed->isChecked())
    str += "P";
  if (_cvvNotOnCard->isChecked())
    str += "S";
  if (_cvvInvalidIssuer->isChecked())
    str += "U";
  _metrics->set("CCCVVErrors", str);

  if (_avsAddrNotMatch->isChecked() && _avsAddrNotAvail->isChecked())
    _metrics->set("CCAvsAddr", QString("NX"));
  else if (_avsAddrNotMatch->isChecked())
    _metrics->set("CCAvsAddr", QString("N"));
  else if (_avsAddrNotAvail->isChecked())
    _metrics->set("CCAvsAddr", QString("X"));

  if (_avsZIPNotMatch->isChecked() && _avsZIPNotAvail->isChecked())
    _metrics->set("CCAvsZIP", QString("NX"));
  else if (_avsZIPNotMatch->isChecked())
    _metrics->set("CCAvsZIP", QString("N"));
  else if (_avsZIPNotAvail->isChecked())
    _metrics->set("CCAvsZIP", QString("X"));

  if(_testsAllFail->isChecked())
    _metrics->set("CCTestResult", QString("F"));
  else if(_testsSomeFail->isChecked())
    _metrics->set("CCTestResult", QString("S"));
  else if(_testsAllPass->isChecked())
    _metrics->set("CCTestResult", QString("P"));

  _metrics->load();

  if (0 != _metricsenc)
  {
    _metricsenc->set("CCLogin",         _ccLogin->text());
    _metricsenc->set("CCPassword",      _ccPassword->text());
    _metricsenc->set("CCProxyLogin",    _ccProxyLogin->text());
    _metricsenc->set("CCProxyPassword", _ccProxyPassword->text());
    _metricsenc->set("CCYPStoreNum",    _ccYPStoreNum->text());
    _metricsenc->set("CCANMD5Hash",     _anMD5Hash->text());

    _metricsenc->load();
  }

  CreditCardProcessor *cardproc =
		  CreditCardProcessor::getProcessor(_ccCompany->currentText());
  if (! cardproc)
  {
    QMessageBox::warning(this, tr("Error getting Credit Card Processor"),
			 tr("<p>Internal error finding the right Credit Card "
			    "Processor. The application saved what it could "
			    "but you should re-open this window and double-"
			    "check all of the settings before continuing."));
  }
  else if (cardproc && cardproc->testConfiguration() != 0)
  {
    if (QMessageBox::question(this, tr("Invalid Credit Card Configuration"),
			      tr("<p>The configuration has been saved but "
				 "at least one configuration option appears "
				 "to be invalid:<p>%1"
				 "<p>Would you like to fix it now?")
			      .arg(cardproc->errorMsg()),
			      QMessageBox::Yes | QMessageBox::Default,
			      QMessageBox::No) == QMessageBox::Yes)
      return;
  }

  configureEncryption *encryption = _keyPage->findChild<configureEncryption*>("_encryption");
  if (encryption)
    encryption->sSave();

  accept();
}

void configureCC::sDuplicateWindow(int p)
{
  QTime time;
  time.addSecs(p);
  _anDuplicateWindowAsHMS->setText(time.toString("HH:mm:ss"));
}

void configureCC::sCCCompanyChanged(const int pindex)
{
  if (pindex == ANINDEX)
  {
    _fraudDetectionIgnoredLit->setText(tr("For Authorize.Net please configure "
                                          "CVV and AVS using the Merchant "
                                          "Interface."));
    _ccPasswordLit->setText(tr("Transaction Key:"));
    _cvvCheckGroup->setEnabled(false);
    _cvvFailGroup->setEnabled(false);
    _avsCheckGroup->setEnabled(false);
    _avsFailGroup->setEnabled(false);
  }
  else if (pindex == YPINDEX)
  {
    _fraudDetectionIgnoredLit->setText("");
    _ccPasswordLit->setText(tr("Password:"));
    _cvvCheckGroup->setEnabled(true);
    _cvvFailGroup->setEnabled(true);
    _avsCheckGroup->setEnabled(true);
    _avsFailGroup->setEnabled(true);
  }
  else if (pindex == EXTINDEX)
  {
  }
}
