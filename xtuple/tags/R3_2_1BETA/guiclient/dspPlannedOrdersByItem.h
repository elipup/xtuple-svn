/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPPLANNEDORDERSBYITEM_H
#define DSPPLANNEDORDERSBYITEM_H

#include "guiclient.h"
#include "xwidget.h"

#include "ui_dspPlannedOrdersByItem.h"

class dspPlannedOrdersByItem : public XWidget, public Ui::dspPlannedOrdersByItem
{
    Q_OBJECT

public:
    dspPlannedOrdersByItem(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~dspPlannedOrdersByItem();

public slots:
    virtual void sPrint();
    virtual void sPopulateMenu( QMenu * pMenu, QTreeWidgetItem * pSelected );
    virtual void sDspRunningAvailability();
    virtual void sFirmOrder();
    virtual void sSoftenOrder();
    virtual void sReleaseOrder();
    virtual void sChangeType();
    virtual void sDeleteOrder();
    virtual void sFillList();
    virtual bool setParams(ParameterList &);

protected slots:
    virtual void languageChange();

};

#endif // DSPPLANNEDORDERSBYITEM_H
