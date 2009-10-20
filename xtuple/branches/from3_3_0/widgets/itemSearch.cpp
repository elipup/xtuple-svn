/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */


#include "itemSearch.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

#include <parameter.h>
#include <xsqlquery.h>

#include "itemcluster.h"
#include "xtreewidget.h"

QString buildItemLineEditQuery(const QString, const QStringList, const QString, const unsigned int);
QString buildItemLineEditTitle(const unsigned int, const QString);

itemSearch::itemSearch( QWidget* parent, const char* name, bool modal, Qt::WFlags fl ) : QDialog( parent, name, modal, fl )
{
    if ( !name )
      setObjectName( "itemSearch" );

  _itemid = -1;
  _itemType = ItemLineEdit::cUndefined;
  _useQuery = FALSE;

    setWindowTitle( tr( "Search for Item" ) );

    QVBoxLayout *itemSearchLayout = new QVBoxLayout( this, 5, 5, "itemSearchLayout"); 
    QHBoxLayout *Layout72 = new QHBoxLayout( 0, 0, 7, "Layout72"); 
    QVBoxLayout *Layout71 = new QVBoxLayout( 0, 0, 5, "Layout71"); 
    QHBoxLayout *Layout70 = new QHBoxLayout( 0, 0, 5, "Layout70"); 
    QHBoxLayout *Layout66 = new QHBoxLayout( 0, 0, 7, "Layout66"); 
    QVBoxLayout *Layout64 = new QVBoxLayout( 0, 0, 1, "Layout64"); 
    QVBoxLayout *Layout67 = new QVBoxLayout( 0, 0, 0, "Layout67"); 
    QVBoxLayout *Layout18 = new QVBoxLayout( 0, 0, 5, "Layout18"); 
    QVBoxLayout *Layout65 = new QVBoxLayout( 0, 0, 0, "Layout65"); 
    QVBoxLayout *Layout20 = new QVBoxLayout( 0, 0, 0, "Layout20"); 

    QLabel *_searchLit = new QLabel(tr("S&earch for:"), this, "_searchLit");
    _searchLit->setAlignment( int( Qt::AlignVCenter | Qt::AlignRight ) );
    Layout70->addWidget( _searchLit );

    _search = new QLineEdit( this, "_search" );
    _searchLit->setBuddy( _search );
    Layout70->addWidget( _search );
    Layout71->addLayout( Layout70 );

    _searchNumber = new XCheckBox(tr("Search through Item Numbers"));
    _searchNumber->setChecked( TRUE );
    _searchNumber->setObjectName("_searchNumber");
    Layout64->addWidget( _searchNumber );

    _searchDescrip1 = new XCheckBox(tr("Search through Description 1"));
    _searchDescrip1->setChecked( TRUE );
    _searchDescrip1->setObjectName("searchDescrip1");
    Layout64->addWidget( _searchDescrip1 );

    _searchDescrip2 = new XCheckBox(tr("Search through Description 2"));
    _searchDescrip2->setChecked( TRUE );
    _searchDescrip2->setObjectName("_searchDescrip2");
    Layout64->addWidget( _searchDescrip2 );
    
    _searchUpc = new XCheckBox(tr("Search through UPC Code"));
    _searchUpc->setChecked( TRUE );
    _searchUpc->setObjectName("_searchUpc");
    Layout64->addWidget( _searchUpc );
    Layout66->addLayout( Layout64 );

    _showInactive = new XCheckBox(tr("Show &Inactive Items"));
    _showInactive->setObjectName("_showInactive");
    Layout65->addWidget( _showInactive );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Preferred );
    Layout65->addItem( spacer );
    Layout66->addLayout( Layout65 );
    Layout71->addLayout( Layout66 );
    Layout72->addLayout( Layout71 );

    _close = new QPushButton(tr("&Cancel"), this, "_close");
    Layout18->addWidget( _close );

    _select = new QPushButton(tr("&Select"), this, "_select");
    _select->setEnabled( FALSE );
    _select->setAutoDefault( TRUE );
    _select->setDefault( TRUE );
    Layout18->addWidget( _select );

    Layout67->addLayout( Layout18 );
    QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Preferred );
    Layout67->addItem( spacer_2 );
    Layout72->addLayout( Layout67 );
    itemSearchLayout->addLayout( Layout72 );

    QLabel *_itemsLit = new QLabel(tr("&Items:"), this, "_itemsLit");
    Layout20->addWidget( _itemsLit );

    _item = new XTreeWidget(this);
    _item->setObjectName("_item");
    _itemsLit->setBuddy( _item );
    Layout20->addWidget( _item );

    itemSearchLayout->addLayout( Layout20 );
    resize( QSize(462, 391).expandedTo(minimumSizeHint()) );
    //clearWState( WState_Polished );

    // signals and slots connections
    connect( _item, SIGNAL( itemSelected(int) ), this, SLOT( sSelect() ) );
    connect( _select, SIGNAL( clicked() ), this, SLOT( sSelect() ) );
    connect( _close, SIGNAL( clicked() ), this, SLOT( sClose() ) );
    connect( _showInactive, SIGNAL( clicked() ), this, SLOT( sFillList() ) );
    connect( _searchNumber, SIGNAL( toggled(bool) ), this, SLOT( sFillList() ) );
    connect( _searchDescrip1, SIGNAL( toggled(bool) ), this, SLOT( sFillList() ) );
    connect( _searchDescrip2, SIGNAL( toggled(bool) ), this, SLOT( sFillList() ) );
    connect( _searchUpc, SIGNAL( toggled(bool) ), this, SLOT( sFillList() ) );
    connect( _search, SIGNAL( lostFocus() ), this, SLOT( sFillList() ) );
    connect( _item, SIGNAL( valid(bool) ), _select, SLOT( setEnabled(bool) ) );

  _item->addColumn(tr("Item Number"), 100,  Qt::AlignLeft );
  _item->addColumn(tr("Description"), -1,   Qt::AlignLeft );
  _item->addColumn(tr("UPC Code"),    100,  Qt::AlignLeft );
}

