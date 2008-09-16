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

//  Pocluster.h
//  Created 02/27/2002 JSL
//  Copyright (c) 2002-2008, OpenMFG, LLC

#ifndef __POCLUSTER_H__
#define __POCLUSTER_H__

#include "xlineedit.h"

#include "OpenMFGWidgets.h"

class QLabel;
class QPushButton;

class PoCluster;

//  Possible P/O Status
#define cPOUnposted 0x01
#define cPOOpen     0x02
#define cPOClosed   0x04

#define cPOItem     0x01
#define cPOItemsrc  0x02

class OPENMFGWIDGETS_EXPORT PoLineEdit : public XLineEdit
{
  Q_OBJECT

  Q_PROPERTY(int type READ type WRITE setType DESIGNABLE false);

friend class PoCluster;

  public:
    PoLineEdit(QWidget *, const char * = 0);

    Q_INVOKABLE inline int vendId()     { return _vendid; }
    inline int type()       { return _type;   }

    inline void setType(int pType) { _type = pType;  }

  public slots:
    void setId(int);
    void setNumber(int);
    void clear();
    void sParse();

  signals:
    void valid(bool);
    void newId(int);
    void vendidChanged(int);
    void vendNameChanged(const QString &);
    void vendAddress1Changed(const QString &);
    void vendAddress2Changed(const QString &);
    void vendAddress3Changed(const QString &);
    void vendCityStateZipChanged(const QString &);
    void numberChanged(int);
    void numberChanged(const QString &);

  private:
    int  _vendid;
    int  _type;
    int  _number;

  private slots:
    void sMarkDirty();
    
  protected:
    XDataWidgetMapper *_mapper;
    
};

class OPENMFGWIDGETS_EXPORT PoCluster : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString fieldName      READ fieldName      WRITE setFieldName);
  Q_PROPERTY(QString number         READ number         WRITE setNumber       DESIGNABLE false);
  Q_PROPERTY(QString defaultNumber  READ defaultNumber                        DESIGNABLE false);
  Q_PROPERTY(int     id             READ id             WRITE setId           DESIGNABLE false);
  Q_PROPERTY(int     type           READ type           WRITE setType         DESIGNABLE false);

  public:
    PoCluster(QWidget *, const char * = 0);

    inline void     setId(int pId)     { _poNumber->setId(pId);     }
    inline int      id()               { return _poNumber->_id;     }
    Q_INVOKABLE inline int  vendid()   { return _poNumber->_vendid; }
    Q_INVOKABLE inline bool isValid()  { return _poNumber->_valid;  }
    inline QString  defaultNumber()    { return QString();          }
    inline QString  fieldName()        { return _fieldName;         }
    inline QString  number()           { return _poNumber->text();  }
    inline void setType(int pType)     { _poNumber->setType(pType); }
    inline int  type()                 { return _poNumber->type();  }

    inline QString poNumber()          { return QString("%1").arg(_poNumber->_number); }

    void setReadOnly(bool);

  signals:
    void newId(int);
    void newVendid(int);
    void valid(bool);

  public slots:
    void setDataWidgetMap(XDataWidgetMapper* m);
    void setFieldName(const QString& name)        { _fieldName = name; }
    void setNumber(const QString& number);
    
  private slots:
    void sList();

  private:
    PoLineEdit  *_poNumber;
    QPushButton *_poList;
    QLabel      *_vendName;
    QString      _fieldName;
};

#endif

