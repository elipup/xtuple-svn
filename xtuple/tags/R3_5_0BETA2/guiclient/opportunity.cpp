/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "opportunity.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>
#include <QMenu>

#include <openreports.h>

#include "storedProcErrorLookup.h"
#include "todoItem.h"
#include "socluster.h"
#include "salesOrder.h"
#include "salesOrderList.h"
#include "quoteList.h"
#include "printQuote.h"
#include "printSoForm.h"
#include "characteristicAssignment.h"

/*
 *  Constructs a opportunity as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
opportunity::opportunity(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  if(!_privileges->check("EditOwner")) _owner->setEnabled(false);

  // signals and slots connections
  connect(_crmacct, SIGNAL(newId(int)), this, SLOT(sHandleCrmacct(int)));
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_deleteTodoItem, SIGNAL(clicked()), this, SLOT(sDeleteTodoItem()));
  connect(_viewTodoItem, SIGNAL(clicked()), this, SLOT(sViewTodoItem()));
  connect(_editTodoItem, SIGNAL(clicked()), this, SLOT(sEditTodoItem()));
  connect(_newTodoItem, SIGNAL(clicked()), this, SLOT(sNewTodoItem()));
  connect(_todoList, SIGNAL(itemSelected(int)), _editTodoItem, SLOT(animateClick()));
  connect(_todoList, SIGNAL(populateMenu(QMenu*, QTreeWidgetItem*, int)), this, SLOT(sPopulateTodoMenu(QMenu*)));
  connect(_todoList, SIGNAL(valid(bool)), this, SLOT(sHandleTodoPrivs()));
  connect(_deleteSale, SIGNAL(clicked()), this, SLOT(sDeleteSale()));
  connect(_viewSale, SIGNAL(clicked()), this, SLOT(sViewSale()));
  connect(_editSale, SIGNAL(clicked()), this, SLOT(sEditSale()));
  connect(_convertQuote, SIGNAL(clicked()), this, SLOT(sConvertQuote()));
  connect(_printSale, SIGNAL(clicked()), this, SLOT(sPrintSale()));
  connect(_salesList, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*)), this, SLOT(sPopulateSalesMenu(QMenu*)));
  connect(_salesList, SIGNAL(valid(bool)), this, SLOT(sHandleSalesPrivs()));
  connect(omfgThis,	SIGNAL(quotesUpdated(int, bool)), this, SLOT(sFillSalesList()));
  connect(omfgThis,	SIGNAL(salesOrdersUpdated(int, bool)), this, SLOT(sFillSalesList()));
  connect(_charass, SIGNAL(itemSelected(int)), _editCharacteristic, SLOT(animateClick()));
  connect(_newCharacteristic, SIGNAL(clicked()), this, SLOT(sNewCharacteristic()));
  connect(_editCharacteristic, SIGNAL(clicked()), this, SLOT(sEditCharacteristic()));
  connect(_deleteCharacteristic, SIGNAL(clicked()), this, SLOT(sDeleteCharacteristic()));

  _probability->setValidator(0);
  
  _opheadid = -1;
  _custid = -1;
  _prospectid = -1;
  
  _todoList->addColumn(tr("Priority"),   _userColumn, Qt::AlignRight, true, "incdtpriority_name");
  _todoList->addColumn(tr("User"),       _userColumn, Qt::AlignLeft,  true, "todoitem_username" );
  _todoList->addColumn(tr("Name"),               100, Qt::AlignLeft,  true, "todoitem_name" );
  _todoList->addColumn(tr("Description"),         -1, Qt::AlignLeft,  true, "todoitem_description" );
  _todoList->addColumn(tr("Status"),   _statusColumn, Qt::AlignLeft,  true, "todoitem_status" );
  _todoList->addColumn(tr("Due Date"),   _dateColumn, Qt::AlignLeft,  true, "todoitem_due_date" );

  _salesList->addColumn(tr("Doc #"),       _orderColumn, Qt::AlignLeft,  true, "sale_number" );
  _salesList->addColumn(tr("Type"),                  -1, Qt::AlignLeft,  true, "sale_type" );
  _salesList->addColumn(tr("Date"),         _dateColumn, Qt::AlignLeft,  true, "sale_date" );
  _salesList->addColumn(tr("Ext. Price"),  _priceColumn, Qt::AlignRight, true, "sale_extprice");

  _charass->addColumn(tr("Characteristic"), _itemColumn, Qt::AlignLeft,  true, "char_name" );
  _charass->addColumn(tr("Value"),          -1,          Qt::AlignLeft,  true, "charass_value" );
  _charass->addColumn(tr("Default"),        _ynColumn,   Qt::AlignCenter,true, "charass_default" );

  _owner->setUsername(omfgThis->username());
  _owner->setType(UsernameLineEdit::UsersActive);

  _saved = false;
}

/*
 *  Destroys the object and frees any allocated resources
 */