void itemSearch::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
    _itemid = param.toInt();
  else
    _itemid = -1;

  param = pParams.value("sql", &valid);
  if (valid)
  {
    _sql = param.toString();
    _useQuery = TRUE;
  }
  else
    _useQuery = FALSE;

  param = pParams.value("itemType", &valid);
  if (valid)
  {
    _itemType = param.toUInt();
    setWindowTitle(buildItemLineEditTitle(_itemType, tr("Items")));
  }
  else
    _itemType = ItemLineEdit::cUndefined;

  param = pParams.value("extraClauses", &valid);
  if (valid)
    _extraClauses = param.toStringList();

  _showInactive->setChecked(FALSE);
  _showInactive->setEnabled(!(_itemType & ItemLineEdit::cActive));

  param = pParams.value("caption", &valid);
  if (valid)
    setWindowTitle(param.toString());
    
  param = pParams.value("search", &valid);
  if (valid)
    _search->setText(param.toString());
    
  param = pParams.value("searchNumber", &valid);
  if (valid)
    _searchNumber->setChecked(true);
    
  param = pParams.value("searchUpc", &valid);
  if (valid)
    _searchUpc->setChecked(true);

  sFillList();
}

void itemSearch::sClose()
{
  done(_itemid);
}

void itemSearch::sSelect()
{
  done(_item->id());
}

void itemSearch::sFillList()
{
  _search->setText(_search->text().trimmed().toUpper());
  if (_search->text().length() == 0)
    return;

  QString sql;

  if (_useQuery)
  {
    QStringList clauses;
    if (!_showInactive->isChecked())
      clauses << "(item_active)";

    QStringList subClauses;
    if (_searchNumber->isChecked())
      subClauses << "(item_number ~* :searchString)";

    if (_searchDescrip1->isChecked())
      subClauses << "(item_descrip1 ~* :searchString)";

    if (_searchDescrip2->isChecked())
      subClauses << "(item_descrip2 ~* :searchString)";
      
    if (_searchUpc->isChecked())
      subClauses << "(item_upccode ~* :searchString)";

    if(!subClauses.isEmpty())
      clauses << QString("( " + subClauses.join(" OR ") + " )");

    sql = "SELECT * FROM (" + _sql + ") AS dummy WHERE (" +
          clauses.join(" AND ") + ");" ;
  }
  else
  { 
    if ( (!_searchNumber->isChecked()) &&
         (!_searchDescrip1->isChecked()) &&
         (!_searchDescrip2->isChecked()) &&
         (!_searchUpc->isChecked()) )
    {
      _item->clear();
      return;
    }

    QString pre;
    QString post;
    if(_x_preferences && _x_preferences->boolean("ListNumericItemNumbersFirst"))
    {
      pre =  "SELECT DISTINCT ON (toNumeric(item_number, 999999999999999), item_number) item_id, item_number, (item_descrip1 || ' ' || item_descrip2), item_upccode ";
      post = "ORDER BY toNumeric(item_number, 999999999999999), item_number";
    }
    else
    {
      pre =  "SELECT DISTINCT item_id, item_number, (item_descrip1 || ' ' || item_descrip2), item_upccode ";
      post = "ORDER BY item_number";
    }

    QStringList clauses;
    clauses = _extraClauses;
    if(!(_itemType & ItemLineEdit::cActive) && !_showInactive->isChecked())
      clauses << "(item_active)";

    QStringList subClauses;
    if (_searchNumber->isChecked())
      subClauses << "(item_number ~* :searchString)";

    if (_searchDescrip1->isChecked())
      subClauses << "(item_descrip1 ~* :searchString)";

    if (_searchDescrip2->isChecked())
      subClauses << "(item_descrip2 ~* :searchString)";
      
    if (_searchUpc->isChecked())
      subClauses << "(item_upccode ~* :searchString)";

    if(!subClauses.isEmpty())
      clauses << QString("( " + subClauses.join(" OR ") + " )");

    sql = buildItemLineEditQuery(pre, clauses, post, _itemType);
  }

  XSqlQuery search;
  search.prepare(sql);
  search.bindValue(":searchString", _search->text());
  search.exec();
  _item->populate(search, _itemid);
}
