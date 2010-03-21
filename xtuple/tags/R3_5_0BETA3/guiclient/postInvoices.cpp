/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "postInvoices.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>

#include "submitAction.h"

postInvoices::postInvoices(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_post, SIGNAL(clicked()), this, SLOT(sPost()));
  connect(_submit, SIGNAL(clicked()), this, SLOT(sSubmit()));

  if (!_metrics->boolean("EnableBatchManager"))
    _submit->hide();

  if (_preferences->boolean("XCheckBox/forgetful"))
    _printJournal->setChecked(true);

  _post->setFocus();
}

postInvoices::~postInvoices()
{
  // no need to delete child widgets, Qt does it all for us
}

void postInvoices::languageChange()
{
  retranslateUi(this);
}

void postInvoices::sPost()
{
  q.exec( "SELECT invchead_printed, COUNT(*) AS number "
          "FROM ( "
          "  SELECT * FROM invchead WHERE NOT (invchead_posted)) AS data "
          "WHERE (checkInvoiceSitePrivs(invchead_id)) "
          "GROUP BY invchead_printed;" );
  if (q.first())
  {
    int printed   = 0;
    int unprinted = 0;

    do
    {
      if (q.value("invchead_printed").toBool())
        printed = q.value("number").toInt();
      else
        unprinted = q.value("number").toInt();
    }
    while (q.next());

    if ( ( (unprinted) && (!printed) ) && (!_postUnprinted->isChecked()) )
    {
      QMessageBox::warning( this, tr("No Invoices to Post"),
                            tr( "Although there are unposted Invoices, there are no unposted Invoices that have been printed.\n"
                                "You must manually print these Invoices or select 'Post Unprinted Invoices' before these Invoices\n"
                                "may be posted." ) );
      _postUnprinted->setFocus();
      return;
    }
  }
  else
  {
    QMessageBox::warning( this, tr("No Invoices to Post"),
                          tr("There are no Invoices, printed or not, to post.\n" ) );
    _close->setFocus();
    return;
  }

  bool inclZero = false;
  q.exec("SELECT COUNT(invchead_id) AS numZeroInvcs "
	 "FROM ( SELECT invchead_id "
	 "         FROM invchead LEFT OUTER JOIN"
         "              invcitem ON (invcitem_invchead_id=invchead_id) LEFT OUTER JOIN"
	 "              item ON (invcitem_item_id=item_id)  "
	 "        WHERE (NOT invchead_posted) "
	 "        GROUP BY invchead_id, invchead_freight, invchead_tax, invchead_misc_amount "
	 "       HAVING (COALESCE(SUM(round((invcitem_billed * invcitem_qty_invuomratio) * (invcitem_price /  "
	 "     	     CASE WHEN (item_id IS NULL) THEN 1 "
	 "     	     ELSE invcitem_price_invuomratio END), 2)),0) + invchead_freight + invchead_tax + "
         "                  invchead_misc_amount) <= 0) AS foo "
         "WHERE  (checkInvoiceSitePrivs(invchead_id));");
  if (q.first() && q.value("numZeroInvcs").toInt() > 0)
  {
    int toPost = QMessageBox::question(this, tr("Invoices for 0 Amount"),
				       tr("There are %1 invoices with a total value of 0.\n"
					  "Would you like to post them?")
					 .arg(q.value("numZeroInvcs").toString()),
				       tr("Post All"), tr("Post Only Non-0"),
				       tr("Cancel"), 1, 2);
    if (toPost == 2)
      return;
    else if (toPost == 1)
      inclZero = false;
    else
      inclZero = true;
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  q.prepare("SELECT postInvoices(:postUnprinted, :inclZero) AS result;");
  q.bindValue(":postUnprinted", QVariant(_postUnprinted->isChecked()));
  q.bindValue(":inclZero",      QVariant(inclZero, 0));
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();

    if (result == -5)
    {
      QMessageBox::critical( this, tr("Cannot Post one or more Invoices"),
                             tr( "The G/L Account Assignments for one or more of the Invoices that you are trying to post are not\n"
                                 "configured correctly.  Because of this, G/L Transactions cannot be posted for these Invoices.\n"
                                 "You must contact your Systems Administrator to have this corrected before you may\n"
                                 "post these Invoices." ) );
      return;
    }
    else if (result < 0)
    {
      systemError( this, tr("A System Error occurred at %1::%2, Error #%3.")
                         .arg(__FILE__)
                         .arg(__LINE__)
                         .arg(q.value("result").toInt()) );
      return;
    }


    omfgThis->sInvoicesUpdated(-1, TRUE);
    omfgThis->sSalesOrdersUpdated(-1);

    if (_printJournal->isChecked())
    {
      ParameterList params;
      params.append("journalNumber", result);

      orReport report("SalesJournal", params);
      if (report.isValid())
        report.print();
      else
        report.reportError(this);
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError( this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  accept();
}

void postInvoices::sSubmit()
{
  ParameterList params;
  params.append("action_name", "PostInvoices");
  params.append("postUnprinted", QVariant(_postUnprinted->isChecked(), 0));
  params.append("printSalesJournal", QVariant(_printJournal->isChecked(), 0));

  submitAction newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() == XDialog::Accepted)
    accept();
}

