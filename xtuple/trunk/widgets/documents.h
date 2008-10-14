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

#ifndef documents_h
#define documents_h

#include <QWidget>

#include <xsqlquery.h>

#include "ui_documents.h"

class OPENMFGWIDGETS_EXPORT Documents : public QWidget, public Ui::documents
{
  Q_OBJECT

  Q_ENUMS(DocumentSources)

  Q_PROPERTY(DocumentSources type READ type WRITE setType)

  friend class image;
  friend class file;

  public:
    Documents(QWidget *);

    // if you add to this then add to the _documentMap[] below
    enum DocumentSources
    {
      Uninitialized,
      Address,          BBOMHead,           BBOMItem,
      BOMHead,          BOMItem,            BOOHead,
      BOOItem,          CRMAccount,         Contact, 
      Customer,         Employee,           Incident,   
      Item,             ItemSite,           ItemSource,
      Location,		LotSerial,          Opportunity,
      Project,		PurchaseOrder,      PurchaseOrderItem,
      ReturnAuth,       ReturnAuthItem,     Quote, 
      QuoteItem,        SalesOrder,         SalesOrderItem,
      TransferOrder,	TransferOrderItem,  Vendor,
      Warehouse,	WorkOrder
    };

    inline int  sourceid()             { return _sourceid; }
    inline enum DocumentSources type() { return _source;   }

    struct DocumentMap
    {
      enum DocumentSources source;
      QString             ident;

      DocumentMap(enum DocumentSources s, const QString & i)
      {
        source = s;
        ident = i;
      }
    };
    static const struct DocumentMap _documentMap[]; // see Documents.cpp for init

  public slots:
    void setType(enum DocumentSources);
    void setId(int);
    void setReadOnly(bool);

    void sNewFile();
    void sEditFile();
    void sViewFile();
    void sDeleteFile();
    void sOpenFile();
    
    void sPrintImage();
    void sNewImage();
    void sEditImage();
    void sViewImage();
    void sDeleteImage();
    
    void refresh();

  private:
    enum DocumentSources _source;
    int                  _sourceid;

};

#endif
