/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "printInvoicesByShipvia.h"

#include <QVariant>
#include <parameter.h>
#include <QMessageBox>
#include <QValidator>
#include <openreports.h>
#include "editICMWatermark.h"
#include "deliverInvoice.h"
#include "submitAction.h"

/*
 *  Constructs a printInvoicesByShipvia as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
printInvoicesByShipvia::printInvoicesByShipvia(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_invoiceNumOfCopies, SIGNAL(valueChanged(int)), this, SLOT(sHandleCopies(int)));
    connect(_invoiceWatermarks, SIGNAL(itemSelected(int)), this, SLOT(sEditWatermark()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
printInvoicesByShipvia::~printInvoicesByShipvia()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void printInvoicesByShipvia::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <QSqlError>

void printInvoicesByShipvia::init()
{
  _invoiceWatermarks->addColumn( tr("Copy #"),      _dateColumn, Qt::AlignCenter );
  _invoiceWatermarks->addColumn( tr("Watermark"),   -1,          Qt::AlignLeft   );
  _invoiceWatermarks->addColumn( tr("Show Prices"), _dateColumn, Qt::AlignCenter );

  _invoiceNumOfCopies->setValue(_metrics->value("InvoiceCopies").toInt());
  if (_invoiceNumOfCopies->value())
  {
    for (int i = 0; i < _invoiceWatermarks->topLevelItemCount(); i++)
    {
      _invoiceWatermarks->topLevelItem(i)->setText(1, _metrics->value(QString("InvoiceWatermark%1").arg(i)));
      _invoiceWatermarks->topLevelItem(i)->setText(2, ((_metrics->value(QString("InvoiceShowPrices%1").arg(i)) == "t") ? tr("Yes") : tr("No")));
    }
  }

  _shipvia->populate( "SELECT DISTINCT -1, invchead_shipvia "
                      "  FROM invchead "
                      " WHERE ( (NOT invchead_printed)"
                      "   AND   (NOT invchead_posted) )" );

  if(!_privileges->check("PostMiscInvoices"))
  {
    _post->setChecked(false);
    _post->setEnabled(false);
  }
}

void printInvoicesByShipvia::sPrint()
{
  XSqlQuery invoices;
  invoices.prepare( "SELECT * FROM ("
                    "  SELECT invchead_id, invchead_invcnumber, invchead_ordernumber, "
                    "    findCustomerForm(invchead_cust_id, 'I') AS reportname "
                    "  FROM invchead "
                    "  WHERE ( (NOT invchead_printed)"
                    "  AND   (NOT invchead_posted)"
                    "  AND   (invchead_shipvia=:shipvia)) ) AS data "
                    "WHERE   (checkInvoiceSitePrivs(invchead_id)) "
                    "ORDER BY invchead_ordernumber;" );

  invoices.bindValue(":shipvia", _shipvia->currentText());
  invoices.exec();
  if (invoices.first())
  {
    XSqlQuery local;
    QPrinter  printer(QPrinter::HighResolution);
    bool      setupPrinter = TRUE;
    bool userCanceled = false;
    if (orReport::beginMultiPrint(&printer, userCanceled) == false)
    {
      if(!userCanceled)
        systemError(this, tr("Could not initialize printing system for multiple reports."));
      return;
    }

    do
    {
      int invoiceNumber = invoices.value("invchead_invcnumber").toInt();
      if (invoiceNumber == 0)
      {
        local.prepare("SELECT fetchInvcNumber() AS invoicenumber;");
        local.exec();
        if (local.first())
        {
          invoiceNumber = local.value("invoicenumber").toInt();
          local.prepare( "UPDATE invchead "
                         "SET invchead_invcnumber=text(:invoicenumber) "
                         "WHERE (invchead_id=:invchead_id);" );
          local.bindValue(":invoicenumber", invoiceNumber);
          local.bindValue(":invchead_id", invoices.value("invchead_id").toInt());
          local.exec();
        }
	// not else because local is re-prepared inside the block
	if (local.lastError().type() != QSqlError::NoError)
	{
	  systemError(this, local.lastError().databaseText(), __FILE__, __LINE__);
	  return;
	}
      }

      XSqlQuery xrate;
      xrate.prepare("SELECT curr_rate "
		   "FROM curr_rate, invchead "
		   "WHERE ((curr_id=invchead_curr_id)"
		   "  AND  (invchead_id=:invchead_id)"
		   "  AND  (invchead_invcdate BETWEEN curr_effective AND curr_expires));");
      // if SUM becomes dependent on curr_id then move XRATE before it in the loop

      XSqlQuery sum;
      sum.prepare("SELECT COALESCE(SUM(round((invcitem_billed * invcitem_qty_invuomratio) *"
                  "                 (invcitem_price / "
                  "                  CASE WHEN (item_id IS NULL) THEN 1"
                  "                       ELSE invcitem_price_invuomratio"
                  "                  END), 2)),0) + "
                  "       invchead_freight + invchead_tax + "
                  "       invchead_misc_amount AS subtotal "
                  "  FROM invchead LEFT OUTER JOIN"
                  "       invcitem ON (invcitem_invchead_id=invchead_id) LEFT OUTER JOIN"
                  "       item ON (invcitem_item_id=item_id) "
                  " WHERE(invchead_id=:invchead_id) "
                  " GROUP BY invchead_freight, invchead_tax, invchead_misc_amount;");
      message( tr("Printing Invoice #%1...")
               .arg(invoiceNumber) );

      for (int i = 0; i < _invoiceWatermarks->topLevelItemCount(); i++ )
      {
	QTreeWidgetItem *cursor = _invoiceWatermarks->topLevelItem(i);
        ParameterList params;
        params.append("invchead_id", invoices.value("invchead_id").toInt());
        params.append("showcosts", ((cursor->text(2) == tr("Yes")) ? "TRUE" : "FALSE"));
        params.append("watermark", cursor->text(1));

        orReport report(invoices.value("reportname").toString(), params);

        message( tr("Printing Invoice #%1...")
                 .arg(invoiceNumber) );

        if (!report.isValid())
        {
          QMessageBox::critical( this, tr("Cannot Find Invoice Form"),
                                 tr( "The Invoice Form '%1' cannot be found for Invoice #%2.\n"
                                     "This Invoice cannot be printed until a Customer Form Assignment is updated to remove any\n"
                                     "references to this Invoice Form or this Invoice Form is created." )
                                 .arg(invoices.value("reportname").toString())
                                 .arg(invoiceNumber) );

          resetMessage();
        }
        else
        {
          if (report.print(&printer, setupPrinter))
            setupPrinter = FALSE;
          else
          {
            systemError( this, tr("A Printing Error occurred at printInvoices::%1.")
                               .arg(__LINE__) );
	    orReport::endMultiPrint(&printer);
            return;
          }

          if (_post->isChecked())
          {
	    int invcheadId = invoices.value("invchead_id").toInt();
	    sum.bindValue(":invchead_id", invcheadId);
	    if (sum.exec() && sum.first() && sum.value("subtotal").toDouble() == 0)
	    {
	      if (QMessageBox::question(this, tr("Invoice Has Value 0"),
					tr("Invoice #%1 has a total value of 0.\n"
					   "Would you like to post it anyway?")
					  .arg(invoiceNumber),
					QMessageBox::Yes,
					QMessageBox::No | QMessageBox::Default)
		    == QMessageBox::No)
		continue;
	    }
	    else if (sum.lastError().type() != QSqlError::NoError)
	    {
	      systemError(this, sum.lastError().databaseText(), __FILE__, __LINE__);
	      continue;
	    }
	    else if (sum.value("subtotal").toDouble() != 0)
	    {
	      xrate.bindValue(":invchead_id", invcheadId);
	      xrate.exec();
	      if (xrate.lastError().type() != QSqlError::NoError)
	      {
		 systemError(this, tr("System Error posting Invoice #%1\n%2")
				     .arg(invoiceNumber)
				     .arg(xrate.lastError().databaseText()),
			     __FILE__, __LINE__);
		 continue;
	      }
	      else if (!xrate.first() || xrate.value("curr_rate").isNull())
	      {
		 systemError(this, tr("Could not post Invoice #%1 because of a missing exchange rate.")
				     .arg(invoiceNumber));
		 continue;
	      }
	    }

            message( tr("Posting Invoice #%1...")
                     .arg(invoiceNumber) );

            local.prepare("SELECT postInvoice(:invchead_id) AS result;");
            local.bindValue(":invchead_id", invcheadId);
            local.exec();
	    if (local.lastError().type() != QSqlError::NoError)
	    {
	      systemError(this, local.lastError().databaseText(), __FILE__, __LINE__);
	    }
          }
        }
      }
      if (_metrics->boolean("EnableBatchManager"))
      {  
        // TODO: Check for EDI and handle submission to Batch here
        XSqlQuery query;
        query.prepare("SELECT CASE WHEN (COALESCE(shipto_ediprofile_id, -2) = -2)"
                "              THEN COALESCE(cust_ediprofile_id,-1)"
                "            ELSE COALESCE(shipto_ediprofile_id,-2)"
                "       END AS result,"
                "       COALESCE(cust_emaildelivery, false) AS custom"
                "  FROM cust, invchead"
                "       LEFT OUTER JOIN shipto"
                "         ON (invchead_shipto_id=shipto_id)"
                "  WHERE ((invchead_cust_id=cust_id)"
                "    AND  (invchead_id=:invchead_id)); ");
        query.bindValue(":invchead_id", invoices.value("invchead_id").toInt());
        query.exec();
        if(query.first())
        {
          if(query.value("result").toInt() == -1)
          {
            if(query.value("custom").toBool())
            {
              ParameterList params;
              params.append("invchead_id", invoices.value("invchead_id").toInt());

              deliverInvoice newdlg(this, "", TRUE);
              newdlg.set(params);
              newdlg.exec();
            }
          }
          else
          {
            ParameterList params;
            params.append("action_name", "TransmitInvoice");
            params.append("invchead_id", invoices.value("invchead_id").toInt());

            submitAction newdlg(this, "", TRUE);
            newdlg.set(params);
            newdlg.exec();
          }
        }
        else if (query.lastError().type() != QSqlError::NoError)
        {
	      systemError(this, query.lastError().databaseText(), __FILE__, __LINE__);
	      return;
        }
      }
    }
    while (invoices.next());
    orReport::endMultiPrint(&printer);

    resetMessage();

    if (!_post->isChecked())
    {
      if ( QMessageBox::information( this, tr("Mark Invoices as Printed?"),
                                     tr("Did all of the Invoices print correctly?"),
                                     tr("&Yes"), tr("&No"), QString::null, 0, 1) == 0)
      {
        q.prepare( "UPDATE invchead "
                   "   SET invchead_printed=TRUE "
                   " WHERE ( (NOT invchead_printed)"
                   "   AND   (invchead_shipvia=:shipvia) ); ");
        q.bindValue(":shipvia", _shipvia->currentText());
        q.exec();
	if (q.lastError().type() != QSqlError::NoError)
	{
	  systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	  return;
	}

        omfgThis->sInvoicesUpdated(-1, TRUE);
      }
    }
  }
  else if (invoices.lastError().type() != QSqlError::NoError)
  {
    systemError(this, invoices.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  else
    QMessageBox::information( this, tr("No Invoices to Print"),
                              tr("There aren't any Invoices to print for the selected Ship Via.") );

  accept();
}

void printInvoicesByShipvia::sEditWatermark()
{
  QTreeWidgetItem *cursor = _invoiceWatermarks->currentItem();
  ParameterList params;
  params.append("watermark", cursor->text(1));
  params.append("showPrices", (cursor->text(2) == tr("Yes")));

  editICMWatermark newdlg(this, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec() == XDialog::Accepted)
  {
    cursor->setText(1, newdlg.watermark());
    cursor->setText(2, ((newdlg.showPrices()) ? tr("Yes") : tr("No")));
  }
}

void printInvoicesByShipvia::sHandleCopies( int pValue )
{
  if (_invoiceWatermarks->topLevelItemCount() > pValue)
    _invoiceWatermarks->takeTopLevelItem(_invoiceWatermarks->topLevelItemCount() - 1);
  else
  {
    for (int i = (_invoiceWatermarks->topLevelItemCount() + 1); i <= pValue; i++)
      new XTreeWidgetItem(_invoiceWatermarks,
			  _invoiceWatermarks->topLevelItem(_invoiceWatermarks->topLevelItemCount() - 1),
			  i, i, tr("Copy #%1").arg(i), "", tr("Yes"));
  }
}


