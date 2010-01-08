/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef CONFIGURECC_H
#define CONFIGURECC_H

#include "guiclient.h"
#include "xdialog.h"

#include "ui_configureCC.h"

class configureCC : public XDialog, public Ui::configureCC
{
    Q_OBJECT

public:
    configureCC(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
    ~configureCC();

public slots:
    virtual void sDuplicateWindow(int);
    virtual void sSave();

protected slots:
    virtual void languageChange();
    virtual void sCCCompanyChanged(const int);

private:

};

#endif // CONFIGURECC_H
