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

//  plCluster.h
//  Created 04/15/2002 JSL
//  Copyright (c) 2002-2008, OpenMFG, LLC

#ifndef plCluster_h
#define plCluster_h

#include "xlineedit.h"

#include "OpenMFGWidgets.h"

class QLabel;
class QPushButton;

class PlanOrdCluster;

class OPENMFGWIDGETS_EXPORT PlanOrdLineEdit : public XLineEdit
{
  Q_OBJECT

friend class PlanOrdCluster;

  public:
    PlanOrdLineEdit(QWidget *, const char * = 0);

  public slots:
    void setId(int);
    void sParse();

  private:
    QChar  _status;
    double _qtyOrdered;
    XDataWidgetMapper *_mapper;

  signals:
    void newId(int);
    void warehouseChanged(const QString &);
    void itemNumberChanged(const QString &);
    void uomChanged(const QString &);
    void itemDescrip1Changed(const QString &);
    void itemDescrip2Changed(const QString &);
    void dueDateChanged(const QString &);
    void qtyChanged(const QString &);
    void statusChanged(const QString &);
    void valid(bool);
};

class OPENMFGWIDGETS_EXPORT PlanOrdCluster : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString  fieldName      READ fieldName      WRITE setFieldName)
  Q_PROPERTY(QString  number         READ number         WRITE setNumber         DESIGNABLE false)
  Q_PROPERTY(QString  defaultNumber  READ defaultNumber                          DESIGNABLE false)

  public:
    PlanOrdCluster(QWidget *, const char * = 0);
    PlanOrdCluster(int, QWidget *, const char * = 0);

    QString woNumber() const;

    inline int id()                 { return _number->_id;          }
    inline bool isValid()           { return _number->_valid;       }
    inline char status()            { return _number->_status.toAscii(); }
    inline double qty()             { return _number->_qtyOrdered;  }
    inline QString number()  const  { return _number->text();       }
    QString defaultNumber()  const  { return _default; }
    QString fieldName()      const  { return _fieldName; };

  public slots:
    void setDataWidgetMap(XDataWidgetMapper* m);
    void setFieldName(QString p)    { _fieldName = p; };
    void setId(int);
    void setNumber(const QString& number);
    void setReadOnly(bool);

  private slots:
    void sList();

  private:
    void constructor();

    PlanOrdLineEdit  *_number;
    QPushButton      *_list;
    QLabel           *_warehouse;
    QLabel           *_itemNumber;
    QLabel           *_uom;
    QLabel           *_descrip1;
    QLabel           *_descrip2;
    QLabel           *_status;
    QString          _default;
    QString          _fieldName;

  signals:
    void newId(int);
    void dueDateChanged(const QString &);
    void qtyChanged(const QString &);

    void valid(bool);
};

#endif

