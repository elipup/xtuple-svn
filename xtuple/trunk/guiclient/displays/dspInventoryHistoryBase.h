/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef __DSPINVENTORYHISTORYBASE_H__
#define __DSPINVENTORYHISTORYBASE_H__

#include "guiclient.h"
#include "display.h"
#include <parameter.h>

#include "ui_dspInventoryHistoryBase.h"

class dspInventoryHistoryBase : public display, public Ui::dspInventoryHistoryBase
{
    Q_OBJECT

public:
    dspInventoryHistoryBase(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);

    virtual bool setParams(ParameterList & params);

public slots:
    virtual SetResponse set(const ParameterList & pParams );
    virtual void sViewTransInfo();
    virtual void sEditTransInfo();
    virtual void sViewWOInfo();
    virtual void sPopulateMenu( QMenu * pMenu, QTreeWidgetItem * pItem );
    virtual void sSalesOrderList();

protected slots:
    virtual void languageChange();

};

#endif // __DSPINVENTORYHISTORYBASE_H__