opportunity::~opportunity()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void opportunity::languageChange()
{
  retranslateUi(this);
}

enum SetResponse opportunity::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("ophead_id", &valid);
  if (valid)
  {
    _opheadid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      connect(_charass, SIGNAL(valid(bool)), _editCharacteristic, SLOT(setEnabled(bool)));
      connect(_charass, SIGNAL(valid(bool)), _deleteCharacteristic, SLOT(setEnabled(bool)));

      _comments->setReadOnly(true);
      
      param = pParams.value("crmacct_id", &valid);
      if (valid)
        _crmacct->setId(param.toInt());
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      connect(_charass, SIGNAL(valid(bool)), _editCharacteristic, SLOT(setEnabled(bool)));
      connect(_charass, SIGNAL(valid(bool)), _deleteCharacteristic, SLOT(setEnabled(bool)));

      _crmacct->setEnabled(true);
      _save->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _crmacct->setEnabled(false);
      _owner->setEnabled(false);
      _oppstage->setEnabled(false);
      _oppsource->setEnabled(false);
      _opptype->setEnabled(false);
      _notes->setReadOnly(true);
      _name->setEnabled(false);
      _targetDate->setEnabled(false);
      _actualDate->setEnabled(false);
      _amount->setEnabled(false);
      _probability->setEnabled(false);
      _deleteTodoItem->setEnabled(false);
      _editTodoItem->setEnabled(false);
      _newTodoItem->setEnabled(false);
      _deleteSale->setEnabled(false);
      _editSale->setEnabled(false);
      _printSale->setEnabled(false);
      _newSale->setEnabled(false);
      _attachSale->setEnabled(false);
      _newCharacteristic->setEnabled(FALSE);

      _save->hide();
      _cancel->setText(tr("&Close"));
      _cancel->setFocus();
      _comments->setReadOnly(true);
    }
  }

  param = pParams.value("crmacct_id", &valid);
  if (valid)
  {
    _crmacct->setId(param.toInt());
    _crmacct->setEnabled(false);
  }

  sHandleTodoPrivs();
  sHandleSalesPrivs();
  return NoError;
}

