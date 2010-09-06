/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef INCIDENTWORKBENCH_H
#define INCIDENTWORKBENCH_H

#include "display.h"
#include "ui_incidentWorkbench.h"

class incidentWorkbench : public display, public Ui::incidentWorkbench
{
    Q_OBJECT

public:
    incidentWorkbench(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);

public slots:
    virtual enum SetResponse set( const ParameterList & pParams );
    virtual bool setParams(ParameterList&);
    virtual void sNew();
    virtual void sEdit();
    virtual void sView();
};

#endif // INCIDENTWORKBENCH_H
