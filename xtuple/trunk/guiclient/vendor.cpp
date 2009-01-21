/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "vendor.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include <openreports.h>

#include "addresscluster.h"
#include "comment.h"
#include "storedProcErrorLookup.h"
#include "taxRegistration.h"
#include "vendorAddress.h"
#include "xcombobox.h"

vendor::vendor(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_printAddresses, SIGNAL(clicked()), this, SLOT(sPrintAddresses()));
  connect(_newAddress, SIGNAL(clicked()), this, SLOT(sNewAddress()));
  connect(_editAddress, SIGNAL(clicked()), this, SLOT(sEditAddress()));
  connect(_viewAddress, SIGNAL(clicked()), this, SLOT(sViewAddress()));
  connect(_deleteAddress, SIGNAL(clicked()), this, SLOT(sDeleteAddress()));
  connect(_deleteTaxreg, SIGNAL(clicked()), this, SLOT(sDeleteTaxreg()));
  connect(_editTaxreg,   SIGNAL(clicked()), this, SLOT(sEditTaxreg()));
  connect(_newTaxreg,    SIGNAL(clicked()), this, SLOT(sNewTaxreg()));
  connect(_viewTaxreg,   SIGNAL(clicked()), this, SLOT(sViewTaxreg()));
  connect(_next, SIGNAL(clicked()), this, SLOT(sNext()));
  connect(_previous, SIGNAL(clicked()), this, SLOT(sPrevious()));
  connect(_mainButton, SIGNAL(clicked()), this, SLOT(sHandleButtons()));
  connect(_altButton, SIGNAL(clicked()), this, SLOT(sHandleButtons()));
  connect(_poButton, SIGNAL(clicked()), this, SLOT(sHandleButtons()));
  connect(_checksButton, SIGNAL(clicked()), this, SLOT(sHandleButtons()));

  _defaultCurr->setLabel(_defaultCurrLit);
  _routingNumber->setValidator(new QIntValidator(100000000, 999999999, this));
  _achAccountNumber->setValidator(new QRegExpValidator(QRegExp("^\\d{4,17}$"), this));

  _vendaddr->addColumn(tr("Number"), 70, Qt::AlignLeft, true, "vendaddr_code");
  _vendaddr->addColumn(tr("Name"),   50, Qt::AlignLeft, true, "vendaddr_name");
  _vendaddr->addColumn(tr("City"),   -1, Qt::AlignLeft, true, "vendaddr_city");
  _vendaddr->addColumn(tr("State"),  -1, Qt::AlignLeft, true, "vendaddr_state");
  _vendaddr->addColumn(tr("Country"),-1, Qt::AlignLeft, true, "vendaddr_country");
  _vendaddr->addColumn(tr("Postal Code"),-1, Qt::AlignLeft, true, "vendaddr_zipcode");

  _taxreg->addColumn(tr("Tax Authority"), 100, Qt::AlignLeft, true, "taxauth_code");
  _taxreg->addColumn(tr("Registration #"), -1, Qt::AlignLeft, true, "taxreg_number");

  _crmacctid = -1;
  _ignoreClose = false;
  _NumberGen = -1;
  
  if (_metrics->boolean("EnableBatchManager") &&
      ! _metrics->boolean("ACHEnabled"))
  {
    _poButton->hide();
    _checksButton->hide();
  }
  else if (! _metrics->boolean("EnableBatchManager") &&
           _metrics->boolean("ACHEnabled"))
  {
    _poButton->hide();
    _checksButton->hide();
    _transmitStack->setCurrentIndex(1);
  }
  else if (! _metrics->boolean("EnableBatchManager") &&
           ! _metrics->boolean("ACHEnabled"))
    ediTab->setVisible(false);
  // else defaults are OK

  if (_metrics->boolean("ACHEnabled") && omfgThis->_key.isEmpty())
    _checksButton->setEnabled(false);
}

vendor::~vendor()
{
  // no need to delete child widgets, Qt does it all for us
}

void vendor::languageChange()
{
  retranslateUi(this);
}

