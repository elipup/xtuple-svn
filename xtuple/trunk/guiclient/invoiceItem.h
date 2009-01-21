/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef INVOICEITEM_H
#define INVOICEITEM_H

#include "guiclient.h"
#include "xdialog.h"
#include <parameter.h>

#include "ui_invoiceItem.h"

class invoiceItem : public XDialog, public Ui::invoiceItem
{
    Q_OBJECT

public:
    invoiceItem(QWidget* = 0, const char* = 0, bool = false, Qt::WFlags = 0);
    ~invoiceItem();

    virtual void populate();

public slots:
    virtual SetResponse set( const ParameterList & pParams );
    virtual void sSave();
    virtual void sCalculateExtendedPrice();
    virtual void sPopulateItemInfo( int pItemid );
    virtual void sDeterminePrice();
    virtual void sListPrices();
    virtual void sLookupTax();
    virtual void sLookupTaxCode();
    virtual void sPriceGroup();
    virtual void sTaxDetail();
    virtual void sQtyUOMChanged();
    virtual void sPriceUOMChanged();
    virtual void sMiscSelected(bool);

protected slots:
    virtual void languageChange();

private:
    int _mode;
    int _invcheadid;
    int _custid;
    int _invcitemid;
    double _priceRatioCache;
    double	_cachedPctA;
    double	_cachedPctB;
    double	_cachedPctC;
    double	_cachedRateA;
    double	_cachedRateB;
    double	_cachedRateC;
    int		_taxauthid;
    int _invuomid;
    double _qtyinvuomratio;
    double _priceinvuomratio;
};

#endif // INVOICEITEM_H
