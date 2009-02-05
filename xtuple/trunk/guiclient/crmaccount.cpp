/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "crmaccount.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>
#include <QCloseEvent>

#include <metasql.h>

#include "characteristicAssignment.h"
#include "contact.h"
#include "customer.h"
#include "incident.h"
#include "prospect.h"
#include "taxAuthority.h"
#include "todoItem.h"
#include "vendor.h"
#include "storedProcErrorLookup.h"
#include "dspCustomerInformation.h"
#include "vendorWorkBench.h"
#include "opportunity.h"
#include "mqlutil.h"
#include "lotSerialRegistration.h"

#define DEBUG false

crmaccount::crmaccount(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  if(!_privileges->check("EditOwner")) _owner->setEnabled(false);

  _owner->setUsername(omfgThis->username());
  
  int menuItem;
  QMenu * todoMenu = new QMenu;
  menuItem = todoMenu->insertItem(tr("Incident"), this, SLOT(sNewIncdt()));
  if (!_privileges->check("MaintainIncidents"))
    todoMenu->setItemEnabled(menuItem, FALSE);
  menuItem = todoMenu->insertItem(tr("To-Do Item"),   this, SLOT(sNewTodo()));
  if (!_privileges->check("MaintainPersonalTodoList") &&
      !_privileges->check("MaintainOtherTodoLists"))
    todoMenu->setItemEnabled(menuItem, FALSE);
  _newTodo->setMenu(todoMenu);

  if (_privileges->check("MaintainOpportunities"))
    _newOpportunity->setEnabled(TRUE);
    
  connect(_activeTodoIncdt, SIGNAL(toggled(bool)), this, SLOT(sPopulateTodo()));
  connect(_attach,		SIGNAL(clicked()), this, SLOT(sAttach()));
  connect(_autoUpdateTodo,  SIGNAL(toggled(bool)), this, SLOT(sHandleAutoUpdate()));
  connect(_close,		SIGNAL(clicked()), this, SLOT(sClose()));
  connect(_competitor,		SIGNAL(clicked()), this, SLOT(sCompetitor()));
  connect(_completedTodoIncdt, SIGNAL(toggled(bool)), this, SLOT(sPopulateTodo()));
  connect(_contacts, SIGNAL(populateMenu(QMenu*, QTreeWidgetItem*)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_deleteCharacteristic,SIGNAL(clicked()), this, SLOT(sDeleteCharacteristic()));
  connect(_deleteOpportunity,		SIGNAL(clicked()), this, SLOT(sOplistDelete()));
  connect(_deleteReg,		SIGNAL(clicked()), this, SLOT(sDeleteReg()));
  connect(_deleteTodoIncdt,	SIGNAL(clicked()), this, SLOT(sDeleteTodoIncdt()));
  connect(_detach,		SIGNAL(clicked()), this, SLOT(sDetach()));
  connect(_edit,		SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_editCharacteristic,	SIGNAL(clicked()), this, SLOT(sEditCharacteristic()));
  connect(_editOpportunity,		SIGNAL(clicked()), this, SLOT(sOplistEdit()));
  connect(_editReg,		SIGNAL(clicked()), this, SLOT(sEditReg()));
  connect(_editTodoIncdt,	SIGNAL(clicked()), this, SLOT(sEditTodoIncdt()));
  connect(_new,			SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_newCharacteristic,	SIGNAL(clicked()), this, SLOT(sNewCharacteristic()));
  connect(_newOpportunity,		SIGNAL(clicked()), this, SLOT(sOplistNew()));
  connect(_newReg,		SIGNAL(clicked()), this, SLOT(sNewReg()));
  connect(_partner,		SIGNAL(clicked()), this, SLOT(sPartner()));
  connect(_prospectButton,	SIGNAL(clicked()), this, SLOT(sProspect()));
  connect(_save,		SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_showTodo,	    SIGNAL(toggled(bool)), this, SLOT(sPopulateTodo()));
  connect(_showIncdt,	    SIGNAL(toggled(bool)), this, SLOT(sPopulateTodo()));
  connect(_taxauthButton,	SIGNAL(clicked()), this, SLOT(sTaxAuth()));
  connect(_todo, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(sHandleTodoPrivs()));
  connect(_todo, SIGNAL(populateMenu(QMenu*, QTreeWidgetItem*)), this, SLOT(sPopulateTodoMenu(QMenu*)));
  connect(_viewOpportunity,	SIGNAL(clicked()), this, SLOT(sOplistView()));
  connect(_viewTodoIncdt,	SIGNAL(clicked()), this, SLOT(sViewTodoIncdt()));
  connect(omfgThis, SIGNAL(customersUpdated(int, bool)), this, SLOT(sUpdateRelationships()));
  connect(omfgThis, SIGNAL(prospectsUpdated()),  this, SLOT(sUpdateRelationships()));
  connect(omfgThis, SIGNAL(taxAuthsUpdated(int)),this, SLOT(sUpdateRelationships()));
  connect(omfgThis, SIGNAL(vendorsUpdated()),    this, SLOT(sUpdateRelationships()));
  connect(_customer, SIGNAL(toggled(bool)), this, SLOT(sCustomerToggled()));
  connect(_prospect, SIGNAL(toggled(bool)), this, SLOT(sProspectToggled()));
  connect(_oplist, SIGNAL(populateMenu(QMenu*, QTreeWidgetItem*, int)), this, SLOT(sPopulateOplistMenu(QMenu*)));
  connect(_number, SIGNAL(lostFocus()), this, SLOT(sCheckNumber()));
  connect(_primaryButton, SIGNAL(toggled(bool)), this, SLOT(sHandleButtons())); 	 
  connect(_secondaryButton, SIGNAL(toggled(bool)), this, SLOT(sHandleButtons())); 	 
	connect(_allButton, SIGNAL(toggled(bool)), this, SLOT(sHandleButtons()));
  connect(_customerButton, SIGNAL(clicked()), this, SLOT(sCustomer()));
  connect(_vendorButton, SIGNAL(clicked()), this, SLOT(sVendor()));

  _contacts->addColumn(tr("First Name"),   50, Qt::AlignLeft, true, "cntct_first_name");
  _contacts->addColumn(tr("Last Name"),	   -1, Qt::AlignLeft, true, "cntct_last_name");
  _contacts->addColumn(tr("Phone"),	  100, Qt::AlignLeft, true, "cntct_phone");
  _contacts->addColumn(tr("Alternate"),	  100, Qt::AlignLeft, true, "cntct_phone2");
  _contacts->addColumn(tr("Fax"),	  100, Qt::AlignLeft, true, "cntct_fax");
  _contacts->addColumn(tr("E-Mail"),	  100, Qt::AlignLeft, true, "cntct_email");
  _contacts->addColumn(tr("Web Address"), 100, Qt::AlignLeft, true, "cntct_webaddr");

  _charass->addColumn(tr("Characteristic"), _itemColumn, Qt::AlignLeft, true, "char_name");
  _charass->addColumn(tr("Value"),          -1,          Qt::AlignLeft, true, "charass_value");

  _todo->addColumn(tr("Type"),    _statusColumn, Qt::AlignCenter, true, "type");	
  _todo->addColumn(tr("Priority"),  _userColumn, Qt::AlignRight,  true, "priority");
  _todo->addColumn(tr("User"),      _userColumn, Qt::AlignLeft,   true, "usr");
  _todo->addColumn(tr("Name"),		    100, Qt::AlignLeft,   true, "name");
  _todo->addColumn(tr("Description"),        -1, Qt::AlignLeft,   true, "descrip");
  _todo->addColumn(tr("Status"),  _statusColumn, Qt::AlignLeft,   true, "status");
  _todo->addColumn(tr("Due Date"),  _dateColumn, Qt::AlignLeft,   true, "due");
  _todo->addColumn(tr("Incident"), _orderColumn, Qt::AlignLeft,   true, "incdt");

  _oplist->addColumn(tr("Name"),         _itemColumn, Qt::AlignLeft,  true, "ophead_name");
  _oplist->addColumn(tr("CRM Acct."),    _userColumn, Qt::AlignLeft,  true, "crmacct_number");
  _oplist->addColumn(tr("Owner"),        _userColumn, Qt::AlignLeft,  true, "ophead_owner_username");
  _oplist->addColumn(tr("Stage"),       _orderColumn, Qt::AlignLeft,  true, "opstage_name");
  _oplist->addColumn(tr("Source"),      _orderColumn, Qt::AlignLeft,  false,"opsource_name");
  _oplist->addColumn(tr("Type"),        _orderColumn, Qt::AlignLeft,  false, "optype_name");
  _oplist->addColumn(tr("Prob.%"),      _prcntColumn, Qt::AlignRight, false,"ophead_probability_prcnt");
  _oplist->addColumn(tr("Amount"),      _moneyColumn, Qt::AlignRight, false,"ophead_amount");
  _oplist->addColumn(tr("Currency"), _currencyColumn, Qt::AlignLeft,  false,"ophead_currabbr");
  _oplist->addColumn(tr("Target Date"),  _dateColumn, Qt::AlignLeft,  true, "ophead_target_date");
  _oplist->addColumn(tr("Actual Date"),  _dateColumn, Qt::AlignLeft,   false,"ophead_actual_date");
  
  _reg->addColumn(tr("Lot/Serial")  ,  _itemColumn,  Qt::AlignLeft, true, "ls_number" );
  _reg->addColumn(tr("Item")        ,  _itemColumn,  Qt::AlignLeft, true, "item_number" );
  _reg->addColumn(tr("Description") ,  -1,  	     Qt::AlignLeft, true, "item_descrip1" );
  _reg->addColumn(tr("Qty"       )  ,  _qtyColumn,   Qt::AlignLeft, true, "lsreg_qty" );
  _reg->addColumn(tr("Sold"        ),  _dateColumn,  Qt::AlignLeft, true, "lsreg_solddate" );
  _reg->addColumn(tr("Registered"  ),  _dateColumn,  Qt::AlignLeft, true, "lsreg_regdate" );
  _reg->addColumn(tr("Expires"   )  ,  _dateColumn,  Qt::AlignLeft, true, "lsreg_expiredate" );

  if (_preferences->boolean("XCheckBox/forgetful"))
  {
    _active->setChecked(true);
    _activeTodoIncdt->setChecked(true);
    _showIncdt->setChecked(true);
    _showTodo->setChecked(true);
  }

  _NumberGen    = -1;
  _mode		= cNew;
  _crmacctId    = -1;
  _competitorId	= -1;
  _custId       = -1;
  _partnerId    = -1;
  _prospectId   = -1;
  _taxauthId    = -1;
  _vendId       = -1;
  _comments->setId(-1);
  _documents->setId(-1);
  _modal        = false;
  
  if (!_metrics->boolean("LotSerialControl"))
    _tab->removeTab(_tab->indexOf(_registrationsTab));

  sHandleTodoPrivs();
  
  _primary->setMinimalLayout(FALSE);
  _primary->setAccountVisible(FALSE);
  _primary->setActiveVisible(FALSE);
  _secondary->setMinimalLayout(FALSE);
  _secondary->setAccountVisible(FALSE);
  _secondary->setActiveVisible(FALSE);
  
  sHandleAutoUpdate();
  
  _primary->setInitialsVisible(false);
  _secondary->setInitialsVisible(false);
  _primary->setListVisible(true);
  _secondary->setListVisible(true);
}

crmaccount::~crmaccount()
{
    // no need to delete child widgets, Qt does it all for us
}

void crmaccount::languageChange()
{
    retranslateUi(this);
}

enum SetResponse crmaccount::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  if (_mode == cView)
  {
    _customer->setEnabled(false);
    _prospect->setEnabled(false);
    _taxauth->setEnabled(false);
    _vendor->setEnabled(false);
    _partner->setEnabled(false);
    _competitor->setEnabled(false);
  }
  else
  {
    _customer->setEnabled(_privileges->check("MaintainCustomerMasters") && !_modal);
    _prospect->setEnabled(_privileges->check("MaintainProspects") && !_modal);
    _taxauth->setEnabled(_privileges->check("MaintainTaxAuthorities") && !_modal);
    _vendor->setEnabled(_privileges->check("MaintainVendors") && !_modal);
    _partner->setEnabled(_privileges->check("MaintainPartners"));
    _competitor->setEnabled(_privileges->check("MaintainCompetitorMasters"));
  }

  if (! _privileges->check("MaintainContacts") || _mode == cView)
    _edit->setText("View");
  _edit->setEnabled(_privileges->check("MaintainContacts"));
  _attach->setEnabled(_privileges->check("MaintainContacts"));
  _detach->setEnabled(_privileges->check("MaintainContacts"));

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _number->setFocus();

      q.prepare("SELECT createCrmAcct('TEMPORARY' || (last_value + 1), '',"
                "		      false, 'O', NULL, NULL, NULL, NULL,"
                "		      NULL, NULL, NULL, NULL) AS result "
                "FROM crmacct_crmacct_id_seq;");
      q.bindValue(":crmacct_id", _crmacctId);
      q.exec();
      if (q.first())
      {
        _crmacctId = q.value("result").toInt();
        if (_crmacctId < 0)
        {
          QMessageBox::critical(this, tr("Error creating Initial Account"),
                storedProcErrorLookup("createCrmAcct", _crmacctId));
          _crmacctId = -1;
          return UndefinedError;
        }
        _comments->setId(_crmacctId);
        _documents->setId(_crmacctId);
      }
      else if (q.lastError().type() != QSqlError::NoError)
      {
        systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
        return UndefinedError;
      }
      _number->clear();

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
      
      connect(_charass, SIGNAL(valid(bool)), _editCharacteristic, SLOT(setEnabled(bool)));
      connect(_charass, SIGNAL(valid(bool)), _deleteCharacteristic, SLOT(setEnabled(bool)));
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;   
      
      connect(_charass, SIGNAL(valid(bool)), _editCharacteristic, SLOT(setEnabled(bool)));
      connect(_charass, SIGNAL(valid(bool)), _deleteCharacteristic, SLOT(setEnabled(bool)));
      _name->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _owner->setEnabled(FALSE);
      _number->setEnabled(FALSE);
      _name->setEnabled(FALSE);
      _active->setEnabled(FALSE);
      _typeBox->setEnabled(FALSE);
      _primary->setEnabled(FALSE);
      _secondary->setEnabled(FALSE);
      _notes->setEnabled(FALSE);
      _comments->setEnabled(FALSE);
      _documents->setReadOnly(TRUE);
      _parentCrmacct->setEnabled(FALSE);
      _newCharacteristic->setEnabled(FALSE);
      _editCharacteristic->setEnabled(FALSE);
      _new->setEnabled(FALSE);
      //_newIncdt->setEnabled(FALSE);
      _newOpportunity->setEnabled(FALSE);
      _newTodo->setEnabled(FALSE);
      _deleteTodoIncdt->setEnabled(FALSE);
      _editTodoIncdt->setEnabled(FALSE);

      _attach->hide();
      _detach->hide();
      _save->hide();

      _close->setFocus();
    }
  }
  
  param = pParams.value("crmacct_id", &valid);
  if (valid)
  {
    _crmacctId = param.toInt();
    sPopulate();
  }

  int scripted;
  scripted = pParams.inList("scripted");
  if(scripted)
  {
	  _tab->setCurrentIndex(4);
  }
  
  if(_metrics->value("CRMAccountNumberGeneration") == "A")
    _number->setEnabled(FALSE);

  return NoError;
}

