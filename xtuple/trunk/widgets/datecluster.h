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
 * The Original Code is PostBooks Accounting, ERP, and CRM Suite. 
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
 * Powered by PostBooks, an open source solution from xTuple
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


#ifndef datecluster_h
#define datecluster_h

#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "xdatawidgetmapper.h"
#include "xlineedit.h"

class ParameterList;
class QFocusEvent;
class XSqlQuery;

class XDateEdit : public XLineEdit
{
  Q_OBJECT
  Q_ENUMS   (Defaults)
  Q_PROPERTY(QDate    date            READ date        WRITE setDate)
  Q_PROPERTY(Defaults defaultDate     READ defaultDate WRITE setDefaultDate);
  Q_PROPERTY(QDate    currentDefault  READ currentDefault);
  Q_PROPERTY(QString  fieldName       READ fieldName   WRITE setFieldName);

  public:
    XDateEdit(QWidget *parent = 0, const char * = 0);
    virtual ~XDateEdit();
    
    enum Defaults     { Empty, Current, None };

    virtual bool      isNull();
    virtual bool      isValid();
    virtual Defaults  defaultDate()                             { return _default;};
    virtual QDate     currentDefault();
    virtual QDate     date();
    virtual QString   fieldName()   const                       { return _fieldName;        };
    virtual void      clear();
    inline  void      setAllowNullDate(bool pAllowNull)         { _allowNull  = pAllowNull;  };
    inline  void      setNullString(const QString &pNullString) { _nullString = pNullString;};
    inline  void      setNullDate(const QDate &pNullDate)       { _nullDate   = pNullDate;    };

  public slots:
    virtual void setDataWidgetMap(XDataWidgetMapper* m);
    virtual void setFieldName(QString p) { _fieldName = p; };

    void setNull();
    void setDate(const QDate &, bool = false);
    void setDefaultDate(Defaults p)                             { _default = p; };
    void parseDate();
    void showCalendar();

  signals:
    void newDate(const QDate &);

  private:
    bool fixMonthEnd(int *, int, int);
    
    bool          _allowNull;
    enum Defaults _default;
    QDate         _currentDate;
    QDate         _nullDate;
    QString       _fieldName;
    QString       _nullString;

};

class OPENMFGWIDGETS_EXPORT DLineEdit : public QWidget
{
  Q_OBJECT
  Q_ENUMS(XDateEdit::Defaults)
  Q_PROPERTY(QDate                 date            READ date        WRITE setDate);
  Q_PROPERTY(XDateEdit::Defaults   defaultDate     READ defaultDate WRITE setDefaultDate);
  Q_PROPERTY(QString               fieldName       READ fieldName   WRITE setFieldName);

  public:
    DLineEdit(QWidget *parent = 0, const char * = 0);
    
    inline void                 setAllowNullDate(bool p)            { _lineedit.setAllowNullDate(p); };
    inline void                 setNullString(const QString &p)     { _lineedit.setNullString(p); };
    inline void                 setNullDate(const QDate &p)         { _lineedit.setNullDate(p); };

    virtual bool                isNull()                            { return _lineedit.isNull(); };
    virtual bool                isValid()                           { return _lineedit.isValid(); };
    virtual XDateEdit::Defaults defaultDate()                       { return _lineedit.defaultDate(); };
    virtual QDate               date()                              { return _lineedit.date(); };
    virtual QString             fieldName() const                   { return _lineedit.fieldName(); };
    virtual void                clear()                             { _lineedit.clear(); };
      
  public slots:
    virtual void setDataWidgetMap(XDataWidgetMapper* m)             { _lineedit.setDataWidgetMap(m); };
    virtual void setDefaultDate(XDateEdit::Defaults p)              { _lineedit.setDefaultDate(p); };
    virtual void setEnabled(const bool);
    virtual void setFieldName(QString p)                            { _lineedit.setFieldName(p); };

    void setDate(const QDate &p, bool b = false)                    { _lineedit.setDate(p, b); };
    void setNull()                                                  { _lineedit.setNull(); };
    void showCalendar()                                             { _lineedit.showCalendar(); };

  signals:
    void newDate(const QDate &);

  private:
    QPushButton _calbutton;
    XDateEdit   _lineedit;
};


class OPENMFGWIDGETS_EXPORT DateCluster : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate)
  Q_PROPERTY(QDate endDate   READ endDate   WRITE setEndDate)

  public:
    DateCluster(QWidget *, const char * = 0);

    void setStartNull(const QString &, const QDate &, bool);
    void setEndNull(const QString &, const QDate &, bool);

    void appendValue(ParameterList &);
    void bindValue(XSqlQuery &);
    
    inline QDate startDate() { return _startDate->date(); };
    inline QDate endDate()   { return _endDate->date();   };

    inline void  setStartDate(const QDate &pDate) { _startDate->setDate(pDate); };
    inline void  setEndDate(const QDate &pDate)   { _endDate->setDate(pDate);   };

    inline bool  allValid() { return ((_startDate->isValid()) && (_endDate->isValid())); };

    void setStartCaption(const QString &);
    void setEndCaption(const QString &);

  signals:
    void updated();

  protected:
    DLineEdit *_startDate;
    DLineEdit *_endDate;

  private:
    QLabel    *_startDateLit;
    QLabel    *_endDateLit;
    QString   _fieldNameStart;
    QString   _fieldNameEnd;
};

#endif

