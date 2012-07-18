/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2012 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "currencies.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QSqlError>

#include <parameter.h>

#include "currency.h"
#include "errorReporter.h"

currencies::currencies(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_new,    SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit,   SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_view,   SIGNAL(clicked()), this, SLOT(sView()));
  connect(_curr, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));

  if (_privileges->check("MaintainCurrencies"))
  {
    connect(_curr, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_curr, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_curr, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_curr, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }
    
  _curr->addColumn(tr("Base"),		 _ynColumn,       Qt::AlignCenter, true,  "curr_base");
  _curr->addColumn(tr("Name"),		 -1,              Qt::AlignLeft,   true,  "curr_name");
  _curr->addColumn(tr("Symbol"),	 _currencyColumn, Qt::AlignCenter, true,  "curr_symbol");
  _curr->addColumn(tr("Abbreviation"),	 _currencyColumn, Qt::AlignLeft,   true,  "curr_abbr");
  _curr->addColumn(tr("ISO Number"),     _currencyColumn, Qt::AlignLeft,   false, "curr_number");
  _curr->addColumn(tr("Decimal Places"), _currencyColumn, Qt::AlignRight,  false, "curr_scale");
  _curr->addColumn(tr("Active"),         _ynColumn,       Qt::AlignCenter, false, "curr_active");
    
  sFillList();
}

currencies::~currencies()
{
  // no need to delete child widgets, Qt does it all for us
}

void currencies::languageChange()
{
  retranslateUi(this);
}

void currencies::sNew()
{
  XSqlQuery currenciesNew;
  bool single = omfgThis->singleCurrency();

  ParameterList params;
  params.append("mode", "new");
    
  currency *newdlg = new currency(this, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
  sFillList();

  if(single && !omfgThis->singleCurrency() &&
     _metrics->value("GLCompanySize").toInt() == 0)
  {
    // Check for the gain/loss and discrep accounts
    currenciesNew.prepare("SELECT COUNT(*) = 2 AS result"
              "  FROM accnt "
              " WHERE accnt_id IN (fetchMetricValue('CurrencyGainLossAccount'),"
              "                    fetchMetricValue('GLSeriesDiscrepancyAccount'));");
    currenciesNew.exec();
    if(currenciesNew.first() && currenciesNew.value("result").toBool() != true)
    {
      QMessageBox::warning( this, tr("Additional Configuration Required"),
        tr("<p>Your system is configured to use multiple Currencies, but the "
           "Currency Gain/Loss Account and/or the G/L Series Discrepancy Account "
           "does not appear to be configured correctly. You should define these "
           "Accounts in 'System | Configure Modules | Configure G/L...' before "
           "posting any transactions in the system.") );
    }
    else if (ErrorReporter::error(QtCriticalMsg, this, tr("Getting Accounts"),
                                  currenciesNew, __FILE__, __LINE__))
      return;
  }
}

void currencies::sEdit()
{
  ParameterList params;
  params.append("curr_id", _curr->id());
  params.append("mode", "edit");
    
  currency *newdlg = new currency(this, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
  sFillList();
}

void currencies::sView()
{
  ParameterList params;
  params.append("curr_id", _curr->id());
  params.append("mode", "view");
    
  currency *newdlg = new currency(this, "", TRUE);
  newdlg->set(params);
  newdlg->exec();
}

void currencies::sDelete()
{
  XSqlQuery delq;
  delq.prepare("SELECT curr_base FROM curr_symbol "
               "WHERE curr_id = :curr_id");
  delq.bindValue(":curr_id", _curr->id());
  delq.exec();
  if (delq.first() && delq.value("curr_base").toBool())
  {
    QMessageBox::critical(this,
                          tr("Cannot delete base currency"),
                          tr("You cannot delete the base currency."));
    return;
  }
  else if (ErrorReporter::error(QtCriticalMsg, this, tr("Getting Base Currency"),
                                delq, __FILE__, __LINE__))
    return;

  if (QMessageBox::question(this, tr("Delete?"),
                            tr("<p>Are you sure you want to delete this currency?"),
                            QMessageBox::No | QMessageBox::Default,
                            QMessageBox::Yes) == QMessageBox::No)
    return;
  
  delq.prepare("DELETE FROM curr_symbol WHERE curr_id = :curr_id");
  delq.bindValue(":curr_id", _curr->id());
  delq.exec();
  if (ErrorReporter::error(QtCriticalMsg, this, tr("Getting Base Currency"),
                           delq, __FILE__, __LINE__))
    return;
  
  sFillList();
}

void currencies::sFillList()
{
  XSqlQuery currenciesFillList;
  currenciesFillList.prepare("SELECT *"
                             "  FROM curr_symbol"
                             " ORDER BY curr_base DESC, curr_name;");
  currenciesFillList.exec();
  _curr->populate(currenciesFillList);
  if (ErrorReporter::error(QtCriticalMsg, this, tr("Getting Currencies"),
                           currenciesFillList, __FILE__, __LINE__))
    return;
}

void currencies::sPopulateMenu(QMenu* pMenu)
{
  QAction *menuItem;
  
  menuItem = pMenu->addAction(tr("View"));
  connect(menuItem, SIGNAL(triggered()), this, SLOT(sView()));
  
  menuItem = pMenu->addAction(tr("Edit"));
  connect(menuItem, SIGNAL(triggered()), this, SLOT(sEdit()));
  menuItem->setEnabled(_privileges->check("MaintainCurrencies"));
  
  menuItem = pMenu->addAction(tr("Delete..."));
  connect(menuItem, SIGNAL(triggered()), this, SLOT(sDelete()));
  menuItem->setEnabled(_privileges->check("MaintainCurrencies"));
}
