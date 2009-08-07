/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPAROPENITEMS_H
#define DSPAROPENITEMS_H

#include "guiclient.h"
#include "xwidget.h"
#include <parameter.h>

#include "ui_dspAROpenItems.h"

class dspAROpenItems : public XWidget, public Ui::dspAROpenItems
{
    Q_OBJECT

public:
    dspAROpenItems(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~dspAROpenItems();
    
    Q_INVOKABLE bool checkInvoiceSitePrivs(int);
    Q_INVOKABLE bool checkCreditMemoSitePrivs(int);
    Q_INVOKABLE bool checkSalesOrderSitePrivs(int);

public slots:
    virtual SetResponse set( const ParameterList & pParams );
    virtual bool setParams(ParameterList &);
    virtual void sApplyAropenCM();
    virtual void sCCRefundCM();
    virtual void sPopulateMenu( QMenu * pMenu, QTreeWidgetItem *pItem );
    virtual void sDeleteCreditMemo();
    virtual void sDeleteInvoice();
    virtual void sDspShipmentStatus();
    virtual void sEdit();
    virtual void sEditSalesOrder();
    virtual void sEnterMiscArCreditMemo();
    virtual void sEnterMiscArDebitMemo();
    virtual void sFillList();
    virtual void sCreateInvoice();
    virtual void sNewCashrcpt();
    virtual void sNewCreditMemo();
    virtual void sView();
    virtual void sViewCreditMemo();
    virtual void sViewInvoice();
    virtual void sViewInvoiceDetails();
    virtual void sViewSalesOrder();
    virtual void sIncident();
    virtual void sViewIncident();
    virtual void sPrint();
    virtual void sPrintItem();
    virtual void sPrintStatement();
    virtual void sPost();
    virtual void sPostInvoice();
    virtual void sPostCreditMemo();
    virtual void sShipment();
    virtual void sHandleButtons(bool);
    virtual void sHandleStatementButton();
    virtual void sClosedToggled(bool);

protected slots:
    virtual void languageChange();

};

#endif // DSPAROPENITEMS_H