void vendor::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("crmacct_number", &valid);
  if (valid)
    _number->setText(param.toString());

  param = pParams.value("crmacct_name", &valid);
  if (valid)
    _name->setText(param.toString());

  param = pParams.value("crmacct_id", &valid);
  if (valid)
  {
    _crmacctid = param.toInt();
    _contact1->setSearchAcct(param.toInt());
    _contact2->setSearchAcct(param.toInt());
  }

  param = pParams.value("vend_id", &valid);
  if (valid)
  {
    _vendid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      q.exec("SELECT NEXTVAL('vend_vend_id_seq') AS vend_id;");
      if (q.first())
        _vendid = q.value("vend_id").toInt();
      else
        systemError(this, tr("A System Error occurred at %1::%2.")
                          .arg(__FILE__)
                          .arg(__LINE__) );

      if(((_metrics->value("CRMAccountNumberGeneration") == "A") ||
          (_metrics->value("CRMAccountNumberGeneration") == "O"))
       && _number->text().isEmpty() )
      {
        q.exec("SELECT fetchCRMAccountNumber() AS number;");
        if (q.first())
        {
          _number->setText(q.value("number"));
          _NumberGen = q.value("number").toInt();
        }
      }

      _comments->setId(_vendid);
      _defaultShipVia->setText(_metrics->value("DefaultPOShipVia"));
  
      connect(_number, SIGNAL(lostFocus()), this, SLOT(sCheck()));

      if (_privileges->check("MaintainVendorAddresses"))
      {
        connect(_vendaddr, SIGNAL(valid(bool)), _editAddress, SLOT(setEnabled(bool)));
        connect(_vendaddr, SIGNAL(valid(bool)), _deleteAddress, SLOT(setEnabled(bool)));
        connect(_vendaddr, SIGNAL(itemSelected(int)), _editAddress, SLOT(animateClick()));
      }
      else
      {
        _newAddress->setEnabled(FALSE);
        connect(_vendaddr, SIGNAL(itemSelected(int)), _viewAddress, SLOT(animateClick()));
      }

      _number->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      if (_privileges->check("MaintainVendorAddresses"))
      {
        connect(_vendaddr, SIGNAL(valid(bool)), _editAddress, SLOT(setEnabled(bool)));
        connect(_vendaddr, SIGNAL(valid(bool)), _deleteAddress, SLOT(setEnabled(bool)));
        connect(_vendaddr, SIGNAL(itemSelected(int)), _editAddress, SLOT(animateClick()));
      }
      else
      {
        _newAddress->setEnabled(FALSE);
        connect(_vendaddr, SIGNAL(itemSelected(int)), _viewAddress, SLOT(animateClick()));
      }

      _save->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _number->setEnabled(FALSE);
      _vendtype->setEnabled(FALSE);
      _active->setEnabled(FALSE);
      _name->setEnabled(FALSE);
      _accountNumber->setEnabled(FALSE);
      _defaultTerms->setEnabled(FALSE);
      _defaultShipVia->setEnabled(FALSE);
      _defaultCurr->setEnabled(FALSE);
      _contact1->setEnabled(FALSE);
      _contact2->setEnabled(FALSE);
      _address->setEnabled(FALSE);
      _notes->setReadOnly(FALSE);
      _poComments->setReadOnly(TRUE);
      _poItems->setEnabled(FALSE);
      _restrictToItemSource->setEnabled(FALSE);
      _receives1099->setEnabled(FALSE);
      _qualified->setEnabled(FALSE);
      _emailPODelivery->setEnabled(FALSE);
      _newAddress->setEnabled(FALSE);
      _defaultFOBGroup->setEnabled(false);
      _taxauth->setEnabled(false);
      _match->setEnabled(false);
      _newTaxreg->setEnabled(false);
      _comments->setReadOnly(TRUE);

      _achGroup->setEnabled(false);
      _routingNumber->setEnabled(false);
      _achAccountNumber->setEnabled(false);
      _individualId->setEnabled(false);
      _individualName->setEnabled(false);
      _accountType->setEnabled(false);

      _save->hide();
      _close->setText(tr("&Close"));

      disconnect(_taxreg, SIGNAL(valid(bool)), _deleteTaxreg, SLOT(setEnabled(bool)));
      disconnect(_taxreg, SIGNAL(valid(bool)), _editTaxreg, SLOT(setEnabled(bool)));
      disconnect(_taxreg, SIGNAL(itemSelected(int)), _editTaxreg, SLOT(animateClick()));
      connect(_taxreg, SIGNAL(itemSelected(int)), _viewTaxreg, SLOT(animateClick()));

      _close->setFocus();
    }
  }

  if(_metrics->value("CRMAccountNumberGeneration") == "A")
    _number->setEnabled(FALSE);

  if(cNew == _mode || !pParams.inList("showNextPrev"))
  {
    _next->hide();
    _previous->hide();
  }
}

