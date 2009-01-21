/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "forwardUpdateAccounts.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include "guiclient.h"

/*
 *  Constructs a forwardUpdateAccounts as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
forwardUpdateAccounts::forwardUpdateAccounts(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_close, SIGNAL(clicked()), this, SLOT(accept()));
    connect(_typeSelected, SIGNAL(toggled(bool)), _type, SLOT(setEnabled(bool)));
    connect(_update, SIGNAL(clicked()), this, SLOT(sUpdate()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
forwardUpdateAccounts::~forwardUpdateAccounts()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void forwardUpdateAccounts::languageChange()
{
    retranslateUi(this);
}


void forwardUpdateAccounts::sUpdate()
{
  if(_accntSelected->isChecked() && _accnt->id() == -1)
  {
    QMessageBox::warning(this, tr("No Account Selected"),
      tr("You have choosen to use an account but no account was specified.") );
    return;
  }
  
  if(_accntSelected->isChecked())
  {
    q.prepare("SELECT forwardUpdateAccount(:accnt_id) AS result;");
    q.bindValue(":accnt_id", _accnt->id());
  }
  else if(_typeSelected->isChecked())
  {
    q.prepare("SELECT forwardUpdateAccount(accnt_id) AS result"
              "  FROM accnt JOIN company ON ( (company_number=accnt_company) AND (NOT company_external) )"
              " WHERE (accnt_type=:accnt_type);");
    if (_type->currentIndex() == 0)
      q.bindValue(":accnt_type", "A");
    else if (_type->currentIndex() == 1)
      q.bindValue(":accnt_type", "L");
    else if (_type->currentIndex() == 2)
      q.bindValue(":accnt_type", "E");
    else if (_type->currentIndex() == 3)
      q.bindValue(":accnt_type", "R");
    else if (_type->currentIndex() == 4)
      q.bindValue(":accnt_type", "Q");
  }
  else
    q.prepare("SELECT forwardUpdateAccount(accnt_id) AS result"
              "  FROM accnt JOIN company ON ( (company_number=accnt_company) AND (NOT company_external) );");
  
  q.exec();
}

