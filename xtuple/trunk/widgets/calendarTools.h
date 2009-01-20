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

#ifndef calendarTools_h
#define calendarTools_h

#include <QDateTime>

#include <parameter.h>

#include "widgets.h"
#include "xcombobox.h"
#include "xtreewidget.h"

class QString;

class XTUPLEWIDGETS_EXPORT Numeric
{
  public:
    Numeric()
    {
      value = 0.0;
    }
  
    inline double operator+=(double pValue) { value += pValue; return value; }
    inline double toDouble()                { return value;                  }

    double value;
};

class XTUPLEWIDGETS_EXPORT DatePair
{
  public:
    DatePair()
    {
    }

    DatePair(QDate pStartDate, QDate pEndDate)
    {
      startDate = pStartDate;
      endDate = pEndDate;
    }

    QDate startDate;
    QDate endDate;
};


class XTUPLEWIDGETS_EXPORT CalendarComboBox : public XComboBox
{
  Q_OBJECT

  public:
    CalendarComboBox(QWidget *, const char * = 0);

    void load(ParameterList &);

  signals:
    void newCalendarId(int);
    void select(ParameterList &);
};


class PeriodListViewItem;

class XTUPLEWIDGETS_EXPORT PeriodsListView : public XTreeWidget
{
  Q_OBJECT

  public:
    PeriodsListView(QWidget *, const char * = 0);

    QList<QVariant>    periodList();
    QString            periodString();
    PeriodListViewItem *getSelected(int);
    void               getSelected(ParameterList &);
    bool               isPeriodSelected();

  public slots:
    void populate(int);
    void load(ParameterList &);

  private:
    int _calheadid;
};

class XTUPLEWIDGETS_EXPORT PeriodListViewItem : public XTreeWidgetItem
{
  public:
    PeriodListViewItem( PeriodsListView *, XTreeWidgetItem *, int,
                        QDate, QDate,
                        QString, QString );

    inline QDate startDate() { return _startDate; }
    inline QDate endDate()   { return _endDate;   }

  private:
    QDate _startDate;
    QDate _endDate;
};

#endif