void opportunity::sCancel()
{
  if (_saved && cNew == _mode)
  {
    q.prepare("SELECT deleteOpportunity(:ophead_id) AS result;");
    q.bindValue(":ophead_id", _opheadid);
    q.exec();
    if (q.first())
    {
      int result = q.value("result").toInt();
      if (result < 0)
      {
	systemError(this, storedProcErrorLookup("deleteOpportunity", result));
	return;
      }
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  reject();
}

void opportunity::sSave()
{
  if (! save(false)) // if error
    return;

  done(_opheadid);
}

bool opportunity::save(bool partial)
{
  if (! partial)
  {
    if(_crmacct->id() == -1)
    {
      QMessageBox::critical( this, tr("Incomplete Information"),
	tr("You must specify the Account that this opportunity is for.") );
      return false;
    }

    if(_name->text().trimmed().isEmpty())
    {
      QMessageBox::critical( this, tr("Incomplete Information"),
	tr("You must specify a Name for this opportunity report.") );
      _name->setFocus();
      return false;
    }
  }

  XSqlQuery rollback;
  rollback.prepare("ROLLBACK;");

  if (!q.exec("BEGIN"))
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return false;
  }

  if (cNew == _mode && !_saved)
    q.prepare("INSERT INTO ophead"
              "      (ophead_name, ophead_crmacct_id,"
              "       ophead_owner_username,"
              "       ophead_opstage_id, ophead_opsource_id,"
              "       ophead_optype_id, ophead_probability_prcnt,"
              "       ophead_amount, ophead_curr_id, ophead_target_date,"
              "       ophead_actual_date,"
              "       ophead_notes, ophead_active) "
              "VALUES(:ophead_name, :ophead_crmacct_id,"
              "       :ophead_owner_username,"
              "       :ophead_opstage_id, :ophead_opsource_id,"
              "       :ophead_optype_id, :ophead_probability_prcnt,"
              "       :ophead_amount, :ophead_curr_id, :ophead_target_date,"
              "       :ophead_actual_date,"
              "       :ophead_notes, :ophead_active);" );
  else if (cEdit == _mode || _saved)
    q.prepare("UPDATE ophead"
              "   SET ophead_name=:ophead_name,"
              "       ophead_crmacct_id=:ophead_crmacct_id,"
              "       ophead_owner_username=:ophead_owner_username,"
              "       ophead_opstage_id=:ophead_opstage_id,"
              "       ophead_opsource_id=:ophead_opsource_id,"
              "       ophead_optype_id=:ophead_optype_id,"
              "       ophead_probability_prcnt=:ophead_probability_prcnt,"
              "       ophead_amount=:ophead_amount,"
              "       ophead_curr_id=:ophead_curr_id,"
              "       ophead_target_date=:ophead_target_date,"
              "       ophead_actual_date=:ophead_actual_date,"
              "       ophead_notes=:ophead_notes,"
              "       ophead_active=:ophead_active"
              " WHERE (ophead_id=:ophead_id); ");

  q.bindValue(":ophead_id", _opheadid);
  q.bindValue(":ophead_name", _name->text());
  q.bindValue(":ophead_active",	QVariant(_active->isChecked()));
  if (_crmacct->id() > 0)
    q.bindValue(":ophead_crmacct_id", _crmacct->id());
  q.bindValue(":ophead_owner_username", _owner->username());
  if(_oppstage->isValid())
    q.bindValue(":ophead_opstage_id", _oppstage->id());
  if(_oppsource->isValid())
    q.bindValue(":ophead_opsource_id", _oppsource->id());
  if(_opptype->isValid())
    q.bindValue(":ophead_optype_id", _opptype->id());
  if(!_probability->text().isEmpty())
    q.bindValue(":ophead_probability_prcnt", _probability->text().toInt());
  if(!_amount->isZero())
  {
    q.bindValue(":ophead_amount", _amount->localValue());
    q.bindValue(":ophead_curr_id", _amount->id());
  }
  if(_targetDate->isValid())
    q.bindValue(":ophead_target_date", _targetDate->date());
  if(_actualDate->isValid())
    q.bindValue(":ophead_actual_date", _actualDate->date());
  q.bindValue(":ophead_notes", _notes->toPlainText());

  if(!q.exec() && q.lastError().type() != QSqlError::NoError)
  {
    rollback.exec();
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return false;
  }

  q.exec("COMMIT;");
  if(q.lastError().type() != QSqlError::NoError)
  {
    rollback.exec();
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return false;
  }

  if (cNew == _mode && ! _saved)
  {
    q.exec("SELECT CURRVAL('ophead_ophead_id_seq') AS result;");
    if (q.first())
      _opheadid = q.value("result").toInt();
    else if(q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return false;
    }
  }

  _saved = true;
  return true;
}

void opportunity::populate()
{ 
  q.prepare("SELECT ophead_name,"
            "       ophead_crmacct_id,"
            "       ophead_owner_username,"
            "       ophead_opstage_id, ophead_opsource_id,"
            "       ophead_optype_id,"
            "       ophead_probability_prcnt, ophead_amount,"
            "       COALESCE(ophead_curr_id, basecurrid()) AS curr_id,"
            "       ophead_target_date, ophead_actual_date,"
            "       ophead_notes, ophead_active"
            "  FROM ophead"
            " WHERE(ophead_id=:ophead_id); ");
  q.bindValue(":ophead_id", _opheadid);
  q.exec();
  if(q.first())
  {
    _name->setText(q.value("ophead_name").toString());
    _active->setChecked(q.value("ophead_active").toBool());
    _crmacct->setId(q.value("ophead_crmacct_id").toInt());
    _owner->setUsername(q.value("ophead_owner_username").toString());

    _oppstage->setNull();
    if(!q.value("ophead_opstage_id").toString().isEmpty())
      _oppstage->setId(q.value("ophead_opstage_id").toInt());

    _oppsource->setNull();
    if(!q.value("ophead_opsource_id").toString().isEmpty())
      _oppsource->setId(q.value("ophead_opsource_id").toInt());

    _opptype->setNull();
    if(!q.value("ophead_optype_id").toString().isEmpty())
      _opptype->setId(q.value("ophead_optype_id").toInt());

    _probability->clear();
    if(!q.value("ophead_probability_prcnt").toString().isEmpty())
      _probability->setText(q.value("ophead_probability_prcnt").toDouble());

    _amount->clear();
    _amount->setId(q.value("curr_id").toInt());
    if(!q.value("ophead_amount").toString().isEmpty())
      _amount->setLocalValue(q.value("ophead_amount").toDouble());

    _targetDate->clear();
    if(!q.value("ophead_target_date").toString().isEmpty())
      _targetDate->setDate(q.value("ophead_target_date").toDate());

    _actualDate->clear();
    if(!q.value("ophead_actual_date").toString().isEmpty())
      _actualDate->setDate(q.value("ophead_actual_date").toDate());

    _notes->setText(q.value("ophead_notes").toString());

    _comments->setId(_opheadid);

    sFillTodoList();
    sFillSalesList();
    sFillCharList();
  }
}

void opportunity::sNewTodoItem()
{
  if (! save(true))
    return;

  ParameterList params;
  params.append("mode", "new");
  params.append("ophead_id", _opheadid);
  if (_crmacct->isValid())
    params.append("crmacct_id", _crmacct->id());

  todoItem newdlg(this, 0, true);
  newdlg.set(params);
  if (newdlg.exec() == XDialog::Accepted)
    sFillTodoList();
}

void opportunity::sEditTodoItem()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("todoitem_id", _todoList->id());

  todoItem newdlg(this, 0, true);
  newdlg.set(params);
  if (newdlg.exec() == XDialog::Accepted)
    sFillTodoList();
}

