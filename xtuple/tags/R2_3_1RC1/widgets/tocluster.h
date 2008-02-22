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
 * (c) 1999-2007 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
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
 * Copyright (c) 1999-2007 by OpenMFG, LLC, d/b/a xTuple
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

//  tocluster.h
//  Created 06/27/2007 GJM
//  Copyright (c) 2002-2007, OpenMFG, LLC

#ifndef toCluster_h
#define toCluster_h

#include "wcombobox.h"
#include "xlineedit.h"

#include "OpenMFGWidgets.h"

class QLabel;
class QPushButton;

class ToCluster;

//  Possible Transfer Order Status
#define cToOpen               0x01
#define cToClosed             0x02
#define cToAtShipping         0x04

class OPENMFGWIDGETS_EXPORT ToLineEdit : public XLineEdit
{
  Q_OBJECT

friend class ToCluster;

  public:
    ToLineEdit(QWidget *, const char * = 0);

  public slots:
    void setId(int);
    void setNumber(int);
    void clear();
    void sParse();

  signals:
    void valid(bool);
    void newId(int);
    void newSrcwhs(int);
    void newDstwhs(int);
    void numberChanged(int);
    void numberChanged(const QString &);

  private:
    int  _dstwhs;
    int  _number;
    int  _srcwhs;
    int  _type;
};


class OPENMFGWIDGETS_EXPORT ToCluster : public QWidget
{
  Q_OBJECT

  public:
    ToCluster(QWidget *, const char * = 0);
    ToCluster(int, QWidget *);

    inline int  dstwhs()           { return _toNumber->_dstwhs;  }
    inline int  id()               { return _toNumber->_id;      }
    inline int  number()           { return _toNumber->text().toInt();  }
    inline int  srcwhs()           { return _toNumber->_srcwhs;  }
    inline bool isValid()          { return _toNumber->_valid;   }
    inline void setType(int pType) { _toNumber->_type = pType;   }

  public slots:
    void setId(int);

  signals:
    void newId(int);
    void newSrcwhs(int);
    void newDstwhs(int);
    void valid(bool);

  private slots:
    void sList();

  private:
    void constructor();

    WComboBox    *_dstwhs;
    QPushButton  *_list;
    WComboBox    *_srcwhs;
    ToLineEdit   *_toNumber;
};

#endif
