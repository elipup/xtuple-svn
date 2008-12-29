/*
 * Common Public Attribution License Version 1.0.
 *
 * The contents of this file are subject to the Common Public Attribution
 * License Version 1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License
 * at http://www.xTuple.com/CPAL.  The License is based on the Mozilla
 * Public License Version 1.1 but Sections 14 and 15 have been added to
 * cover use of software over a computer network and provide for limited
 * attribution for the Original Developer. In addition, Exhibit A has
 * been modified to be consistent with Exhibit B.
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is xTuple ERP: PostBooks Edition
 *
 * The Original Developer is not the Initial Developer and is __________.
 * If left blank, the Original Developer is the Initial Developer.
 * The Initial Developer of the Original Code is OpenMFG, LLC,
 * d/b/a xTuple. All portions of the code written by xTuple are Copyright
 * (c) 1999-2008 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved.
 *
 * Contributor(s): ______________________.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the xTuple End-User License Agreeement (the xTuple License), in which
 * case the provisions of the xTuple License are applicable instead of
 * those above.  If you wish to allow use of your version of this file only
 * under the terms of the xTuple License and not to allow others to use
 * your version of this file under the CPAL, indicate your decision by
 * deleting the provisions above and replace them with the notice and other
 * provisions required by the xTuple License. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the CPAL or the xTuple License.
 *
 * EXHIBIT B.  Attribution Information
 *
 * Attribution Copyright Notice:
 * Copyright (c) 1999-2008 by OpenMFG, LLC, d/b/a xTuple
 *
 * Attribution Phrase:
 * Powered by xTuple ERP: PostBooks Edition
 *
 * Attribution URL: www.xtuple.org
 * (to be included in the "Community" menu of the application if possible)
 *
 * Graphic Image as provided in the Covered Code, if any.
 * (online at www.xtuple.com/poweredby)
 *
 * Display of Attribution Information is required in Larger Works which
 * are defined in the CPAL as a work which combines Covered Code or
 * portions thereof with code not governed by the terms of the CPAL.
 */

#ifndef DSPCUSTOMERINFORMATION_H
#define DSPCUSTOMERINFORMATION_H

#include "guiclient.h"
#include "xwidget.h"
#include <QStandardItemModel>
#include <parameter.h>

#include "ui_dspCustomerInformation.h"

class dspCustomerInformation : public XWidget, public Ui::dspCustomerInformation
{
    Q_OBJECT

public:
    dspCustomerInformation(QWidget* parent = 0, Qt::WFlags f = 0);
    ~dspCustomerInformation();

    virtual SetResponse set( const ParameterList & pParams );

    static void doDialog(QWidget * parent, const ParameterList & pParams);
    virtual bool checkSitePrivs(int invcid);

public slots:
    virtual void sPopulate();
    virtual void sEdit();
    virtual void sPrint();
    virtual void sFillQuoteList();
    virtual void sNewQuote();
    virtual void sEditQuote();
    virtual void sViewQuote();
    virtual void sFillOrderList();
    virtual void sNewOrder();
    virtual void sEditOrder();
    virtual void sViewOrder();
    virtual void sEditInvOrder();
    virtual void sViewInvOrder();
    virtual void sFillInvoiceList();
    virtual void sNewInvoice();
    virtual void sEditInvoice();
    virtual void sViewInvoice();
    virtual void sPostInvoice();
    virtual void sFillCreditMemoList();
    virtual void sNewCreditMemo();
    virtual void sEditCreditMemo();
    virtual void sViewCreditMemo();
    virtual void sEditAropen();
    virtual void sViewAropen();
    virtual void sFillPaymentsList();
    virtual void sPopulateMenuQuote(QMenu*);
    virtual void sPopulateMenuSalesOrder(QMenu*);
    virtual void sPopulateMenuInvoice(QMenu*, QTreeWidgetItem *selected);
    virtual void sPopulateMenuCreditMemo(QMenu*);
    virtual void sPopulateMenuArhist(QMenu*, QTreeWidgetItem *selected);
    virtual void sConvertQuote();
    virtual void sCreditMemoSelected(bool);
    virtual void sFillARHistory();
    virtual void sPopulateCustInfo();
    virtual void sCRMAccount();
    virtual void sARWorkbench();
    virtual void sShipment();
    virtual void sDspShipmentStatus();
    virtual void sInvShipment();
    virtual void sInvShipmentStatus();
    virtual void sCashReceipt();
    virtual void sPrintCCReceipt();
    virtual void sPrintSalesOrder();
    virtual void sPrintQuote();
    virtual void sPrintCreditMemo();
    virtual void sPrintInvoice();
    virtual void sHandleCreditMemoPrint();
    virtual void sPrintStatement();

protected slots:
    virtual void languageChange();

private:
    int _crmacctId;

};

#endif // DSPCUSTOMERINFORMATION_H