void opportunity::sViewTodoItem()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("todoitem_id", _todoList->id());

  todoItem newdlg(this, 0, true);
  newdlg.set(params);
  newdlg.exec();
}

void opportunity::sDeleteTodoItem()
{
  q.prepare("SELECT deleteTodoItem(:todoitem_id) AS result;");
  q.bindValue(":todoitem_id", _todoList->id());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("deleteTodoItem", result));
      return;
    }
    else
      sFillTodoList();
    }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void opportunity::sFillTodoList()
{
  q.prepare("SELECT todoitem_id, incdtpriority_name, incdtpriority_order, "
	    "       todoitem_username, todoitem_name, "
	    "       firstLine(todoitem_description) AS todoitem_description, "
	    "       todoitem_status, todoitem_due_date, "
            "       CASE "
            "         WHEN (todoitem_status != 'C' AND todoitem_due_date < current_date) THEN "
            "           'error' "
            "         WHEN (todoitem_status != 'C' AND todoitem_due_date > current_date) THEN "
            "           'altemphasis' "
            "       END AS qtforegroundrole "
	    "  FROM todoitem "
            "       LEFT OUTER JOIN incdtpriority ON (incdtpriority_id=todoitem_priority_id) "
	    "WHERE ( (todoitem_ophead_id=:ophead_id) "
	    "  AND   (todoitem_active) ) "
	    "ORDER BY incdtpriority_order, todoitem_username;");

  q.bindValue(":ophead_id", _opheadid);
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  _todoList->populate(q);
}

void opportunity::sPopulateTodoMenu(QMenu *pMenu)
{
  int menuItem;

  bool newPriv = (cNew == _mode || cEdit == _mode) &&
      (_privileges->check("MaintainPersonalTodoList") ||
       _privileges->check("MaintainOtherTodoLists") );

  bool editPriv = (cNew == _mode || cEdit == _mode) && (
      (omfgThis->username() == _todoList->currentItem()->text("todoitem_username") && _privileges->check("MaintainPersonalTodoList")) ||
      (omfgThis->username() != _todoList->currentItem()->text("todoitem_username") && _privileges->check("MaintainOtherTodoLists")) );

  bool viewPriv =
      (omfgThis->username() == _todoList->currentItem()->text("todoitem_username") && _privileges->check("ViewPersonalTodoList")) ||
      (omfgThis->username() != _todoList->currentItem()->text("todoitem_username") && _privileges->check("ViewOtherTodoLists"));

  menuItem = pMenu->insertItem(tr("New..."), this, SLOT(sNewTodoItem()), 0);
  pMenu->setItemEnabled(menuItem, newPriv);

  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEditTodoItem()), 0);
  pMenu->setItemEnabled(menuItem, editPriv);

  menuItem = pMenu->insertItem(tr("View..."), this, SLOT(sViewTodoItem()), 0);
  pMenu->setItemEnabled(menuItem, viewPriv);

  menuItem = pMenu->insertItem(tr("Delete"), this, SLOT(sDeleteTodoItem()), 0);
  pMenu->setItemEnabled(menuItem, editPriv);
}

void opportunity::sHandleTodoPrivs()
{
  bool newPriv = (cNew == _mode || cEdit == _mode) &&
      (_privileges->check("MaintainPersonalTodoList") ||
       _privileges->check("MaintainOtherTodoLists") );

  bool editPriv = false;
  bool viewPriv = false;

  if(_todoList->currentItem())
  {
    editPriv = (cNew == _mode || cEdit == _mode) && (
      (omfgThis->username() == _todoList->currentItem()->text("todoitem_username") && _privileges->check("MaintainPersonalTodoList")) ||
      (omfgThis->username() != _todoList->currentItem()->text("todoitem_username") && _privileges->check("MaintainOtherTodoLists")) );

    viewPriv =
      (omfgThis->username() == _todoList->currentItem()->text("todoitem_username") && _privileges->check("ViewPersonalTodoList")) ||
      (omfgThis->username() != _todoList->currentItem()->text("todoitem_username") && _privileges->check("ViewOtherTodoLists"));
  }

  _newTodoItem->setEnabled(newPriv);
  _editTodoItem->setEnabled(editPriv && _todoList->id() > 0);
  _viewTodoItem->setEnabled((editPriv || viewPriv) && _todoList->id() > 0);
  _deleteTodoItem->setEnabled(editPriv && _todoList->id() > 0);

  if (editPriv)
  {
    disconnect(_todoList,SIGNAL(itemSelected(int)),_viewTodoItem, SLOT(animateClick()));
    connect(_todoList,	SIGNAL(itemSelected(int)), _editTodoItem, SLOT(animateClick()));
  }
  else if (viewPriv)
  {
    disconnect(_todoList,SIGNAL(itemSelected(int)),_editTodoItem, SLOT(animateClick()));
    connect(_todoList,	SIGNAL(itemSelected(int)), _viewTodoItem, SLOT(animateClick()));
  }
}