// similar code in address, customer, shipto, vendor, vendorAddress, warehouse
int vendor::saveContact(ContactCluster* pContact)
{
  pContact->setAccount(_crmacctid);

  int answer = 2;	// Cancel
  int saveResult = pContact->save(AddressCluster::CHECK);

  if (-1 == saveResult)
    systemError(this, tr("There was an error saving a Contact (%1, %2).\n"
			 "Check the database server log for errors.")
		      .arg(pContact->label()).arg(saveResult),
		__FILE__, __LINE__);
  else if (-2 == saveResult)
    answer = QMessageBox::question(this,
		    tr("Question Saving Address"),
		    tr("There are multiple Contacts sharing this address (%1).\n"
		       "What would you like to do?")
		    .arg(pContact->label()),
		    tr("Change This One"),
		    tr("Change Address for All"),
		    tr("Cancel"),
		    2, 2);
  else if (-10 == saveResult)
    answer = QMessageBox::question(this,
		    tr("Question Saving %1").arg(pContact->label()),
		    tr("Would you like to update the existing Contact or create a new one?"),
		    tr("Create New"),
		    tr("Change Existing"),
		    tr("Cancel"),
		    2, 2);
  if (0 == answer)
    return pContact->save(AddressCluster::CHANGEONE);
  else if (1 == answer)
    return pContact->save(AddressCluster::CHANGEALL);

  return saveResult;
}

