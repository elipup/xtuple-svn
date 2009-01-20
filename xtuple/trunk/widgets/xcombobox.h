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

#ifndef xcombobox_h
#define xcombobox_h

#include <QComboBox>
#include <QLabel>
#include <QMouseEvent>
#include <QList>
#include <QSqlTableModel>

#include "widgets.h"
#include "xdatawidgetmapper.h"

class XSqlQuery;

class XTUPLEWIDGETS_EXPORT XComboBox : public QComboBox
{
  Q_OBJECT

  Q_ENUMS(Defaults)
  Q_ENUMS(XComboBoxTypes)

  Q_PROPERTY(bool           allowNull             READ allowNull            WRITE setAllowNull                            )
  Q_PROPERTY(QString        nullStr               READ nullStr              WRITE setNullStr                              )
  Q_PROPERTY(XComboBoxTypes type                  READ type                 WRITE setType                                 )
  Q_PROPERTY(QString        code                  READ code                 WRITE setCode                 DESIGNABLE false)
  Q_PROPERTY(Defaults       defaultCode           READ defaultCode          WRITE setDefaultCode                          )
  Q_PROPERTY(QString        fieldName             READ fieldName            WRITE setFieldName                            )
  Q_PROPERTY(QString        listSchemaName        READ listSchemaName       WRITE setListSchemaName                       )
  Q_PROPERTY(QString        listTableName         READ listTableName        WRITE setListTableName                        )
  Q_PROPERTY(QString        listIdFieldName       READ listIdFieldName      WRITE setListIdFieldName                      )
  Q_PROPERTY(QString        listDisplayFieldName  READ listDisplayFieldName WRITE setListDisplayFieldName                 )
  Q_PROPERTY(QString        currentDefault        READ currentDefault                                     DESIGNABLE false)
  Q_PROPERTY(QString        text                  READ currentText          WRITE setText                 DESIGNABLE false)

  public:
    XComboBox(QWidget * = 0, const char * = 0);
    XComboBox(bool, QWidget * = 0, const char * = 0);

    enum Defaults { First, None };
    enum XComboBoxTypes
      {
      Adhoc,
      APBankAccounts,	APTerms,		ARBankAccounts,
      ARTerms,		AccountingPeriods, 	Agent,
      AllCommentTypes,	AllProjects,		CRMAccounts,
      ClassCodes,	Companies,		CostCategories,
      Countries,
      Currencies,	CurrenciesNotBase,	CustomerCommentTypes,
      CustomerGroups,	CustomerTypes,		ExpenseCategories,
      FinancialLayouts,	FiscalYears, FreightClasses,		Honorifics,
      IncidentCategory,
      IncidentPriority,	IncidentResolution,	IncidentSeverity,
      ItemCommentTypes,	ItemGroups,             Locales,
      LocaleCountries,  LocaleLanguages,        LotSerialCommentTypes,
      OpportunityStages, OpportunitySources,    OpportunityTypes,
      PlannerCodes,	PoProjects,		ProductCategories,
      ProfitCenters,	ProjectCommentTypes,
      ReasonCodes,	RegistrationTypes,      Reports,                
      SalesCategories,	SalesReps,		SalesRepsActive,
      ShipVias,		ShippingCharges,	ShippingForms,
      SiteTypes,    SoProjects,	Subaccounts,
      TaxAuths,		TaxCodes,
      TaxTypes,		Terms, 			UOMs,
      Users,		VendorCommentTypes,	VendorGroups,
      VendorTypes,	WoProjects,		WorkCenters
      };

    XComboBoxTypes type();
    void setType(XComboBoxTypes);

    void setCode(QString);
    

    virtual bool      allowNull()            const  { return _allowNull; };
    virtual Defaults  defaultCode()                 { return _default;};
    virtual void      setAllowNull(bool);
    virtual void      setNull();

    QString           nullStr()              const  { return _nullStr; };
    void              setNullStr(const QString &);

    bool              editable()             const;
    void              setEditable(bool);

    inline            QLabel* label()        const  { return _label; };
    void              setLabel(QLabel* pLab);

    bool              isValid()              const;
    int               id(int)                const;
    Q_INVOKABLE int               id()                   const;
    QString           code()                 const;
    QString           fieldName()            const  { return _fieldName;            };
    QString           listDisplayFieldName() const  { return _listDisplayFieldName; };
    QString           listIdFieldName()      const  { return _listIdFieldName;      };
    QString           listSchemaName()       const  { return _listSchemaName;       };
    QString           listTableName()        const  { return _listTableName;        };

    QSize             sizeHint()             const;

  public slots:
    void clear();
    void append(int, const QString &);
    void append(int, const QString &, const QString &);
    void populate(XSqlQuery &, int = -1);
    void populate(const QString &, int = -1);
    void populate();
    void setDataWidgetMap(XDataWidgetMapper* m);
    void setDefaultCode(Defaults p)                 { _default = p;               };
    void setFieldName(QString p)                    { _fieldName = p;             };
    void setListDisplayFieldName(QString p)         { _listDisplayFieldName = p;  };
    void setListIdFieldName(QString p)              { _listIdFieldName = p;       };
    void setListSchemaName(QString p);
    void setListTableName(QString p);
    void setId(int);
    void setText(QVariant &);
    void setText(const QString &);
    void setText(const QVariant &);
    void updateMapperData();

  private slots:
    void sHandleNewIndex(int);

  signals:
    void clicked();
    void newID(int);
    void notNull(bool);
    void valid(bool);

  protected:
    QString   currentDefault();
    void      mousePressEvent(QMouseEvent *);

    bool                _allowNull;
    int                 _lastId;
    enum XComboBoxTypes _type;
    QLabel*             _label;
    QList<int>          _ids;
    QList<QString>      _codes;
    QString             _nullStr;
    
  private:
    enum Defaults       _default;
    QString             _fieldName;
    QString             _listDisplayFieldName;
    QString             _listIdFieldName;
    QString             _listSchemaName;
    QString             _listTableName;
    XDataWidgetMapper   *_mapper;
};

#endif

