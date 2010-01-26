/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPBOOKINGSBYCUSTOMER_H
#define DSPBOOKINGSBYCUSTOMER_H

#include "guiclient.h"
#include "xwidget.h"
#include <parameter.h>

#include "ui_dspBookingsByCustomer.h"

class dspBookingsByCustomer : public XWidget, public Ui::dspBookingsByCustomer
{
    Q_OBJECT

public:
    dspBookingsByCustomer(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~dspBookingsByCustomer();

    virtual bool setParams(ParameterList &);

public slots:
    virtual enum SetResponse set( const ParameterList & pParams );

protected slots:
    virtual void languageChange();

    virtual void sPrint();
    virtual void sFillList();

};

#endif // DSPBOOKINGSBYCUSTOMER_H