void vendor::sSave()
{
  struct {
    bool        condition;
    QString     msg;
    QWidget    *widget;
  } error[] = {
    { _number->text().trimmed().length() == 0,
      tr("Please enter a Number for this new Vendor."),
      _number },
    { _name->text().trimmed().length() == 0,
      tr("Please enter a Name for this new Vendor."),
      _name },
    { _defaultTerms->id() == -1,
      tr("You must select a Terms code for this Vendor before continuing."),
      _defaultTerms },
    { _vendtype->id() == -1,
      tr("You must select a Vendor Type for this Vendor before continuing."),
      _vendtype },
    { _achGroup->isChecked() &&
      _routingNumber->text().stripWhiteSpace().length() == 0 &&
      !omfgThis->_key.isEmpty(),
      tr("Please enter a Routing Number if ACH Check Printing is enabled."),
      _routingNumber },
    { _achGroup->isChecked() &&
      _routingNumber->text().stripWhiteSpace().length() < 9  &&
      !omfgThis->_key.isEmpty(),
      tr("Routing Numbers for ACH Check Printing must be 9 digits long."),
      _routingNumber },
    { _achGroup->isChecked() &&
      _achAccountNumber->text().stripWhiteSpace().length() == 0 &&
      !omfgThis->_key.isEmpty(),
      tr("Please enter an Account Number if ACH Check Printing is enabled."),
      _achAccountNumber },
    { _achGroup->isChecked() && _useACHSpecial->isChecked() &&
      _individualName->text().stripWhiteSpace().length() == 0 &&
      !omfgThis->_key.isEmpty(),
      tr("Please enter an Individual Name if ACH Check Printing is enabled and "
         "'%1' is checked.").arg(_useACHSpecial->title()),
      _individualName }
  };

  for (unsigned int i = 0; i < sizeof(error) / sizeof(error[0]); i++)
    if (error[i].condition)
    {
      QMessageBox::critical(this, tr("Cannot Save Vendor"),
                            error[i].msg);
      error[i].widget->setFocus();
      return;
    }

  if (_number->text().trimmed().toUpper() != _cachedNumber.upper())
  {
    q.prepare( "SELECT vend_name "
	       "FROM vendinfo "
	       "WHERE (UPPER(vend_number)=UPPER(:vend_number)) "
	       "  AND (vend_id<>:vend_id);" );
    q.bindValue(":vend_number", _number->text().trimmed());
    q.bindValue(":vend_id", _vendid);
    q.exec();
    if (q.first())
    {
      QMessageBox::critical(this, tr("Vendor Number Used"),
			    tr("<p>The newly entered Vendor Number cannot be "
                               "used as it is already used by the Vendor '%1'. "
                               "Please correct or enter a new Vendor Number." )
			     .arg(q.value("vend_name").toString()) );
      _number->setFocus();
      return;
    }
  }

  XSqlQuery rollback;
  rollback.prepare("ROLLBACK;");

  if (! q.exec("BEGIN"))
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  int saveResult = _address->save(AddressCluster::CHECK);
  if (-2 == saveResult)
  {
    int answer = QMessageBox::question(this,
		    tr("Question Saving Address"),
		    tr("<p>There are multiple uses of this Vendor's "
		       "Address. What would you like to do?"),
		    tr("Change This One"),
		    tr("Change Address for All"),
		    tr("Cancel"),
		    2, 2);
    if (0 == answer)
      saveResult = _address->save(AddressCluster::CHANGEONE);
    else if (1 == answer)
      saveResult = _address->save(AddressCluster::CHANGEALL);
  }
  if (saveResult < 0)	// not else-if: this is error check for CHANGE{ONE,ALL}
  {
    systemError(this, tr("There was an error saving this address (%1).\n"
			 "Check the database server log for errors.")
		      .arg(saveResult), __FILE__, __LINE__);
    rollback.exec();
    _address->setFocus();
    return;
  }

  if (saveContact(_contact1) < 0)
  {
    rollback.exec();
    _contact1->setFocus();
    return;
  }

  if (saveContact(_contact2) < 0)
  {
    rollback.exec();
    _contact2->setFocus();
    return;
  }

  QString sql;
  if (_mode == cEdit)
  {
    sql = "UPDATE vendinfo "
          "SET vend_number=<? value(\"vend_number\") ?>,"
          "    vend_accntnum=<? value(\"vend_accntnum\") ?>,"
          "    vend_active=<? value(\"vend_active\") ?>,"
          "    vend_vendtype_id=<? value(\"vend_vendtype_id\") ?>,"
          "    vend_name=<? value(\"vend_name\") ?>,"
          "    vend_cntct1_id=<? value(\"vend_cntct1_id\") ?>,"
          "    vend_cntct2_id=<? value(\"vend_cntct2_id\") ?>,"
	  "    vend_addr_id=<? value(\"vend_addr_id\") ?>,"
          "    vend_po=<? value(\"vend_po\") ?>,"
          "    vend_restrictpurch=<? value(\"vend_restrictpurch\") ?>,"
          "    vend_1099=<? value(\"vend_1099\") ?>,"
          "    vend_qualified=<? value(\"vend_qualified\") ?>,"
          "    vend_comments=<? value(\"vend_comments\") ?>,"
          "    vend_pocomments=<? value(\"vend_pocomments\") ?>,"
          "    vend_fobsource=<? value(\"vend_fobsource\") ?>,"
          "    vend_fob=<? value(\"vend_fob\") ?>,"
          "    vend_terms_id=<? value(\"vend_terms_id\") ?>,"
          "    vend_shipvia=<? value(\"vend_shipvia\") ?>,"
	  "    vend_curr_id=<? value(\"vend_curr_id\") ?>, "
          "    vend_emailpodelivery=<? value(\"vend_emailpodelivery\") ?>,"
          "    vend_ediemail=<? value(\"vend_ediemail\") ?>,"
          "    vend_ediemailbody=<? value(\"vend_ediemailbody\") ?>,"
          "    vend_edisubject=<? value(\"vend_edisubject\") ?>,"
          "    vend_edifilename=<? value(\"vend_edifilename\") ?>,"
          "    vend_edicc=<? value(\"vend_edicc\") ?>,"
          "    vend_taxauth_id=<? value(\"vend_taxauth_id\") ?>,"
          "    vend_match=<? value(\"vend_match\") ?>,"
          "    vend_ach_enabled=<? value(\"vend_ach_enabled\") ?>,"
          "    vend_ediemailhtml=<? value(\"vend_ediemailhtml\") ?>, "
          "<? if exists(\"key\") ?>"
          "    vend_ach_routingnumber=encrypt(setbytea(<? value(\"vend_ach_routingnumber\") ?>),"
          "                             setbytea(<? value(\"key\") ?>), 'bf'),"
          "    vend_ach_accntnumber=encrypt(setbytea(<? value(\"vend_ach_accntnumber\") ?>),"
          "                           setbytea(<? value(\"key\") ?>), 'bf'),"
          "<? endif ?>"
          "    vend_ach_use_vendinfo=<? value(\"vend_ach_use_vendinfo\") ?>,"
          "    vend_ach_accnttype=<? value(\"vend_ach_accnttype\") ?>,"
          "    vend_ach_indiv_number=<? value(\"vend_ach_indiv_number\") ?>,"
          "    vend_ach_indiv_name=<? value(\"vend_ach_indiv_name\") ?> "
          "WHERE (vend_id=<? value(\"vend_id\") ?>);" ;
  }
  else if (_mode == cNew)
    sql = "INSERT INTO vendinfo "
          "( vend_id, vend_number, vend_accntnum,"
          "  vend_active, vend_vendtype_id, vend_name,"
          "  vend_cntct1_id, vend_cntct2_id, vend_addr_id,"
          "  vend_po, vend_restrictpurch,"
          "  vend_1099, vend_qualified,"
          "  vend_comments, vend_pocomments,"
          "  vend_fobsource, vend_fob,"
          "  vend_terms_id, vend_shipvia, vend_curr_id,"
          "  vend_emailpodelivery, vend_ediemail, vend_ediemailbody,"
          "  vend_edisubject, vend_edifilename, vend_edicc,"
          "  vend_taxauth_id, vend_match, vend_ach_enabled,"
          "  vend_ach_routingnumber, vend_ach_accntnumber,"
          "  vend_ach_use_vendinfo,"
          "  vend_ach_accnttype, vend_ach_indiv_number,"
          "  vend_ach_indiv_name, vend_ediemailhtml ) "
          "VALUES "
          "( <? value(\"vend_id\") ?>,"
          "  <? value(\"vend_number\") ?>,"
          "  <? value(\"vend_accntnum\") ?>,"
          "  <? value(\"vend_active\") ?>,"
          "  <? value(\"vend_vendtype_id\") ?>,"
          "  <? value(\"vend_name\") ?>,"
          "  <? value(\"vend_cntct1_id\") ?>,"
          "  <? value(\"vend_cntct2_id\") ?>,"
          "  <? value(\"vend_addr_id\") ?>,"
          "  <? value(\"vend_po\") ?>,"
          "  <? value(\"vend_restrictpurch\") ?>,"
          "  <? value(\"vend_1099\") ?>,"
          "  <? value(\"vend_qualified\") ?>,"
          "  <? value(\"vend_comments\") ?>,"
          "  <? value(\"vend_pocomments\") ?>,"
          "  <? value(\"vend_fobsource\") ?>,"
          "  <? value(\"vend_fob\") ?>,"
          "  <? value(\"vend_terms_id\") ?>,"
          "  <? value(\"vend_shipvia\") ?>,"
          "  <? value(\"vend_curr_id\") ?>, "
          "  <? value(\"vend_emailpodelivery\") ?>,"
          "  <? value(\"vend_ediemail\") ?>,"
          "  <? value(\"vend_ediemailbody\") ?>,"
          "  <? value(\"vend_edisubject\") ?>,"
          "  <? value(\"vend_edifilename\") ?>,"
          "  <? value(\"vend_edicc\") ?>,"
          "  <? value(\"vend_taxauth_id\") ?>,"
          "  <? value(\"vend_match\") ?>,"
          "  <? value(\"vend_ach_enabled\") ?>,"
          "<? if exists(\"key\") ?>"
          "  encrypt(setbytea(<? value(\"vend_ach_routingnumber\") ?>),"
          "          setbytea(<? value(\"key\") ?>), 'bf'),"
          "  encrypt(setbytea(<? value(\"vend_ach_accntnumber\") ?>),"
          "          setbytea(<? value(\"key\") ?>), 'bf'),"
          "<? else ?>"
          "  '',"
          "  '',"
          "<? endif ?>"
          "  <? value(\"vend_ach_use_vendinfo\") ?>,"
          "  <? value(\"vend_ach_accnttype\") ?>,"
          "  <? value(\"vend_ach_indiv_number\") ?>,"
          "  <? value(\"vend_ach_indiv_name\") ?>,"
          "  <? value(\"vend_ediemailhtml\") ?>"
          "   );"  ;
 
  ParameterList params;
  params.append("vend_id", _vendid);
  params.append("vend_vendtype_id", _vendtype->id());
  params.append("vend_terms_id", _defaultTerms->id());
  params.append("vend_curr_id", _defaultCurr->id());

  params.append("vend_number",   _number->text().trimmed().upper());
  params.append("vend_accntnum", _accountNumber->text().trimmed());
  params.append("vend_name",     _name->text().trimmed());

  if (_contact1->id() > 0)
    params.append("vend_cntct1_id", _contact1->id());		// else NULL
  if (_contact2->id() > 0)
    params.append("vend_cntct2_id", _contact2->id());		// else NULL
  if (_address->id() > 0)
    params.append("vend_addr_id", _address->id());		// else NULL

  params.append("vend_comments",   _notes->toPlainText());
  params.append("vend_pocomments", _poComments->toPlainText());
  params.append("vend_shipvia",    _defaultShipVia->text());

  params.append("vend_active",        QVariant(_active->isChecked()));
  params.append("vend_po",            QVariant(_poItems->isChecked()));
  params.append("vend_restrictpurch", QVariant(_restrictToItemSource->isChecked()));
  params.append("vend_1099",          QVariant(_receives1099->isChecked()));
  params.append("vend_qualified",     QVariant(_qualified->isChecked()));
  params.append("vend_match",         QVariant(_match->isChecked()));

  params.append("vend_emailpodelivery", QVariant(_emailPODelivery->isChecked()));
  params.append("vend_ediemail",     _ediEmail->text());
  params.append("vend_ediemailbody", _ediEmailBody->toPlainText());
  params.append("vend_edisubject",   _ediSubject->text());
  params.append("vend_edifilename",  _ediFilename->text());
  params.append("vend_edicc",        _ediCC->text().trimmed());
  params.append("vend_ediemailhtml", QVariant(_ediEmailHTML->isChecked()));

  if (!omfgThis->_key.isEmpty())
    params.append("key",                   omfgThis->_key);
  params.append("vend_ach_enabled",      QVariant(_achGroup->isChecked()));
  params.append("vend_ach_routingnumber",_routingNumber->text().trimmed());
  params.append("vend_ach_accntnumber",  _achAccountNumber->text().trimmed());
  params.append("vend_ach_use_vendinfo", QVariant(! _useACHSpecial->isChecked()));
  params.append("vend_ach_indiv_number", _individualId->text().trimmed());
  params.append("vend_ach_indiv_name",   _individualName->text().trimmed());

  if (_accountType->currentItem() == 0)
    params.append("vend_ach_accnttype",  "K");
  else if (_accountType->currentItem() == 1)
    params.append("vend_ach_accnttype",  "C");

  if(_taxauth->isValid())
    params.append("vend_taxauth_id", _taxauth->id());

  if (_useWarehouseFOB->isChecked())
  {
    params.append("vend_fobsource", "W");
    params.append("vend_fob", "");
  }
  else if (_useVendorFOB)
  {
    params.append("vend_fobsource", "V");
    params.append("vend_fob", _vendorFOB->text().trimmed());
  }

  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  if (q.lastError().type() != QSqlError::NoError)
  {
    rollback.exec();
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

//  Get the crmacct that was created by the vendinfo trigger
  q.prepare("SELECT crmacct_id "
	    "FROM crmacct "
	    "WHERE (crmacct_vend_id=:vend_id);");
  q.bindValue(":vend_id", _vendid);
  q.exec();
  if (q.first())
  {
    _crmacctid = q.value("crmacct_id").toInt();
    _contact1->setAccount(_crmacctid);
    _contact2->setAccount(_crmacctid);
  }
  else if (!q.lastError().type() == QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }


// need to save contacts again with updated CRM Account
  if (saveContact(_contact1) < 0)
  {
    rollback.exec();
    _contact1->setFocus();
    return;
  }

  if (saveContact(_contact2) < 0)
  {
    rollback.exec();
    _contact2->setFocus();
    return;
  }

  q.exec("COMMIT;");
  _NumberGen = -1;
  omfgThis->sVendorsUpdated();

  if(!_ignoreClose)
    close();
}

void vendor::sCheck()
{
//  Make sure that the newly entered vend_number is not already in use.
//  Switch to cEdit and populate if so.
  if (_number->text().length())
  {
    _number->setText(_number->text().trimmed().toUpper());
    if(cNew == _mode && -1 != _NumberGen && _number->text().toInt() != _NumberGen)
    {
      XSqlQuery query;
      query.prepare( "SELECT releaseCRMAccountNumber(:Number);" );
      query.bindValue(":Number", _NumberGen);
      query.exec();
      _NumberGen = -1;
    }

    q.prepare( "SELECT vend_id "
               "FROM vendinfo "
               "WHERE (UPPER(vend_number)=UPPER(:vend_number));" );
    q.bindValue(":vend_number", _number->text());
    q.exec();
    if (q.first())
    {
      _vendid = q.value("vend_id").toInt();
      _mode = cEdit;
      populate();

      _number->setEnabled(FALSE);
    }
  }
}

void vendor::populate()
{
  MetaSQLQuery mql(
            "SELECT *, crmacct_id, "
            "<? if exists(\"key\") ?>"
            "       CASE WHEN LENGTH(vend_ach_routingnumber) > 0 THEN"
            "       formatbytea(decrypt(setbytea(vend_ach_routingnumber),"
            "                           setbytea(<? value(\"key\") ?>), 'bf'))"
            "            ELSE '' END AS routingnum,"
            "       CASE WHEN LENGTH(vend_ach_accntnumber) > 0 THEN"
            "       formatbytea(decrypt(setbytea(vend_ach_accntnumber),"
            "                           setbytea(<? value(\"key\") ?>), 'bf'))"
            "            ELSE '' END AS accntnum "
            "<? else ?>"
            "       <? value(\"na\") ?> AS routingnum,"
            "       <? value(\"na\") ?> AS accntnum "
            "<? endif ?>"
            "FROM vendinfo "
            "  JOIN crmacct ON (vend_id=crmacct_vend_id) "
            "WHERE (vend_id=<? value(\"vend_id\") ?>);" );
  ParameterList params;
  params.append("vend_id", _vendid);
  params.append("key",     omfgThis->_key);
  params.append("na",      tr("N/A"));
  q = mql.toQuery(params);
  if (q.first())
  {
    _cachedNumber = q.value("vend_number").toString();

    _number->setText(q.value("vend_number"));
    _accountNumber->setText(q.value("vend_accntnum"));
    _vendtype->setId(q.value("vend_vendtype_id").toInt());
    _active->setChecked(q.value("vend_active").toBool());
    _name->setText(q.value("vend_name"));
    _contact1->setId(q.value("vend_cntct1_id").toInt());
    _contact1->setSearchAcct(q.value("crmacct_id").toInt());
    _contact2->setId(q.value("vend_cntct2_id").toInt());
    _contact2->setSearchAcct(q.value("crmacct_id").toInt());
    _address->setId(q.value("vend_addr_id").toInt());
    _defaultTerms->setId(q.value("vend_terms_id").toInt());
    _defaultShipVia->setText(q.value("vend_shipvia").toString());
    _defaultCurr->setId(q.value("vend_curr_id").toInt());
    _poItems->setChecked(q.value("vend_po").toBool());
    _restrictToItemSource->setChecked(q.value("vend_restrictpurch").toBool());
    _receives1099->setChecked(q.value("vend_1099").toBool());
    _match->setChecked(q.value("vend_match").toBool());
    _qualified->setChecked(q.value("vend_qualified").toBool());
    _notes->setText(q.value("vend_comments").toString());
    _poComments->setText(q.value("vend_pocomments").toString());
    _emailPODelivery->setChecked(q.value("vend_emailpodelivery").toBool());
    _ediEmail->setText(q.value("vend_ediemail"));
    _ediSubject->setText(q.value("vend_edisubject"));
    _ediFilename->setText(q.value("vend_edifilename"));
    _ediEmailBody->setText(q.value("vend_ediemailbody").toString());
    _ediCC->setText(q.value("vend_edicc").toString());
    _ediEmailHTML->setChecked(q.value("vend_ediemailhtml").toBool());
    
    _taxauth->setId(q.value("vend_taxauth_id").toInt());

    if (q.value("vend_fobsource").toString() == "V")
    {
      _useVendorFOB->setChecked(TRUE);
      _vendorFOB->setText(q.value("vend_fob"));
    }
    else
      _useWarehouseFOB->setChecked(TRUE);

    _achGroup->setChecked(q.value("vend_ach_enabled").toBool());
    _routingNumber->setText(q.value("routingnum").toString());
    _achAccountNumber->setText(q.value("accntnum").toString());
    _useACHSpecial->setChecked(! q.value("vend_ach_use_vendinfo").toBool());
    _individualId->setText(q.value("vend_ach_indiv_number").toString());
    _individualName->setText(q.value("vend_ach_indiv_name").toString());

    if (q.value("vend_ach_accnttype").toString() == "K")
      _accountType->setCurrentItem(0);
    else if (q.value("vend_ach_accnttype").toString() == "C")
      _accountType->setCurrentItem(1);

    sFillAddressList();
    sFillTaxregList();
    _comments->setId(_vendid);
  }
  else if (q.lastError().type() == QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  q.prepare("SELECT crmacct_id "
	    "FROM crmacct "
	    "WHERE (crmacct_vend_id=:vend_id);");
  q.bindValue(":vend_id", _vendid);
  q.exec();
  if (q.first())
    _crmacctid = q.value("crmacct_id").toInt();
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void vendor::sPrintAddresses()
{
  ParameterList params;
  params.append("vend_id", _vendid);

  orReport report("VendorAddressList", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void vendor::sNewAddress()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("vend_id", _vendid);

  vendorAddress newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillAddressList();
}

void vendor::sEditAddress()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("vendaddr_id", _vendaddr->id());

  vendorAddress newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillAddressList();
}

void vendor::sViewAddress()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("vendaddr_id", _vendaddr->id());

  vendorAddress newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void vendor::sDeleteAddress()
{
  q.prepare( "DELETE FROM vendaddrinfo "
             "WHERE (vendaddr_id=:vendaddr_id);" );
  q.bindValue(":vendaddr_id", _vendaddr->id());
  q.exec();
  sFillAddressList();
}

void vendor::sFillAddressList()
{
  q.prepare( "SELECT vendaddr_id, vendaddr_code, vendaddr_name,"
             "       vendaddr_city, vendaddr_state, vendaddr_country,"
             "       vendaddr_zipcode "
             "FROM vendaddr "
             "WHERE (vendaddr_vend_id=:vend_id) "
             "ORDER BY vendaddr_code;" );
  q.bindValue(":vend_id", _vendid);
  q.exec();
  _vendaddr->populate(q);
}

void vendor::sFillTaxregList()
{
  XSqlQuery taxreg;
  taxreg.prepare("SELECT taxreg_id, taxreg_taxauth_id, "
                 "       taxauth_code, taxreg_number "
                 "FROM taxreg, taxauth "
                 "WHERE ((taxreg_rel_type='V') "
                 "  AND  (taxreg_rel_id=:vend_id) "
                 "  AND  (taxreg_taxauth_id=taxauth_id));");
  taxreg.bindValue(":vend_id", _vendid);
  taxreg.exec();
  _taxreg->clear();
  _taxreg->populate(taxreg, true);
  if (taxreg.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taxreg.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void vendor::sNewTaxreg()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("taxreg_rel_id", _vendid);
  params.append("taxreg_rel_type", "V");

  taxRegistration newdlg(this, "", TRUE);
  if (newdlg.set(params) == NoError && newdlg.exec() != XDialog::Rejected)
    sFillTaxregList();
}

void vendor::sEditTaxreg()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("taxreg_id", _taxreg->id());

  taxRegistration newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.set(params) == NoError && newdlg.exec() != XDialog::Rejected)
    sFillTaxregList();
}

void vendor::sViewTaxreg()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("taxreg_id", _taxreg->id());

  taxRegistration newdlg(this, "", TRUE);
  if (newdlg.set(params) == NoError)
    newdlg.exec();
}

void vendor::sDeleteTaxreg()
{
  q.prepare("DELETE FROM taxreg "
            "WHERE (taxreg_id=:taxreg_id);");
  q.bindValue(":taxreg_id", _taxreg->id());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  sFillTaxregList();
}

void vendor::sNext()
{
  // Find Next 
  q.prepare("SELECT vend_id "
            "  FROM vendinfo"
            " WHERE (:number < vend_number)"
            " ORDER BY vend_number"
            " LIMIT 1;");
  q.bindValue(":number", _number->text());
  q.exec();
  if(!q.first())
  {
    QMessageBox::information(this, tr("At Last Record"),
       tr("You are already on the last record.") );
    return;
  }
  int newid = q.value("vend_id").toInt();

  if(!sCheckSave())
    return;

  clear();

  _vendid = newid;
  populate();
}

void vendor::sPrevious()
{
  // Find Next 
  q.prepare("SELECT vend_id "
            "  FROM vendinfo"
            " WHERE (:number > vend_number)"
            " ORDER BY vend_number DESC"
            " LIMIT 1;");
  q.bindValue(":number", _number->text());
  q.exec();
  if(!q.first())
  {
    QMessageBox::information(this, tr("At First Record"),
       tr("You are already on the first record.") );
    return;
  }
  int newid = q.value("vend_id").toInt();

  if(!sCheckSave())
    return;

  clear();

  _vendid = newid;
  populate();
}

bool vendor::sCheckSave()
{
  if(cEdit == _mode)
  {
    switch(QMessageBox::question(this, tr("Save Changes?"),
         tr("Would you like to save any changes before continuing?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
    {
      case QMessageBox::Yes:
        _ignoreClose = true;
        sSave();
        _ignoreClose = false;
        break;
      case QMessageBox::No:
        break;
      case QMessageBox::Cancel:
      default:
        return false;
    };
  }
  return true;
}

void vendor::clear()
{
  _cachedNumber = QString::null;
  _crmacctid = -1;
  _vendid = -1;

  _active->setChecked(true);
  _poItems->setChecked(false);
  _restrictToItemSource->setChecked(false);
  _qualified->setChecked(false);
  _match->setChecked(false);
  _receives1099->setChecked(false);
  _emailPODelivery->setChecked(false);

  _vendtype->setId(-1);
  _defaultTerms->setId(-1);
  _defaultCurr->setCurrentIndex(0);
  _taxauth->setId(-1);

  _useWarehouseFOB->setChecked(true);

  _number->clear();
  _name->clear();
  _accountNumber->clear();
  _defaultShipVia->clear();
  _vendorFOB->clear(); 
  _notes->clear();
  _poComments->clear(); 
  _ediEmail->clear();
  _ediCC->clear();
  _ediSubject->clear();
  _ediFilename->clear();
  _ediEmailBody->clear();

  _address->clear();
  _contact1->clear();
  _contact2->clear();

  _vendaddr->clear();
  _taxreg->clear();

  _achGroup->setChecked(false);
  _routingNumber->clear();
  _achAccountNumber->clear();
  _individualId->clear();
  _individualName->clear();
  _accountType->setCurrentItem(0);

  _comments->setId(-1);
  _tabs->setCurrentIndex(0);
}

void vendor::closeEvent(QCloseEvent *pEvent)
{
  if(cNew == _mode && -1 != _NumberGen)
  {
    XSqlQuery query;
    query.prepare( "SELECT releaseCRMAccountNumber(:Number);" );
    query.bindValue(":Number", _NumberGen);
    query.exec();
    _NumberGen = -1;
  }
  XWidget::closeEvent(pEvent);
}

void vendor::sHandleButtons()
{
  if (_mainButton->isChecked())
    _addressStack->setCurrentIndex(0);
  else
    _addressStack->setCurrentIndex(1);
    
  if (_poButton->isChecked())
    _transmitStack->setCurrentIndex(0);
  else
    _transmitStack->setCurrentIndex(1);
}
