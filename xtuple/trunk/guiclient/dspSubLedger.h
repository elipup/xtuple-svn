/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPSUBLEDGER_H
#define DSPSUBLEDGER_H

#include "guiclient.h"
#include "xwidget.h"
#include <parameter.h>

#include "ui_dspSubLedger.h"

class dspSubLedger : public XWidget, public Ui::dspSubLedger
{
    Q_OBJECT

public:
    dspSubLedger(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~dspSubLedger();
    virtual bool setParams(ParameterList &params);

public slots:
    virtual enum SetResponse set( const ParameterList & pParams );
    virtual void sPopulateMenu(QMenu*, QTreeWidgetItem*);
    virtual void sPrint();
    virtual void sFillList();
    virtual void sViewTrans();
    virtual void sViewSeries();
    virtual void sViewDocument();

protected slots:
    virtual void languageChange();

private:
    QStringList _sources;

};

#endif // DSPSUBLEDGER_H