void opportunity::sPrintSale()
{
  if (_salesList->altId() == 0)
    sPrintQuote();
  else
    sPrintSalesOrder();
}

void opportunity::sEditSale()
{
  if (_salesList->altId() == 0)
    sEditQuote();
  else
    sEditSalesOrder();
}

void opportunity::sViewSale()
{
  if (_salesList->altId() == 0)
    sViewQuote();
  else
    sViewSalesOrder();
}

void opportunity::sDeleteSale()
{
  if (_salesList->altId() == 0)
    sDeleteQuote();
  else
    sDeleteSalesOrder();
}

void opportunity::sPrintQuote()
{
  q.prepare( "SELECT findCustomerForm(quhead_cust_id, 'Q') AS report_name "
             "FROM quhead "
             "WHERE (quhead_id=:quheadid); " );
  q.bindValue(":quheadid", _salesList->id());
  q.exec();
  if (q.first())
  {
    ParameterList params;
    params.append("quhead_id", _salesList->id());

    orReport report(q.value("report_name").toString(), params);
    if (report.isValid())
      report.print();
    else
      report.reportError(this);
  }
  else
    QMessageBox::warning( this, tr("Could not locate report"),
                          tr("Could not locate the report definition the form \"%1\"")
                          .arg(q.value("report_name").toString()) );
}

void opportunity::sConvertQuote()
{
  if (QMessageBox::question(this, tr("Convert Selected Quote"),
			    tr("<p>Are you sure that you want to convert "
			       "the selected Quote to a Sales Order?"),
			    QMessageBox::Yes,
			    QMessageBox::No | QMessageBox::Default) == QMessageBox::Yes)
  {
    int quheadid = _salesList->id();
    XSqlQuery convert;
    convert.prepare("SELECT convertQuote(:quhead_id) AS sohead_id;");

    XSqlQuery prospectq;
    prospectq.prepare("SELECT convertProspectToCustomer(quhead_cust_id) AS result "
                      "FROM quhead "
                      "WHERE (quhead_id=:quhead_id);");

    bool tryagain = false;
    do
	{
	  int soheadid = -1;
      convert.bindValue(":quhead_id", quheadid);
      convert.exec();
      if (convert.first())
      {
        soheadid = convert.value("sohead_id").toInt();
        if (soheadid == -3)
        {
          if ((_metrics->value("DefaultSalesRep").toInt() > 0) &&
              (_metrics->value("DefaultTerms").toInt() > 0) &&
			  (_metrics->value("DefaultCustType").toInt() > 0) && 
              (_metrics->value("DefaultShipFormId").toInt() > 0)  && 
              (_privileges->check("MaintainCustomerMasters"))) 
          {
            if (QMessageBox::question(this, tr("Quote for Prospect"),
											tr("<p>This Quote is for a Prospect, not "
                                               "a Customer. Do you want to convert "
                                               "the Prospect to a Customer using global "
                                               "default values?"),
                                      QMessageBox::Yes,
                                      QMessageBox::No | QMessageBox::Default) == QMessageBox::Yes)
            {
              prospectq.bindValue(":quhead_id", quheadid);
              prospectq.exec();
              if (prospectq.first())
              {
                int result = prospectq.value("result").toInt();
                if (result < 0)
                {
                  systemError(this, storedProcErrorLookup("convertProspectToCustomer", result), __FILE__, __LINE__);
				  continue;
                }
                convert.exec();
                if (convert.first())
                {
                  soheadid = convert.value("sohead_id").toInt();
                  if (soheadid < 0)
                  {
                    QMessageBox::warning(this, tr("Cannot Convert Quote"), storedProcErrorLookup("convertQuote", soheadid)
                                        .arg(quheadid));
					continue;
                  }
				}
			  }
              else if (prospectq.lastError().type() != QSqlError::NoError)
              {
                systemError(this, prospectq.lastError().databaseText(), __FILE__, __LINE__);
                continue;
			  }
			}
            else
            {
              QMessageBox::information(this, tr("Quote for Prospect"),
                                             tr("<p>The prospect must be manually "
												"converted to customer from either the "
                                                "CRM Account or Customer windows before "
                                                "coverting this quote."));
              continue;
			}
		  }
          else
          {
            QMessageBox::information(this, tr("Quote for Prospect"),
                                           tr("<p>The prospect must be manually "
                                              "converted to customer from either the "
                                              "CRM Account or Customer windows before "
                                              "coverting this quote."));
            continue;
		  }
		}
        else if (soheadid < 0)
        {
          QMessageBox::warning(this, tr("Cannot Convert Quote"), storedProcErrorLookup("convertQuote", soheadid)
							   .arg(quheadid));
          continue;
        }
        ParameterList params;
        params.append("mode", "edit");
        params.append("sohead_id", soheadid);
    
        salesOrder *newdlg = new salesOrder();
        newdlg->setWindowModality(Qt::WindowModal);
        newdlg->set(params);
        omfgThis->handleNewWindow(newdlg);
	  }
      else if (convert.lastError().type() != QSqlError::NoError)
      {
        systemError(this, convert.lastError().databaseText(), __FILE__, __LINE__);
        continue;
	  }
    } while (tryagain);
  } // if user wants to convert
}

