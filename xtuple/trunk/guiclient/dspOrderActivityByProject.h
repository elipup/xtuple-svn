/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPORDERACTIVITYBYPROJECT_H
#define DSPORDERACTIVITYBYPROJECT_H

#include "xwidget.h"

#include "ui_dspOrderActivityByProject.h"

class dspOrderActivityByProject : public XWidget, public Ui::dspOrderActivityByProject
{
    Q_OBJECT

public:
    dspOrderActivityByProject(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~dspOrderActivityByProject();

public slots:
    virtual void sPopulateMenu( QMenu * );
    virtual void sEdit();
    virtual void sView();
    virtual void sFillList();
    virtual void sPrint();

protected slots:
    virtual void languageChange();

};

#endif // DSPORDERACTIVITYBYPROJECT_H
