/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPITEMSBYCHARACTERISTIC_H
#define DSPITEMSBYCHARACTERISTIC_H

#include "xwidget.h"

#include <parameter.h>

#include "ui_dspItemsByCharacteristic.h"

class dspItemsByCharacteristic : public XWidget, public Ui::dspItemsByCharacteristic
{
    Q_OBJECT

public:
    dspItemsByCharacteristic(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~dspItemsByCharacteristic();

    virtual bool setParams(ParameterList &);

public slots:
    virtual void sPrint();
    virtual void sPopulateMenu( QMenu * pMenu, QTreeWidgetItem * selected );
    virtual void sEdit();
    virtual void sView();
    virtual void sEditBOM();
    virtual void sViewBOM();
    virtual void sFillList();
    virtual void sFillList( int pItemid, bool pLocal );

protected slots:
    virtual void languageChange();

};

#endif // DSPITEMSBYCHARACTERISTIC_H