void opportunity::sNewQuote()
{
  if (! save(false))
    return;

  ParameterList params;
  params.append("mode", "newQuote");
  if (_custid > -1)
    params.append("cust_id", _custid);
  else
    params.append("cust_id", _prospectid);
  params.append("ophead_id", _opheadid);

  salesOrder *newdlg = new salesOrder();
  newdlg->setWindowModality(Qt::WindowModal);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
  sFillSalesList();
}

void opportunity::sAttachQuote()
{
  ParameterList params;
  params.append("cust_id", _custid);
  
  quoteList newdlg(this, "", TRUE);
  newdlg.set(params);

  int id = newdlg.exec();
  if(id != QDialog::Rejected)
  {
    q.prepare("SELECT attachQuoteToOpportunity(:quhead_id, :ophead_id) AS result;");
    q.bindValue(":quhead_id", id);
    q.bindValue(":ophead_id", _opheadid);
    q.exec();
    if (q.first())
    {
      int result = q.value("result").toInt();
      if (result < 0)
      {
        systemError(this, storedProcErrorLookup("attachQuoteToOpportunity", result),
                    __FILE__, __LINE__);
        return;
      }
      else
        sFillSalesList();
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void opportunity::sEditQuote()
{
  ParameterList params;
  params.append("mode", "editQuote");
  params.append("quhead_id", _salesList->id());
    
  salesOrder *newdlg = new salesOrder();
  newdlg->setWindowModality(Qt::WindowModal);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
  sFillSalesList();
}

void opportunity::sViewQuote()
{
  ParameterList params;
  params.append("mode", "viewQuote");
  params.append("quhead_id", _salesList->id());
    
  salesOrder *newdlg = new salesOrder();
  newdlg->setWindowModality(Qt::WindowModal);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void opportunity::sDeleteQuote()
{
  if ( QMessageBox::warning( this, tr("Delete Selected Quote"),
                             tr("Are you sure that you want to delete the selected Quote?" ),
                             tr("&Yes"), tr("&No"), QString::null, 0, 1 ) == 0)
  {
    q.prepare("SELECT deleteQuote(:quhead_id) AS result;");
    q.bindValue(":quhead_id", _salesList->id());
    q.exec();
    if (q.first())
    {
      int result = q.value("result").toInt();
      if (result < 0)
      {
        systemError(this, storedProcErrorLookup("deleteQuote", result),
                    __FILE__, __LINE__);
        return;
      }
      else
        sFillSalesList();
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void opportunity::sPrintSalesOrder()
{
  ParameterList params;
  params.append("sohead_id", _salesList->id());

  printSoForm newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void opportunity::sNewSalesOrder()
{
  if (! save(false))
    return;

  ParameterList params;
  params.append("mode", "new");
  params.append("cust_id", _custid);
  params.append("ophead_id", _opheadid);

  salesOrder *newdlg = new salesOrder();
  newdlg->setWindowModality(Qt::WindowModal);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
  sFillSalesList();
}

void opportunity::sAttachSalesOrder()
{
  ParameterList params;
  params.append("soType", (cSoOpen | cSoCustomer));
  params.append("cust_id", _custid);
  
  salesOrderList newdlg(this, "", TRUE);
  newdlg.set(params);

  int id = newdlg.exec();
  if(id != QDialog::Rejected)
  {
    q.prepare("SELECT attachSalesOrderToOpportunity(:cohead_id, :ophead_id) AS result;");
    q.bindValue(":cohead_id", id);
    q.bindValue(":ophead_id", _opheadid);
    q.exec();
    if (q.first())
    {
      int result = q.value("result").toInt();
      if (result < 0)
      {
        systemError(this, storedProcErrorLookup("attachSalesOrderToOpportunity", result),
                    __FILE__, __LINE__);
        return;
      }
      else
        sFillSalesList();
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void opportunity::sEditSalesOrder()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("sohead_id", _salesList->id());
    
  salesOrder *newdlg = new salesOrder();
  newdlg->setWindowModality(Qt::WindowModal);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
  sFillSalesList();
}

void opportunity::sViewSalesOrder()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("sohead_id", _salesList->id());
    
  salesOrder *newdlg = new salesOrder();
  newdlg->setWindowModality(Qt::WindowModal);
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void opportunity::sDeleteSalesOrder()
{
  if ( QMessageBox::warning( this, tr("Delete Selected Sales Order"),
                             tr("Are you sure that you want to delete the selected Sales Order?" ),
                             tr("&Yes"), tr("&No"), QString::null, 0, 1 ) == 0)
  {
    q.prepare("SELECT deleteSo(:cohead_id) AS result;");
    q.bindValue(":cohead_id", _salesList->id());
    q.exec();
    if (q.first())
    {
      int result = q.value("result").toInt();
      if (result < 0)
      {
        systemError(this, storedProcErrorLookup("deleteSo", result),
                    __FILE__, __LINE__);
        return;
      }
      else
        sFillSalesList();
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void opportunity::sFillSalesList()
{
  q.prepare("SELECT id, alt_id, sale_type, sale_number, sale_date, sale_extprice, "
            "       'curr' AS sale_extprice_xtnumericrole, "
			"       0 AS sale_extprice_xttotalrole "
            "FROM ( "
            "SELECT quhead_id AS id, 0 AS alt_id, :quote AS sale_type, "
			"       quhead_number::TEXT AS sale_number, quhead_quotedate AS sale_date, "
            "       calcQuoteAmt(quhead_id) AS sale_extprice "
            "FROM quhead "
            "WHERE (quhead_ophead_id=:ophead_id) "
			"UNION "
			"SELECT cohead_id AS id, 1 AS alt_id, :salesorder AS sale_type, "
			"       cohead_number AS sale_number, cohead_orderdate AS sale_date,  "
            "       calcSalesOrderAmt(cohead_id) AS sale_extprice "
            "FROM cohead "
            "WHERE (cohead_ophead_id=:ophead_id) "
			"     ) AS data "
            "ORDER BY sale_date;");
  q.bindValue(":ophead_id", _opheadid);
  q.bindValue(":quote", tr("Quote"));
  q.bindValue(":salesorder", tr("Sales Order"));
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  _salesList->populate(q, true);
}

void opportunity::sPopulateSalesMenu(QMenu *pMenu)
{
  bool editPriv = false;
  bool viewPriv = false;
  bool convertPriv = false;

  if(_salesList->currentItem())
  {
    editPriv = (cNew == _mode || cEdit == _mode) && (
      (0 == _salesList->currentItem()->altId() && _privileges->check("MaintainQuotes")) ||
      (1 == _salesList->currentItem()->altId() && _privileges->check("MaintainSalesOrders")) );

    viewPriv = (cNew == _mode || cEdit == _mode) && (
      (0 == _salesList->currentItem()->altId() && _privileges->check("ViewQuotes")) ||
      (1 == _salesList->currentItem()->altId() && _privileges->check("ViewSalesOrders")) );

    convertPriv = (cNew == _mode || cEdit == _mode) &&
      (0 == _salesList->currentItem()->altId() && _privileges->check("ConvertQuotes"));
  }

  int menuItem;
  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEditSale()), 0);
  pMenu->setItemEnabled(menuItem, editPriv);

  menuItem = pMenu->insertItem(tr("View..."), this, SLOT(sViewSale()), 0);
  pMenu->setItemEnabled(menuItem, (editPriv || viewPriv));

  menuItem = pMenu->insertItem(tr("Delete..."), this, SLOT(sDeleteSale()), 0);
  pMenu->setItemEnabled(menuItem, editPriv);

  menuItem = pMenu->insertItem(tr("Print..."), this, SLOT(sPrintSale()), 0);
  pMenu->setItemEnabled(menuItem, (editPriv || viewPriv));
}

void opportunity::sHandleSalesPrivs()
{
  bool isCustomer = (_custid > -1);
  
  bool newQuotePriv = ((cNew == _mode || cEdit == _mode) && _privileges->check("MaintainQuotes"));
  bool newSalesOrderPriv = ((cNew == _mode || cEdit == _mode) && isCustomer && _privileges->check("MaintainSalesOrders"));

  bool editPriv = false;
  bool viewPriv = false;
  bool convertPriv = false;

  int newMenuItem;
  QMenu * newMenu = new QMenu;
  newMenuItem = newMenu->insertItem(tr("Quote"), this, SLOT(sNewQuote()));
  newMenu->setItemEnabled(newMenuItem, newQuotePriv);
  newMenuItem = newMenu->insertItem(tr("Sales Order"), this, SLOT(sNewSalesOrder()));
  newMenu->setItemEnabled(newMenuItem, newSalesOrderPriv);
  _newSale->setMenu(newMenu);

  int attachMenuItem;
  QMenu * attachMenu = new QMenu;
  attachMenuItem = attachMenu->insertItem(tr("Quote"), this, SLOT(sAttachQuote()));
  attachMenu->setItemEnabled(attachMenuItem, newQuotePriv);
  attachMenuItem = attachMenu->insertItem(tr("Sales Order"), this, SLOT(sAttachSalesOrder()));
  attachMenu->setItemEnabled(attachMenuItem, newSalesOrderPriv);
  _attachSale->setMenu(attachMenu);

  if(_salesList->currentItem())
  {
    editPriv = (cNew == _mode || cEdit == _mode) && (
      (0 == _salesList->currentItem()->altId() && _privileges->check("MaintainQuotes")) ||
      (1 == _salesList->currentItem()->altId() && _privileges->check("MaintainSalesOrders")) );

    viewPriv = (cNew == _mode || cEdit == _mode) && (
      (0 == _salesList->currentItem()->altId() && _privileges->check("ViewQuotes")) ||
      (1 == _salesList->currentItem()->altId() && _privileges->check("ViewSalesOrders")) );

    convertPriv = (cNew == _mode || cEdit == _mode) &&
      (0 == _salesList->currentItem()->altId() && _privileges->check("ConvertQuotes"));
  }

  _convertQuote->setEnabled(convertPriv && _salesList->id() > 0);
  _editSale->setEnabled(editPriv && _salesList->id() > 0);
  _viewSale->setEnabled((editPriv || viewPriv) && _salesList->id() > 0);
  _printSale->setEnabled((editPriv || viewPriv) && _salesList->id() > 0);
  _deleteSale->setEnabled(editPriv && _salesList->id() > 0);

  if (editPriv)
  {
    disconnect(_salesList,SIGNAL(itemSelected(int)),_viewSale, SLOT(animateClick()));
    connect(_salesList,	SIGNAL(itemSelected(int)), _editSale, SLOT(animateClick()));
  }
  else if (viewPriv)
  {
    disconnect(_salesList,SIGNAL(itemSelected(int)),_editSale, SLOT(animateClick()));
    connect(_salesList,	SIGNAL(itemSelected(int)), _viewSale, SLOT(animateClick()));
  }
}

void opportunity::sNewCharacteristic()
{
  if (! save(true))
    return;

  ParameterList params;
  params.append("mode", "new");
  params.append("ophead_id", _opheadid);

  characteristicAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillCharList();
}

void opportunity::sEditCharacteristic()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("charass_id", _charass->id());

  characteristicAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillCharList();
}

void opportunity::sDeleteCharacteristic()
{
  q.prepare( "DELETE FROM charass "
             "WHERE (charass_id=:charass_id);" );
  q.bindValue(":charass_id", _charass->id());
  q.exec();

  sFillCharList();
}

void opportunity::sFillCharList()
{
  q.prepare( "SELECT charass_id, char_name, charass_value, charass_default "
             "FROM charass, char "
             "WHERE ( (charass_target_type='OPP')"
             " AND (charass_char_id=char_id)"
             " AND (charass_target_id=:ophead_id) ) "
             "ORDER BY char_name;" );
  q.bindValue(":ophead_id", _opheadid);
  q.exec();
  _charass->populate(q);
}

void opportunity::sHandleCrmacct(int pCrmacctid)
{
  XSqlQuery crmacct;
  crmacct.prepare( "SELECT COALESCE(crmacct_cust_id, -1) AS cust_id, "
                   "       COALESCE(crmacct_prospect_id, -1) AS prospect_id "
                   "FROM crmacct "
                   "WHERE (crmacct_id=:crmacct_id);" );
  crmacct.bindValue(":crmacct_id", pCrmacctid);
  crmacct.exec();
  if (crmacct.first())
  {
    _custid = crmacct.value("cust_id").toInt();
	_prospectid = crmacct.value("prospect_id").toInt();
  }
  if (_custid == -1 && _prospectid == -1)
    _salesTab->setEnabled(false);
  else
    _salesTab->setEnabled(true);
}

