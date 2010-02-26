/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "openSalesOrders.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include <openreports.h>

#include "copySalesOrder.h"
#include "creditcardprocessor.h"
#include "dspSalesOrderStatus.h"
#include "dspShipmentsBySalesOrder.h"
#include "printPackingList.h"
#include "printSoForm.h"
#include "salesOrder.h"
#include "storedProcErrorLookup.h"

openSalesOrders::openSalesOrders(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);
  
  _cust->hide();
  _showGroup->hide();

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_so, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_copy, SIGNAL(clicked()), this, SLOT(sCopy()));
  connect(_autoUpdate, SIGNAL(toggled(bool)), this, SLOT(sHandleAutoUpdate(bool)));
  connect(_showClosed, SIGNAL(toggled(bool)), this, SLOT(sFillList()));

  _so->addColumn(tr("S/O #"),           _orderColumn, Qt::AlignLeft,  true, "cohead_number");
  _so->addColumn(tr("Cust. #"),         _orderColumn, Qt::AlignLeft,  true, "cust_number");
  _so->addColumn(tr("Customer"),         -1,          Qt::AlignLeft,  true, "cohead_billtoname");
  _so->addColumn(tr("Ship-To"),          _itemColumn, Qt::AlignLeft,  false,"cohead_shiptoname");
  _so->addColumn(tr("Cust. P/O Number"), -1         , Qt::AlignLeft,  true, "cohead_custponumber");
  _so->addColumn(tr("Ordered"),          _dateColumn, Qt::AlignCenter,true, "cohead_orderdate");
  _so->addColumn(tr("Scheduled"),        _dateColumn, Qt::AlignCenter,true, "scheddate");
  _so->addColumn(tr("Status"),         _statusColumn, Qt::AlignLeft, false, "status");
  
  if (_privileges->check("MaintainSalesOrders"))
  {
    connect(_so, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_so, SIGNAL(valid(bool)), _copy, SLOT(setEnabled(bool)));
    connect(_so, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_so, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_so, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  connect(omfgThis, SIGNAL(salesOrdersUpdated(int, bool)), this, SLOT(sFillList()));

  sHandleAutoUpdate(_autoUpdate->isChecked());
}

openSalesOrders::~openSalesOrders()
{
  // no need to delete child widgets, Qt does it all for us
}

void openSalesOrders::languageChange()
{
  retranslateUi(this);
}

enum SetResponse openSalesOrders::set(const ParameterList& pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool	   valid;
  
  param = pParams.value("run", &valid);
  if (valid)
  {
    connect(_warehouse, SIGNAL(updated()), this, SLOT(sFillList()));
    sFillList();
  }

  return NoError;
}

void openSalesOrders::setParams(ParameterList &params)
{
  params.append("error", tr("Error"));
  if (_cust->isValid())
    params.append("cust_id", _cust->id());
  if (_showClosed->isChecked() && _showClosed->isVisible())
    params.append("showClosed");
  if (_preferences->boolean("selectedSites") || _warehouse->isSelected())
    params.append("selectedSites");
  _warehouse->appendValue(params);
}

void openSalesOrders::sPrint()
{    
  ParameterList params;
  setParams(params);

  orReport report("ListOpenSalesOrders", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void openSalesOrders::sNew()
{
  salesOrder::newSalesOrder(_cust->id());
}

void openSalesOrders::sEdit()
{
  if (checkSitePrivs())
    salesOrder::editSalesOrder(_so->id(), false);
}

void openSalesOrders::sView()
{
  if (checkSitePrivs())
    salesOrder::viewSalesOrder(_so->id());
}

void openSalesOrders::sCopy()
{
  if (!checkSitePrivs())
    return;
    
  ParameterList params;
  params.append("sohead_id", _so->id());
      
  copySalesOrder newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void openSalesOrders::sDelete()
{
  if (!checkSitePrivs())
    return;
    
  if ( QMessageBox::warning(this, tr("Delete Sales Order?"),
                            tr("<p>Are you sure that you want to completely "
			       "delete the selected Sales Order?"),
			    QMessageBox::Yes,
			    QMessageBox::No | QMessageBox::Default) == QMessageBox::Yes)
  {
    q.prepare("SELECT deleteSo(:sohead_id) AS result;");
    q.bindValue(":sohead_id", _so->id());
    q.exec();
    if (q.first())
    {
      bool closeInstead = false;
      int result = q.value("result").toInt();
      if (result == -1 && _privileges->check("ProcessCreditCards"))
      {
	if ( QMessageBox::question(this, tr("Cannot Delete Sales Order"),
				   storedProcErrorLookup("deleteSo", result) + 
				   "<br>Would you like to refund the amount "
				   "charged and close the Sales Order instead?",
				   QMessageBox::Yes | QMessageBox::Default,
				   QMessageBox::No) == QMessageBox::Yes)
	{
	  CreditCardProcessor *cardproc = CreditCardProcessor::getProcessor();
	  if (! cardproc)
	    QMessageBox::critical(this, tr("Credit Card Processing Error"),
				  CreditCardProcessor::errorMsg());
	  else
	  {
	    XSqlQuery ccq;
            ccq.prepare("SELECT ccpay_id, ccpay_ccard_id, ccpay_curr_id,"
			"       SUM(ccpay_amount     * sense) AS amount,"
			"       SUM(ccpay_r_tax      * sense) AS tax,"
			"       SUM(ccpay_r_shipping * sense) AS freight "
                        "FROM (SELECT ccpay_id, ccpay_ccard_id, ccpay_curr_id, "
			"             CASE WHEN ccpay_status = 'C' THEN  1"
			"                  WHEN ccpay_status = 'R' THEN -1"
			"             END AS sense,"
			"             ccpay_amount,"
			"             COALESCE(ccpay_r_tax::NUMERIC, 0) AS ccpay_r_tax,"
			"             COALESCE(ccpay_r_shipping::NUMERIC, 0) AS ccpay_r_shipping "
			"      FROM ccpay, payco "
			"      WHERE ((ccpay_id=payco_ccpay_id)"
			"        AND  (ccpay_status IN ('C', 'R'))"
			"        AND  (payco_cohead_id=:coheadid)) "
			"      ) AS dummy "
                        "GROUP BY ccpay_id, ccpay_ccard_id, ccpay_curr_id;");
	    ccq.bindValue(":coheadid", _so->id());
	    ccq.exec();
	    if (ccq.first())
	    do
	    {
	      QString docnum = _so->currentItem()->text(0);
	      QString refnum = docnum;
              int ccpayid = ccq.value("ccpay_id").toInt();
	      int coheadid = _so->id();
	      int returnVal = cardproc->credit(ccq.value("ccpay_ccard_id").toInt(),
					       -2,
					       ccq.value("amount").toDouble(),
					       ccq.value("tax").toDouble(),
					       true,
					       ccq.value("freight").toDouble(),
					       0,
					       ccq.value("ccpay_curr_id").toInt(),
					       docnum, refnum, ccpayid,
					       "cohead", coheadid);
	      if (returnVal < 0)
	      {
		QMessageBox::critical(this, tr("Credit Card Processing Error"),
				      cardproc->errorMsg());
		return;
	      }
	      else if (returnVal > 0)
	      {
		QMessageBox::warning(this, tr("Credit Card Processing Warning"),
				     cardproc->errorMsg());
		closeInstead = true;
	      }
	      else if (! cardproc->errorMsg().isEmpty())
	      {
		QMessageBox::information(this, tr("Credit Card Processing Note"),
				     cardproc->errorMsg());
		closeInstead = true;
	      }
              else
                closeInstead = true;
	    } while (ccq.next());
	    else if (ccq.lastError().type() != QSqlError::NoError)
	    {
	      systemError(this, ccq.lastError().databaseText(),
			  __FILE__, __LINE__);
	      return;
	    }
	    else
	    {
	      systemError(this, tr("Could not find the ccpay records!"),
			  __FILE__, __LINE__);
	      return;
	    }

	  }
	}
      }
      else if (result == -2 || result == -5)
      {
	if ( QMessageBox::question(this, tr("Cannot Delete Sales Order"),
				   storedProcErrorLookup("deleteSo", result) + 
				   "<br>Would you like to Close the selected "
				   "Sales Order instead?",
				   QMessageBox::Yes | QMessageBox::Default,
				   QMessageBox::No) == QMessageBox::Yes)
	  closeInstead = true;
      }
      else if (result < 0)
      {
	systemError(this, storedProcErrorLookup("deleteSo", result),
		    __FILE__, __LINE__);
	return;
      }

      if (closeInstead)
      {
	q.prepare( "UPDATE coitem "
		   "SET coitem_status='C' "
		   "WHERE ((coitem_status <> 'X')"
		   "  AND  (coitem_cohead_id=:sohead_id));" );
	q.bindValue(":sohead_id", _so->id());
	q.exec();
	if (q.lastError().type() != QSqlError::NoError)
	{
	  systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	  return;
	}
      }

      omfgThis->sSalesOrdersUpdated(-1);
      omfgThis->sProjectsUpdated(-1);
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void openSalesOrders::sHandleAutoUpdate(bool pAutoUpdate)
{
  if (pAutoUpdate)
    connect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
  else
    disconnect(omfgThis, SIGNAL(tick()), this, SLOT(sFillList()));
}

void openSalesOrders::sPrintPackingList()
{
  if (!checkSitePrivs())
    return;
    
  ParameterList params;
  params.append("sohead_id", _so->id());

  printPackingList newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void openSalesOrders::sAddToPackingListBatch()
{
  if (!checkSitePrivs())
    return;
    
  q.prepare("SELECT addToPackingListBatch(:sohead_id) AS result;");
  q.bindValue(":sohead_id", _so->id());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("addToPackingListBatch", result), __FILE__, __LINE__);
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void openSalesOrders::sPopulateMenu(QMenu *pMenu)
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEdit()), 0);
  if (!_privileges->check("MaintainSalesOrders"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("View..."), this, SLOT(sView()), 0);

  menuItem = pMenu->insertItem(tr("Copy..."), this, SLOT(sCopy()), 0);
  if (!_privileges->check("MaintainSalesOrders"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Delete..."), this, SLOT(sDelete()), 0);
  if (!_privileges->check("MaintainSalesOrders"))
    pMenu->setItemEnabled(menuItem, FALSE);

  pMenu->insertSeparator();

  menuItem = pMenu->insertItem(tr("Print Packing List..."), this, SLOT(sPrintPackingList()), 0);
  if (!_privileges->check("PrintPackingLists"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Add to Packing List Batch..."), this, SLOT(sAddToPackingListBatch()), 0);
  if (!_privileges->check("MaintainPackingListBatch"))
    pMenu->setItemEnabled(menuItem, FALSE);

  menuItem = pMenu->insertItem(tr("Print Sales Order Form..."), this, SLOT(sPrintForms()), 0); 
  
  pMenu->insertSeparator();

  pMenu->insertItem(tr("Shipment Status..."), this, SLOT(sDspShipmentStatus()), 0);
  pMenu->insertItem(tr("Shipments..."), this, SLOT(sShipment()), 0);

}


void openSalesOrders::sFillList()
{
  ParameterList params;
  setParams(params);

  QString sql( "SELECT DISTINCT cohead.*,"
               "       COALESCE(cust_number, :error) AS cust_number,"
               "       getSoSchedDate(cohead_id) AS scheddate, "
               "       getSoStatus(cohead_id) AS status "
               "  FROM cohead "
               "    JOIN custinfo ON (cohead_cust_id=cust_id) "
               "<? if exists(\"selectedSites\") ?> "
               "    JOIN coitem ON (coitem_cohead_id=cohead_id) "
               "    JOIN itemsite ON (coitem_itemsite_id=itemsite_id) "
               "    JOIN site() ON (itemsite_warehous_id=warehous_id) "
               "<? else ?> "
               "    LEFT OUTER JOIN coitem ON (coitem_cohead_id=cohead_id) "
               "    LEFT OUTER JOIN itemsite ON (coitem_itemsite_id=itemsite_id) "
               "    LEFT OUTER JOIN whsinfo ON (itemsite_warehous_id=warehous_id) "
               " <? endif ?> "
               " WHERE((true) "
               "<? if exists(\"cust_id\") ?>"
               "  AND (cust_id=<? value(\"cust_id\") ?> )"
               "<? endif ?>"
               "<? if not exists(\"showClosed\") ?> "
               "  AND ((coitem_status = 'O') OR (coitem_status IS NULL)) "
               "<? endif ?>"
               "<? if  exists(\"warehous_id\") ?>"
               "  AND (warehous_id=<? value(\"warehous_id\") ?>)"
               "<? endif ?>"
               " ) "
               "ORDER BY cohead_number " );
  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);

  _so->populate(q);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  _so->setDragString("soheadid=");
}

void openSalesOrders::sPrintForms()
{
  if (!checkSitePrivs())
    return;
    
  ParameterList params;
  params.append("sohead_id", _so->id());

  printSoForm newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

bool openSalesOrders::checkSitePrivs()
{
  if (_preferences->boolean("selectedSites"))
  {
    XSqlQuery check;
    check.prepare("SELECT checkSOSitePrivs(:coheadid) AS result;");
    check.bindValue(":coheadid", _so->id());
    check.exec();
    if (check.first())
    {
    if (!check.value("result").toBool())
      {
        QMessageBox::critical(this, tr("Access Denied"),
                                       tr("You may not view or edit this Sales Order as it references "
                                       "a Site for which you have not been granted privileges.")) ;
        return false;
      }
    }
  }
  return true;
}

void openSalesOrders::sDspShipmentStatus()
{
  ParameterList params;
  params.append("sohead_id", _so->id());
  params.append("run");

  dspSalesOrderStatus *newdlg = new dspSalesOrderStatus();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void openSalesOrders::sShipment()
{
  ParameterList params;
  params.append("sohead_id", _so->id());

  dspShipmentsBySalesOrder* newdlg = new dspShipmentsBySalesOrder();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}