// similar code in address, customer, shipto, vendor, vendorAddress
int crmaccount::saveContact(ContactCluster* pContact)
{
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
		    tr("<p>There are multiple Contacts sharing this address (%1)."
		       " What would you like to do?")
		    .arg(pContact->label()),
		    tr("Change This One"),
		    tr("Change for All"),
		    tr("Cancel"),
		    2, 2);
  else if (-10 == saveResult)
    answer = QMessageBox::question(this,
		    tr("Question Saving %1").arg(pContact->label()),
		    tr("<p>Would you like to update the existing Contact or "
		       "create a new one?"),
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

void crmaccount::sClose()
{
  if (_mode == cNew)
  {
    if (QMessageBox::question(this, tr("Close without saving?"),
			      tr("<p>Are you sure you want to close this window "
				 "without saving the new CRM Account?"),
			      QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
      return;

    XSqlQuery rollback;
    rollback.prepare("ROLLBACK;");

    struct {
      QString spName;
      int     spArg;
    } toDelete[] = {
      {	"deleteCustomer",	_custId },
      { "deleteProspect",	_prospectId },
      { "deleteVendor",		_vendId },
      { "deleteTaxAuthority",	_taxauthId },
      /* any others? */
      { "deleteCRMAccount",	_crmacctId }
    };

    q.exec("BEGIN;");

    q.prepare("SELECT MIN(detachContact(cntct_id, :crmacct_id)) AS returnVal "
	      "FROM cntct "
	      "WHERE (cntct_crmacct_id=:crmacct_id);");
    q.bindValue(":crmacct_id", _crmacctId);
    q.exec();
    if (q.first())
    {
      int returnVal = q.value("returnVal").toInt();
      if (returnVal < 0)
      {
	rollback.exec();
	systemError(this, tr("Error detaching Contact from CRM Account (%1).")
			  .arg(returnVal), __FILE__, __LINE__);
	return;
      }
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      rollback.exec();
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    for (unsigned int i = 0; i < sizeof(toDelete) / sizeof(toDelete[0]); i++)
    {
      if (toDelete[i].spArg <= 0)
	continue;

      q.prepare(QString("SELECT %1(%2) AS returnVal;")
			.arg(toDelete[i].spName)
			.arg(toDelete[i].spArg));
      q.exec();
      if (q.first())
      {
	int returnVal = q.value("returnVal").toInt();
	if (returnVal < 0)
	{
	  rollback.exec();
	  systemError(this, storedProcErrorLookup(toDelete[i].spName, returnVal),
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
    } // for each thing to delete

    q.exec("COMMIT;");
    omfgThis->sCrmAccountsUpdated(_crmacctId);
  } // if cNew

  close();

}

void crmaccount::sSave()
{
  XSqlQuery rollback;
  rollback.prepare("ROLLBACK;");

  QString incomplete = tr("<p>The %1 relationship is selected but no "
			  "appropriate data have been created. Either click the"
			  " %2 button to enter the data or unselect %1.");
  struct {
    bool	condition;
    QString	msg;
    QWidget*	widget;
  } error[] = {
    { _number->text().trimmed().length() == 0,
      tr("<p>You must enter a number for this CRM Account before saving it."),
      _number
    },
    { _name->text().trimmed().length() == 0,
      tr("<p>You must enter a name for this CRM Account before saving it."),
      _name
    },
    { _customer->isChecked() && _custId <= 0,
      incomplete.arg(_customer->text()).arg(_customerButton->text()),
      _customerButton
    },
    { _prospect->isChecked() && _prospectId <= 0,
      incomplete.arg(_prospect->text()).arg(_prospectButton->text()),
      _prospectButton
    },
    { _vendor->isChecked() && _vendId <= 0,
      incomplete.arg(_vendor->text()).arg(_vendorButton->text()),
      _vendorButton
    },
    /*	nothing necessary for now
    { _partner->isChecked() && _partnerId <= 0,
      incomplete.arg(_partner->text()).arg(_partnerButton->text()),
      _partnerButton
    },
    { _competitor->isChecked() && _competitorId <= 0,
      incomplete.arg(_competitor->text()).arg(_competitorButton->text()),
      _competitorButton
    },
    */
    { _taxauth->isChecked() && _taxauthId <= 0,
      incomplete.arg(_taxauth->text()).arg(_taxauthButton->text()),
      _taxauthButton
    },
    { true, "", NULL }
  }; // error[]

  int errIndex;
  for (errIndex = 0; ! error[errIndex].condition; errIndex++)
    ;
  if (! error[errIndex].msg.isEmpty())
  {
    QMessageBox::critical(this, tr("Cannot Save CRM Account"),
			  error[errIndex].msg);
    error[errIndex].widget->setFocus();
    return;
  }

  q.prepare( "SELECT crmacct_number "
	     "FROM crmacct "
	     "WHERE ((crmacct_number=:number)"
	     " AND   (crmacct_id<>:crmacct_id) );" );

  q.bindValue(":crmacct_id", _crmacctId);
  q.bindValue(":number", _number->text());
  q.exec();
  if (q.first())
  {
    QMessageBox::critical(this, tr("Cannot Save CRM Account"),
			  storedProcErrorLookup("createCrmAcct", -1));
    _number->setFocus();
    return;
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  QString spName = "";
  int	  spArg = 0;
  int	  answer = QMessageBox::No;

  if (_prospectId > 0 && !_prospect->isChecked())
  {
    answer = QMessageBox::question(this, tr("Delete Prospect?"),
		    tr("<p>Are you sure you want to delete %1 as a Prospect?")
		      .arg(_number->text()),
		    QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    if (QMessageBox::No == answer)
    {
      _prospect->setChecked(true);
      return;
    }
    spName = "deleteProspect";
    spArg = _prospectId;
  }

  if (_custId > 0 && !_customer->isChecked())
  {
    answer = QMessageBox::question(this, tr("Delete Customer?"),
		    tr("<p>Are you sure you want to delete %1 as a Customer?")
		      .arg(_number->text()),
		    QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    if (QMessageBox::No == answer)
    {
      _customer->setChecked(true);
      return;
    }
    spName = "deleteCustomer";
    spArg = _custId;
  }

  if (_vendId > 0 && ! _vendor->isChecked())
  {
    answer = QMessageBox::question(this, tr("Delete Vendor?"),
		    tr("<p>Are you sure you want to delete %1 as a Vendor?")
		      .arg(_number->text()),
		    QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    if (QMessageBox::No == answer)
    {
      _vendor->setChecked(true);
      return;
    }
  }

  if (_taxauthId > 0 && ! _taxauth->isChecked())
  {
    answer = QMessageBox::question(this, tr("Delete Tax Authority?"),
		    tr("<p>Are you sure you want to delete %1 as a Tax Authority?")
		      .arg(_number->text()),
		    QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    if (QMessageBox::No == answer)
    {
      _taxauth->setChecked(true);
      return;
    }
  }
  
  q.exec("BEGIN;");
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  int returnVal = 0;
  
  _primary->setAccount(_crmacctId);
  _secondary->setAccount(_crmacctId);
  if (_primary->sChanged())
  {
    returnVal = saveContact(_primary);
    if (returnVal < 0)
    {
      rollback.exec();
      return;
    }
  }

  if (_secondary->sChanged())
  {
    returnVal = saveContact(_secondary);
    if (returnVal < 0)
    {
      rollback.exec();
      return;
    }
  }

  if (! spName.isEmpty())
  {
    q.exec(QString("SELECT %1(%2) AS returnVal;").arg(spName).arg(spArg));
    if (q.first())
    {
      returnVal = q.value("returnVal").toInt();
      if (returnVal < 0)
      {
        rollback.exec();
        systemError(this, storedProcErrorLookup(spName, returnVal), __FILE__, __LINE__);
        return;
      }
    } 
    else if (q.lastError().type() != QSqlError::NoError)
    {
      rollback.exec();
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }

  if (! _taxauth->isChecked() && _taxauthId > 0)
  {
    q.prepare("SELECT deleteTaxAuthority(:taxauthid) AS returnVal;");
    q.bindValue(":taxauthid", _taxauthId);
    q.exec();
    if (q.first())
    {
      returnVal = q.value("returnVal").toInt();
      if (returnVal < 0)
      {
        rollback.exec();
        systemError(this, storedProcErrorLookup("deleteTaxAuthority", returnVal), __FILE__, __LINE__);
        return;
      }
    } 
    else if (q.lastError().type() != QSqlError::NoError)
    {
      rollback.exec();
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
    _taxauthId = -1;
  }

  if (! _vendor->isChecked() && _vendId > 0)
  {
    q.prepare("SELECT deleteVendor(:vendid) AS returnVal;");
    q.bindValue(":vendid", _vendId);
    q.exec();
    if (q.first())
    {
      returnVal = q.value("returnVal").toInt();
      if (returnVal < 0)
      {
        rollback.exec();
        systemError(this, storedProcErrorLookup("deleteVendor", returnVal), __FILE__, __LINE__);
        return;
      }
    } 
    else if (q.lastError().type() != QSqlError::NoError)
    {
      rollback.exec();
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
    _vendId = -1;
  }

  if (saveNoErrorCheck() < 0)	// do the heavy lifting
  {
    rollback.exec();
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  q.exec("COMMIT;");
  _NumberGen = -1;
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  omfgThis->sCrmAccountsUpdated(_crmacctId);
  omfgThis->sCustomersUpdated(-1, TRUE);
  omfgThis->sProspectsUpdated();
  omfgThis->sVendorsUpdated();
  omfgThis->sTaxAuthsUpdated(-1);
  close();
}

int crmaccount::saveNoErrorCheck()
{
  q.prepare("UPDATE crmacct "
	    "SET crmacct_id=:crmacct_id,"
	    "    crmacct_number=UPPER(:crmacct_number),"
	    "    crmacct_name=:crmacct_name,"
	    "    crmacct_parent_id=:crmacct_parent_id,"
	    "    crmacct_active=:crmacct_active,"
	    "    crmacct_type=:crmacct_type,"
	    "    crmacct_cust_id=:crmacct_cust_id,"
	    "    crmacct_competitor_id=:crmacct_competitor_id,"
	    "    crmacct_partner_id=:crmacct_partner_id,"
	    "    crmacct_prospect_id=:crmacct_prospect_id,"
	    "    crmacct_taxauth_id=:crmacct_taxauth_id,"
	    "    crmacct_vend_id=:crmacct_vend_id,"
	    "    crmacct_cntct_id_1=:crmacct_cntct_id_1,"
	    "    crmacct_cntct_id_2 =:crmacct_cntct_id_2,"
            "    crmacct_owner_username =:crmacct_owner_username,"
	    "    crmacct_notes=:crmacct_notes "
	    "WHERE (crmacct_id=:crmacct_id);" );
  
  q.bindValue(":crmacct_id",		_crmacctId);
  q.bindValue(":crmacct_number",	_number->text());
  q.bindValue(":crmacct_name",		_name->text());
  q.bindValue(":crmacct_active",	QVariant(_active->isChecked()));
  q.bindValue(":crmacct_type",		_organization->isChecked() ? "O" : "I");
  q.bindValue(":crmacct_notes",		_notes->toPlainText());
  if (_custId > 0)	 q.bindValue(":crmacct_cust_id",	_custId);
  if (_competitorId > 0) q.bindValue(":crmacct_competitor_id",  _competitorId);
  if (_partnerId > 0)	 q.bindValue(":crmacct_partner_id",	_partnerId);
  if (_prospectId > 0)	 q.bindValue(":crmacct_prospect_id",    _prospectId);
  if (_taxauthId > 0)	 q.bindValue(":crmacct_taxauth_id",	_taxauthId);
  if (_vendor->isChecked())   q.bindValue(":crmacct_vend_id",	_vendId);
  if (_primary->id() > 0)   q.bindValue(":crmacct_cntct_id_1",	_primary->id());
  if (_secondary->id() > 0) q.bindValue(":crmacct_cntct_id_2",	_secondary->id());
  if (_parentCrmacct->id() > 0)
			q.bindValue(":crmacct_parent_id", _parentCrmacct->id());    
  if(_owner->isValid())
    q.bindValue(":crmacct_owner_username", _owner->username());

  if (! q.exec())
  {
    return -1;
  }

  return 0;
}

void crmaccount::sNewCharacteristic()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("crmacct_id", _crmacctId);

  characteristicAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sGetCharacteristics();
}

void crmaccount::sEditCharacteristic()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("charass_id", _charass->id());

  characteristicAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sGetCharacteristics();
}

void crmaccount::sDeleteCharacteristic()
{
  q.prepare( "DELETE FROM charass "
             "WHERE (charass_id=:charass_id);" );
  q.bindValue(":charass_id", _charass->id());
  q.exec();

  sGetCharacteristics();
}

void crmaccount::sGetCharacteristics()
{
  q.prepare( "SELECT charass_id, * "
             "FROM charass, char "
             "WHERE ( (charass_target_type='CRMACCT')"
             " AND (charass_char_id=char_id)"
             " AND (charass_target_id=:crmacct_id) ) "
             "ORDER BY char_name;" );
  q.bindValue(":crmacct_id", _crmacctId);
  q.exec();
  _charass->populate(q);
}

void crmaccount::sPopulate()
{
  q.prepare( "SELECT * "
             "FROM crmacct "
             "WHERE (crmacct_id=:crmacct_id);" );
  q.bindValue(":crmacct_id", _crmacctId);
  q.exec();
  if (q.first())
  {
    _number->setText(q.value("crmacct_number").toString());
    _name->setText(q.value("crmacct_name").toString());
    _active->setChecked(q.value("crmacct_active").toBool());

    QString acctType = q.value("crmacct_type").toString();
    if (acctType == "O")
      _organization->setChecked(true);
    else if (acctType == "I")
      _individual->setChecked(true);
    else
      qWarning("crmaccount::sPopulate() - acctType '%s' incorrect", acctType.toLatin1().data());

    _custId	= q.value("crmacct_cust_id").toInt();
    _competitorId = q.value("crmacct_competitor_id").toInt();
    _partnerId	= q.value("crmacct_partner_id").toInt();
    _prospectId	= q.value("crmacct_prospect_id").toInt();
    _vendId	= q.value("crmacct_vend_id").toInt();
    _taxauthId	= q.value("crmacct_taxauth_id").toInt();
    _primary->setId(q.value("crmacct_cntct_id_1").toInt());
    _secondary->setId(q.value("crmacct_cntct_id_2").toInt());
    _notes->setText(q.value("crmacct_notes").toString());
    _parentCrmacct->setId(q.value("crmacct_parent_id").toInt());
    _comments->setId(_crmacctId);
    _documents->setId(_crmacctId);
    _primary->setSearchAcct(_crmacctId);
    _secondary->setSearchAcct(_crmacctId);
    _owner->setUsername(q.value("crmacct_owner_username").toString());

    if (_custId > 0)
    {
      _customer->setChecked(true);
      _customerButton->setText("Customer");
      disconnect(_customerButton, SIGNAL(clicked()), this, SLOT(sCustomer()));
      
      QMenu * customerMenu = new QMenu;
      if (_mode == cEdit)
        customerMenu->addAction(tr("Edit..."), this, SLOT(sCustomer()));
      else
        customerMenu->addAction(tr("View..."), this, SLOT(sCustomer()));
      customerMenu->addAction(tr("Workbench..."),   this, SLOT(sCustomerInfo()));
      _customerButton->setMenu(customerMenu);
    }

    _customer->setDisabled(_customer->isChecked());
    _prospect->setChecked(_prospectId > 0);
    _prospect->setDisabled(_customer->isChecked());
    _taxauth->setChecked(_taxauthId > 0);
    if (_vendId > 0)
    {
      _vendor->setChecked(true);
      _vendorButton->setText("Vendor");
      disconnect(_vendorButton, SIGNAL(clicked()), this, SLOT(sVendor()));
      
      QMenu * vendorMenu = new QMenu;
      if (_mode == cEdit)
        vendorMenu->addAction(tr("Edit..."), this, SLOT(sVendor()));
      else
        vendorMenu->addAction(tr("View..."), this, SLOT(sVendor()));
      vendorMenu->addAction(tr("Workbench..."),   this, SLOT(sVendorInfo()));
      _vendorButton->setMenu(vendorMenu);   
    }
    _partner->setChecked(_partnerId > 0);
    _competitor->setChecked(_competitorId > 0);
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sPopulateContacts();
  sGetCharacteristics();
  sPopulateTodo();
  sPopulateOplist();
  sPopulateRegistrations();
}

void crmaccount::sPopulateContacts()
{
  q.prepare("SELECT * "
	    "FROM cntct "
	    "WHERE (cntct_crmacct_id=:crmacct_id) "
	    "ORDER BY cntct_last_name, cntct_first_name;");
  q.bindValue(":crmacct_id", _crmacctId);
  q.exec();
  _contacts->populate(q);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void crmaccount::sPopulateTodo()
{
  _todo->clear();

  if (! _showTodo->isChecked() && ! _showIncdt->isChecked())
    return;

  QString sql = "SELECT *, "
                "       CASE WHEN (status != 'C' AND due < CURRENT_DATE) THEN"
                "            'expired'"
                "            WHEN (status != 'C' AND due > CURRENT_DATE) THEN"
                "            'future'"
                "       END AS due_qtforegroundrole "
                "FROM ("
                "<? if exists(\"showTodo\") ?>"
		"SELECT todoitem_id AS id, "
		"       'T' AS type, incdtpriority_name AS priority, incdtpriority_order, "
		"       todoitem_name AS name, "
		"       firstLine(todoitem_description) AS descrip, "
		"       todoitem_status AS status, todoitem_due_date AS due, "
		"       todoitem_username AS usr, incdt_number AS incdt "
		"FROM todoitem LEFT OUTER JOIN "
		"     incdt ON (incdt_id=todoitem_incdt_id) "
                "     LEFT OUTER JOIN incdtpriority ON (incdtpriority_id=todoitem_priority_id) "
		"WHERE ( (todoitem_crmacct_id=<? value(\"crmacct_id\") ?>)"
		"  <? if not exists(\"completed\") ?>"
		"    AND   (todoitem_status != 'C')"
		"  <? endif ?>"
		"  <? if exists(\"active\") ?>AND (todoitem_active) <? endif ?>"
		"  <? if exists(\"username\") ?>"
		"    AND (todoitem_username=<? value(\"username\") ?>)"
		"  <? endif ?>"
		"       ) "
		"  <? if exists(\"showIncidents\")?>"
		"  UNION "
		"  <? endif ?>"
		"<? endif ?>"
		"<? if exists(\"showIncidents\")?>"
		"SELECT incdt_id AS id, "
		"       'I' AS type, incdtpriority_name AS priority, incdtpriority_order, "
		"       incdt_summary AS name, "
		"       firstLine(incdt_descrip) AS descrip, "
		"       incdt_status AS status, CAST(NULL AS DATE) AS due, "
		"       incdt_assigned_username AS usr, incdt_number AS incdt "
		"  FROM incdt "
                "       LEFT OUTER JOIN incdtpriority ON (incdtpriority_id=incdt_incdtpriority_id) "
		"WHERE ((incdt_crmacct_id=<? value(\"crmacct_id\") ?>)"
		"  <? if not exists(\"completed\") ?> "
		"   AND (incdt_status != 'L')"
		"  <? endif ?>"
		"       ) "
		"<? endif ?>"
                ") AS sub "
		"ORDER BY incdt, incdtpriority_order, usr;" ;

  ParameterList params;
  params.append("crmacct_id", _crmacctId);
  if (_showTodo->isChecked())
    params.append("showTodo");
  if (_showIncdt->isChecked())
    params.append("showIncidents");
  if (_activeTodoIncdt->isChecked())
    params.append("active");
  if (_completedTodoIncdt->isChecked())
    params.append("completed");
  if ((_privileges->check("MaintainPersonalTodoList") ||
       _privileges->check("ViewPersonalTodoList")) &&
      ! (_privileges->check("MaintainOtherTodoLists") ||
	 _privileges->check("ViewOtherTodoLists")) )
    params.append("username", omfgThis->username());

  MetaSQLQuery mql(sql);
  XSqlQuery itemQ = mql.toQuery(params);
  _todo->populate(itemQ);
  if (itemQ.lastError().type() != QSqlError::NoError)
  {
    systemError(this, itemQ.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sHandleTodoPrivs();
}

void crmaccount::sHandleTodoPrivs()
{
  bool newTodoPriv = ((cEdit == _mode || cNew == _mode) &&
		      (_privileges->check("MaintainPersonalTodoList") ||
		       _privileges->check("MaintainOtherTodoLists")) );

  _newTodo->setEnabled((cEdit == _mode || cNew == _mode) &&
			(_privileges->check("MaintainIncidents") || newTodoPriv));

  if (_todo->currentItem())
  {
    bool editTodoPriv = (cEdit == _mode || cNew == _mode) && (
      (omfgThis->username() == _todo->currentItem()->text("usr") && _privileges->check("MaintainPersonalTodoList")) ||
      (_privileges->check("MaintainOtherTodoLists")) );

    bool viewTodoPriv =
      (omfgThis->username() == _todo->currentItem()->text("usr") && _privileges->check("ViewPersonalTodoList")) ||
      (_privileges->check("ViewOtherTodoLists"));

    if (_todo->currentItem()->text(0) == "T")
      _editTodoIncdt->setEnabled(editTodoPriv);
    else
      _editTodoIncdt->setEnabled((cEdit == _mode || cNew == _mode) &&
				  _privileges->check("MaintainIncidents"));

    if (_todo->currentItem()->text(0) == "T")
      _viewTodoIncdt->setEnabled(viewTodoPriv);
    else
      _viewTodoIncdt->setEnabled(_privileges->check("ViewIncidents"));

    if (_todo->currentItem()->text(0) == "T")
      _deleteTodoIncdt->setEnabled(editTodoPriv);
    else
      _deleteTodoIncdt->setEnabled((cEdit == _mode || cNew == _mode) &&
				    _privileges->check("MaintainIncidents"));
  }
  else
  {
    _editTodoIncdt->setEnabled(false);
    _viewTodoIncdt->setEnabled(false);
    _deleteTodoIncdt->setEnabled(false);
  }
}

void crmaccount::sPopulateTodoMenu(QMenu *pMenu)
{
  int menuItem;

  bool newTodoPriv = ((cEdit == _mode || cNew == _mode) &&
		      (_privileges->check("MaintainPersonalTodoList") ||
		       _privileges->check("MaintainOtherTodoLists")) );

  bool editTodoPriv = (cEdit == _mode || cNew == _mode) && (
      (omfgThis->username() == _todo->currentItem()->text("usr") && _privileges->check("MaintainPersonalTodoList")) ||
      _privileges->check("MaintainOtherTodoLists"));

  bool viewTodoPriv =
      (omfgThis->username() == _todo->currentItem()->text("usr") && _privileges->check("ViewPersonalTodoList")) ||
      _privileges->check("ViewOtherTodoLists");

  menuItem = pMenu->insertItem(tr("New To-Do Item..."), this, SLOT(sNewTodo()));
  pMenu->setItemEnabled(menuItem, newTodoPriv);

  menuItem = pMenu->insertItem(tr("New Incident..."), this, SLOT(sNewIncdt()));
  pMenu->setItemEnabled(menuItem, (cEdit == _mode || cNew == _mode) &&
				  _privileges->check("MaintainIncidents"));

  if (_todo->currentItem())
  {
    menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEditTodoIncdt()));
    pMenu->setItemEnabled(menuItem,
			  _todo->currentItem()->text(0) == "T" ? editTodoPriv :
			       (cEdit == _mode || cNew == _mode) &&
				_privileges->check("MaintainIncidents") );

    menuItem = pMenu->insertItem(tr("View..."), this, SLOT(sViewTodoIncdt()));
    pMenu->setItemEnabled(menuItem,
			  _todo->currentItem()->text(0) == "T" ? viewTodoPriv :
				_privileges->check("ViewIncidents"));

    menuItem = pMenu->insertItem(tr("Delete"), this, SLOT(sDeleteTodoIncdt()));
    pMenu->setItemEnabled(menuItem,
			  _todo->currentItem()->text(0) == "T" ? editTodoPriv :
			      (cEdit == _mode || cNew == _mode) &&
				_privileges->check("MaintainIncidents"));
  }
}

void crmaccount::sCompetitor()
{
  _competitorId = (_competitor->isChecked() ? 1 : 0);
}

void crmaccount::sCustomer()
{
  ParameterList params;
  
  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  if (! _customer->isChecked())	// don't know how we got here
    return;

  QString confirm;

  if (_prospectId > 0)
  {
	if (QMessageBox::question(this, tr("Convert"),
		tr("<p>Are you sure you want to convert this "
		       "Prospect to a Customer?"),
		QMessageBox::Yes,
		QMessageBox::No | QMessageBox::Default) == QMessageBox::No)
	return;
	
	params.append("prospect_id",	  _prospectId);
	params.append("mode","new");
	
  }
  else if (_custId <= 0)
  {
    params.append("crmacct_id",	  _crmacctId);
    params.append("mode","new");
  } 	
  else
  {
    params.append("cust_id",_custId);
    params.append("mode",		  (_mode == cView) ? "view" : "edit");
  }

  customer *newdlg = new customer(this);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
  
}

void crmaccount::sPartner()
{
  _partnerId = (_partner->isChecked() ? 1 : 0);
}

void crmaccount::sProspect()
{
  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  if (! _prospect->isChecked())	// don't know how we got here
    return;

  QString spName = "";
  int	  spArg = 0;

  if (_custId > 0)
  {
    spName = "convertCustomerToProspect";
    spArg = _custId;
    if (QMessageBox::question(this, tr("Convert"),
		    tr("<p>Are you sure you want to convert this Customer to "
		       "a Prospect and delete its Customer information?"),
		    QMessageBox::Yes,
		    QMessageBox::No | QMessageBox::Default) == QMessageBox::No)
      return;
  }
  else if (_prospectId <= 0)
  {
    spName = "createProspect";
    spArg = _crmacctId;
  }

  if (! spName.isEmpty())
  {
    q.exec(QString("SELECT %1(%2) AS returnVal;").arg(spName).arg(spArg));
    if (q.first())
    {
      int returnVal = q.value("returnVal").toInt();
      if (returnVal < 0)
      {
	systemError(this, storedProcErrorLookup(spName, returnVal), __FILE__, __LINE__);
	return;
      }
      _custId = -1;
      _prospectId = returnVal;
    } 
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    omfgThis->sProspectsUpdated();
    omfgThis->sCustomersUpdated(_custId, true);
  }

  ParameterList params;
  params.append("crmacct_id",	  _crmacctId);
  params.append("crmacct_number", _number->text());
  params.append("crmacct_name",	  _name->text());
  params.append("prospect_id",	  _prospectId);
  params.append("mode",		  (_mode == cView) ? "view" : "edit");

  prospect *newdlg = new prospect(this);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void crmaccount::sTaxAuth()
{
  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  ParameterList params;
  params.append("crmacct_id", _crmacctId);
  params.append("crmacct_number", _number->text());
  params.append("crmacct_name", _name->text());
  if (_taxauthId <= 0 && _privileges->check("MaintainTaxAuthorities") &&
      (_mode == cEdit || _mode == cNew))
    params.append("mode", "new");
  else if (_taxauthId <= 0)
    systemError(this, tr("No existing Tax Authorities to View"));
  else if (_taxauthId > 0 && _privileges->check("MaintainTaxAuthorities") &&
	    (_mode == cEdit || _mode == cNew))
  {
    params.append("taxauth_id", _taxauthId);
    params.append("mode", "edit");
  }
  else if (_taxauthId > 0 && _mode == cView)
  {
    params.append("taxauth_id", _taxauthId);
    params.append("mode", "view");
  }

  taxAuthority newdlg(this);
  newdlg.set(params);
  newdlg.exec();
}

void crmaccount::sVendor()
{
  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  ParameterList params;
  params.append("crmacct_id", _crmacctId);
  params.append("crmacct_number", _number->text());
  params.append("crmacct_name", _name->text());
  if (_vendId <= 0 && _privileges->check("MaintainVendors") &&
      (_mode == cEdit || _mode == cNew))
    params.append("mode", "new");
  else if (_vendId <= 0)
    systemError(this, tr("No existing Vendor to View"));
  else if (_vendId > 0 && _privileges->check("MaintainVendors") &&
	    (_mode == cEdit || _mode == cNew))
  {
    params.append("vend_id", _vendId);
    params.append("mode", "edit");
  }
  else if (_vendId > 0 && _mode == cView)
  {
    params.append("vend_id", _vendId);
    params.append("mode", "view");
  }

  vendor *newdlg = new vendor(this);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void crmaccount::sNew()
{
  ParameterList params;
  params.append("crmacct_id", _crmacctId);
  params.append("mode", "new");

  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  contact newdlg(this);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sPopulate();
}

void crmaccount::sEdit()
{
  ParameterList params;
  params.append("cntct_id", _contacts->id());
  params.append("crmacct_id", _crmacctId);

  if (_mode == cView && _contacts->id() > 0)
    params.append("mode", "view");
  else if (_privileges->check("MaintainContacts") && _contacts->id() > 0)
  {
    params.append("mode", "edit");
    params.append("cntct_id", _contacts->id());
  }
  else  if (_privileges->check("MaintainContacts") && _contacts->id() <= 0)
    params.append("mode", "new");

  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  contact *newdlg = new contact();
  newdlg->set(params);
  if (newdlg->exec() != XDialog::Rejected)
    sPopulate();
}

void crmaccount::sView()
{
  ParameterList params;
  params.append("cntct_id", _contacts->id());

  params.append("mode", "view");

  contact *newdlg = new contact();
  newdlg->set(params);
  if (newdlg->exec() != XDialog::Rejected)
    sPopulate();
}

void crmaccount::sAttach()
{
  ContactCluster attached(this, "attached");
  attached.sEllipses();
  if (attached.id() > 0)
  {
    int answer = QMessageBox::Yes;

    if (attached.crmAcctId() > 0 && attached.crmAcctId() != _crmacctId)
      answer = QMessageBox::question(this, tr("Detach Contact?"),
			    tr("<p>This Contact is currently attached to a "
			       "different CRM Account. Are you sure you want "
			       "to change the CRM Account for this person?"),
			    QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    if (answer == QMessageBox::Yes)
    {
      q.prepare("SELECT attachContact(:cntct_id, :crmacct_id) AS returnVal;");
      q.bindValue(":cntct_id", attached.id());
      q.bindValue(":crmacct_id", _crmacctId);
      q.exec();
      if (q.first())
      {
	int returnVal = q.value("returnVal").toInt();
	if (returnVal < 0)
	{
	  systemError(this, storedProcErrorLookup("attachContact", returnVal),
			    __FILE__, __LINE__);
	  return;
	}
      }
      else if (q.lastError().type() != QSqlError::NoError)
      {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return;
      }
    }
    sPopulate();
  }
}

void crmaccount::sDetach()
{
  int answer = QMessageBox::question(this, tr("Detach Contact?"),
			tr("<p>Are you sure you want to detach this Contact "
			   "from this CRM Account?"),
			QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
  if (answer == QMessageBox::Yes)
  {
    q.prepare("SELECT detachContact(:cntct_id, :crmacct_id) AS returnVal;");
    q.bindValue(":cntct_id", _contacts->id());
    q.bindValue(":crmacct_id", _crmacctId);
    q.exec();
    if (q.first())
    {
      int returnVal = q.value("returnVal").toInt();
      if (returnVal < 0)
      {
	systemError(this, tr("Error detaching Contact from CRM Account (%1).")
			  .arg(returnVal), __FILE__, __LINE__);
	return;
      }
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    sPopulate();
  }
}

void crmaccount::sNewTodo()
{
  ParameterList params;
  params.append("crmacct_id", _crmacctId);
  params.append("mode", "new");

  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  todoItem newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sPopulateTodo();
}

void crmaccount::sNewIncdt()
{
  ParameterList params;
  params.append("crmacct_id", _crmacctId);
  params.append("mode", "new");

  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  incident newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sPopulateTodo();
}

void crmaccount::sEditTodoIncdt()
{
  if (_todo->currentItem()->text(0) == "T")
    sEditTodo();
  else
    sEditIncdt();
}

void crmaccount::sEditTodo()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("todoitem_id", _todo->id());

  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  todoItem newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sPopulateTodo();
}

int crmaccount::getIncidentId()
{
  int returnVal = -1;

  if (_todo->currentItem()->text(0) == "I")
    returnVal = _todo->id();
  else if (! _todo->currentItem()->text(7).isEmpty())
  {
    XSqlQuery incdt;
    incdt.prepare("SELECT incdt_id FROM incdt WHERE (incdt_number=:number);");
    incdt.bindValue(":number", _todo->currentItem()->text(7).toInt());
    if (incdt.exec() && incdt.first())
     returnVal = incdt.value("incdt_id").toInt();
    else if (incdt.lastError().type() != QSqlError::NoError)
      systemError(this, incdt.lastError().databaseText(), __FILE__, __LINE__);
  }

  return returnVal;
}

void crmaccount::sEditIncdt()
{
  ParameterList params;
  params.append("mode",     "edit");
  params.append("incdt_id", getIncidentId());

  if (saveNoErrorCheck() < 0)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  incident newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    sPopulateTodo();
}

void crmaccount::sDeleteTodoIncdt()
{
  if (_todo->currentItem()->text(0) == "T")
    sDeleteTodo();
  else
    sDeleteIncdt();
}

void crmaccount::sDeleteTodo()
{
  q.prepare("SELECT deleteTodoItem(:todoitem_id) AS result;");
  q.bindValue(":todoitem_id", _todo->id());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("deleteTodoItem", result));
      return;
    }
    sPopulateTodo();
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void crmaccount::sDeleteIncdt()
{
  q.prepare("SELECT deleteIncident(:incdt_id) AS result;");
  q.bindValue(":incdt_id", getIncidentId());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("deleteIncident", result));
      return;
    }
    sPopulateTodo();
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void crmaccount::sViewTodoIncdt()
{
  if (_todo->currentItem()->text(0) == "T")
    sViewTodo();
  else
    sViewIncdt();
}

void crmaccount::sViewTodo()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("todoitem_id", _todo->id());

  todoItem newdlg(this, "", true);
  newdlg.set(params);
  newdlg.exec();
}

void crmaccount::sViewIncdt()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("incdt_id", getIncidentId());

  incident newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void crmaccount::sUpdateRelationships()
{
  q.prepare( "SELECT * "
             "FROM crmacct "
             "WHERE (crmacct_id=:crmacct_id);" );
  q.bindValue(":crmacct_id", _crmacctId);
  q.exec();
  if (q.first())
  {
    _custId	= q.value("crmacct_cust_id").toInt();
    _competitorId = q.value("crmacct_competitor_id").toInt();
    _partnerId	= q.value("crmacct_partner_id").toInt();
    _prospectId	= q.value("crmacct_prospect_id").toInt();
    _taxauthId	= q.value("crmacct_taxauth_id").toInt();
    _vendId	= q.value("crmacct_vend_id").toInt();

    _customer->setChecked(_custId > 0);
    _customer->setDisabled(_customer->isChecked());
    _prospect->setChecked(_prospectId > 0);
    _prospect->setDisabled(_customer->isChecked());
    _taxauth->setChecked(_taxauthId > 0);
    _vendor->setChecked(_vendId > 0);
    _partner->setChecked(_partnerId > 0);
    _competitor->setChecked(_competitorId > 0);
    _owner->setUsername(q.value("crmacct_owner_username").toString());
  //  _custInfoButton->setEnabled(_custId > 0 && _customer->isChecked());
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  
  sPopulateContacts();
}

void crmaccount::sPopulateMenu(QMenu *pMenu)
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("New..."), this, SLOT(sNew()), 0);
  if (!_privileges->check("MaintainContacts") || _mode == cView)
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEdit()), 0);
  if (!_privileges->check("MaintainContacts") || _mode == cView)
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("View..."), this, SLOT(sView()), 0);
  if (!_privileges->check("ViewContacts"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Attach..."), this, SLOT(sAttach()), 0);
  if (!_privileges->check("MaintainContacts") || _mode == cView)
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Detach"), this, SLOT(sDetach()), 0);
  if (!_privileges->check("MaintainContacts") || _mode == cView)
    pMenu->setItemEnabled(menuItem, FALSE);
}

void crmaccount::sPopulateOplistMenu(QMenu *pMenu)
{
  int menuItem;

  bool editPriv = _privileges->check("MaintainOpportunities");
  bool viewPriv = _privileges->check("ViewOpportunities") || editPriv;

  menuItem = pMenu->insertItem(tr("New..."), this, SLOT(sOplistNew()), 0);
  pMenu->setItemEnabled(menuItem, editPriv);

  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sOplistEdit()), 0);
  pMenu->setItemEnabled(menuItem, editPriv);

  menuItem = pMenu->insertItem(tr("View..."), this, SLOT(sOplistView()), 0);
  pMenu->setItemEnabled(menuItem, viewPriv);

  menuItem = pMenu->insertItem(tr("Delete"), this, SLOT(sOplistDelete()), 0);
  pMenu->setItemEnabled(menuItem, editPriv);
}

void crmaccount::doDialog(QWidget *parent, const ParameterList & pParams)
{
  //XDialog newdlg(parent);
  //QVBoxLayout * vbox = new QVBoxLayout(&newdlg);
  crmaccount * ci = new crmaccount(parent);
  //vbox->addWidget(ci);
  //newdlg.setWindowTitle(ci->windowTitle());
  //ParameterList params;
  //params = pParams;
  //params.append("modal");
  ci->set(pParams);
  //newdlg.exec();
  omfgThis->handleNewWindow(ci);
}

void crmaccount::sHandleAutoUpdate()
{
  if (_autoUpdateTodo)
    connect(omfgThis, SIGNAL(tick()), this, SLOT(sPopulateTodo()));
  else
    disconnect(omfgThis, SIGNAL(tick()), this, SLOT(sPopulateTodo()));
}

void crmaccount::sCustomerInfo()
{
  ParameterList params;
  params.append("cust_id", _custId);

  dspCustomerInformation *newdlg = new dspCustomerInformation(this, Qt::Window);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void crmaccount::sVendorInfo()
{
  ParameterList params;
  params.append("vend_id", _vendId);

  vendorWorkBench *newdlg = new vendorWorkBench(this, "vendorWorkBench", Qt::Window);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void crmaccount::sCustomerToggled()
{
  if (_customer->isChecked())
    _prospect->setChecked(FALSE);

//  _custInfoButton->setEnabled(_custId > 0 && _customer->isChecked());
}

void crmaccount::sProspectToggled()
{
  if (_prospect->isChecked())
    _customer->setChecked(FALSE);
}

void crmaccount::sOplistNew()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("crmacct_id", _crmacctId);

  opportunity newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sPopulateOplist();
}

void crmaccount::sOplistDelete()
{
  q.prepare("SELECT deleteOpportunity(:ophead_id) AS result;");
  q.bindValue(":ophead_id", _oplist->id());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("deleteOpportunity", result));
      return;
    }
    else
      sPopulateOplist();
    }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

}

void crmaccount::sOplistView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("ophead_id", _oplist->id());

  opportunity newdlg(this, "", TRUE);
  newdlg.set(params);

  newdlg.exec();
}

void crmaccount::sOplistEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("ophead_id", _oplist->id());

  opportunity newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sPopulateOplist();
}

void crmaccount::sPopulateOplist()
{
  MetaSQLQuery mql = mqlLoad("opportunitiesByCRM", "detail");

  ParameterList params;
  params.append("crmacct_id", _crmacctId);

  XSqlQuery itemQ = mql.toQuery(params);

  if (itemQ.lastError().type() != QSqlError::NoError)
  {
    systemError(this, itemQ.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  _oplist->populate(itemQ);
}

void crmaccount::sPopulateRegistrations()
{
  if (_metrics->boolean("LotSerialControl"))
  {
     q.prepare( "SELECT lsreg_id,ls_number,item_number, item_descrip1, "
             "lsreg_qty,'qty' AS lsreg_qty_xtnumericrole,lsreg_solddate,"
	     "lsreg_regdate,lsreg_expiredate, "
	     "CASE WHEN lsreg_expiredate <= current_date THEN "
	     "  'expired' "
	     "END AS lsreg_expiredate_qtforegroundrole "
             "FROM lsreg,ls,item  "
             "WHERE ((lsreg_ls_id=ls_id) "
	     "AND (ls_item_id=item_id) "
             "AND (lsreg_crmacct_id=:crmacct_id));");
    q.bindValue(":crmacct_id", _crmacctId);
    q.exec();
    _reg->clear();
    _reg->populate(q);
    if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void crmaccount::sNewReg()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("crmacct_id", _crmacctId);

  lotSerialRegistration newdlg(this, "", TRUE);
  newdlg.set(params);
  
  newdlg.exec();
  sPopulateRegistrations();
}

void crmaccount::sEditReg()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("lsreg_id", _reg->id());

  lotSerialRegistration newdlg(this, "", TRUE);
  newdlg.set(params);
  
  newdlg.exec();
  sPopulateRegistrations();
}

void crmaccount::sDeleteReg()
{
  q.prepare( "DELETE FROM lsreg "
             "WHERE (lsreg_id=:lsreg_id);"
             "DELETE FROM charass "
             "WHERE ((charass_target_type='LSR') "
             "AND (charass_target_id=:lsreg_id))" );
  q.bindValue(":lsreg_id", _reg->id());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sPopulateRegistrations();
}

void crmaccount::sCheckNumber()
{
  if (DEBUG) qDebug("crmaccount::sCheckNumber()");
  if (_number->text().length())
  {
    _number->setText(_number->text().trimmed().toUpper());

    XSqlQuery newq;
    newq.prepare("SELECT crmacct_id "
                 "FROM crmacct "
                 "WHERE ((UPPER(crmacct_number)=UPPER(:crmacct_number))"
                 "   AND (crmacct_id!=:crmacct_id));" );
    newq.bindValue(":crmacct_number", _number->text());
    newq.bindValue(":crmacct_id",     _crmacctId);
    newq.exec();
    if (newq.first())
    {
      XSqlQuery query;
      if(cNew == _mode && -1 != _NumberGen && _number->text().toInt() != _NumberGen)
      {
        query.prepare( "SELECT releaseCRMAccountNumber(:Number);" );
        query.bindValue(":Number", _NumberGen);
        query.exec();
        _NumberGen = -1;
      }
      // Delete temporary
      query.prepare( "DELETE FROM crmacct WHERE (crmacct_id=:crmacct_id);" );
      query.bindValue(":crmacct_id", _crmacctId);
      query.exec();
      if (query.lastError().type() != QSqlError::NoError)
      {
	systemError(this, query.lastError().databaseText(), __FILE__, __LINE__);
	return;
      }
      
      _crmacctId = newq.value("crmacct_id").toInt();
      _mode = cEdit;
      sPopulate();

      connect(_charass, SIGNAL(valid(bool)), _editCharacteristic, SLOT(setEnabled(bool)));
      connect(_charass, SIGNAL(valid(bool)), _deleteCharacteristic, SLOT(setEnabled(bool)));
      _name->setFocus();
      _number->setEnabled(FALSE);
    }
    else if (newq.lastError().type() != QSqlError::NoError)
    {
      systemError(this, newq.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void crmaccount::closeEvent(QCloseEvent *pEvent)
{
  if(cNew == _mode && -1 != _NumberGen)
  {
    XSqlQuery query;
    query.prepare( "SELECT releaseCRMAccountNumber(:Number);" );
    query.bindValue(":Number", _NumberGen);
    query.exec();
    _NumberGen = -1;
  }
  QWidget::closeEvent(pEvent);
}

void crmaccount::sHandleButtons() 	 
{ 	 
	if (_primaryButton->isChecked()) 	 
	  _widgetStack->setCurrentIndex(_widgetStack->indexOf(_primaryPage)); 	 
	else if (_secondaryButton->isChecked()) 	 
	  _widgetStack->setCurrentIndex(_widgetStack->indexOf(_secondaryPage)); 	 
  else 	 
    _widgetStack->setCurrentIndex(_widgetStack->indexOf(_allPage)); 	 
}
