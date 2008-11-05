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

#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "guiclient.h"
#include "xwidget.h"
#include <QStandardItemModel>
#include <parameter.h>

#include "ui_customer.h"

class customer : public XWidget, public Ui::customer
{
    Q_OBJECT

public:
    customer(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~customer();

public slots:
    virtual SetResponse set(const ParameterList & pParams );
    virtual void populate();
    virtual void sCheck();
    virtual void sDeleteCharacteristic();
    virtual void sDeleteShipto();
    virtual void sDeleteTaxreg();
    virtual void sEditCharacteristic();
    virtual void sEditCreditCard();
    virtual void sEditShipto();
    virtual void sEditTaxreg();
    virtual void sFillCcardList();
    virtual void sFillCharacteristicList();
    virtual void sFillShiptoList();
    virtual void sFillTaxregList();
    virtual void sMoveDown();
    virtual void sMoveUp();
    virtual void sNewCharacteristic();
    virtual void sNewCreditCard();
    virtual void sNewShipto();
    virtual void sNewTaxreg();
    virtual void sPopulateCommission();
    virtual void sPopulateShiptoMenu( QMenu * menuThis );
    virtual void sPrintShipto();
    virtual bool sSave( bool partial );
    virtual void sSave();
    virtual void sViewCreditCard();
    virtual void sViewShipto();
    virtual void sViewTaxreg();
    virtual void sLoadProspect(int);
    virtual void sLoadCrmAcct(int);

protected slots:
    virtual void languageChange();
    virtual int  saveContact(ContactCluster*);
    virtual void sProfileSelected();
    virtual void sSoProfileSelected();
    virtual void sNumberEdited();

protected:
    virtual void closeEvent(QCloseEvent*);

private:
    int _mode;
    int _custid;
    int	_crmacctid;
    int _NumberGen;
    QString _cachedNumber;
    QString key;
    bool _notice;
    QStandardItemModel * _custchar;

};

#endif // CUSTOMER_H
