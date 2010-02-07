/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include <QMessageBox>
#include <QDropEvent>
#include <QStandardItemEditorCreator>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QApplication>

#include <xsqlquery.h>

#include "itemcluster.h"
#include "itemAliasList.h"
#include "xsqltablemodel.h"

QString buildItemLineEditQuery(const QString, const QStringList, const QString, const unsigned int);
QString buildItemLineEditTitle(const unsigned int, const QString);

QString buildItemLineEditQuery(const QString pPre, const QStringList pClauses, const QString pPost, const unsigned int pType)
{
  QStringList clauses = pClauses;
  QString sql = pPre + " FROM item, uom";
  clauses << "(item_inv_uom_id=uom_id)";

  if (pType & (ItemLineEdit::cLocationControlled | ItemLineEdit::cLotSerialControlled | ItemLineEdit::cDefaultLocation | ItemLineEdit::cActive))
  {
    sql += ", itemsite";
    clauses << "(itemsite_item_id=item_id)";
  }

  if (pType & ItemLineEdit::cAllItemTypes_Mask)
  {
    QStringList types;

    if (pType & ItemLineEdit::cPurchased)
      types << "'P'";

    if (pType & ItemLineEdit::cManufactured)
      types << "'M'";

    if (pType & ItemLineEdit::cPhantom)
      types << "'F'";

    if (pType & ItemLineEdit::cBreeder)
      types << "'B'";

    if (pType & ItemLineEdit::cCoProduct)
      types << "'C'";

    if (pType & ItemLineEdit::cByProduct)
      types << "'Y'";

    if (pType & ItemLineEdit::cReference)
      types << "'R'";

    if (pType & ItemLineEdit::cCosting)
      types << "'S'";

    if (pType & ItemLineEdit::cTooling)
      types << "'T'";

    if (pType & ItemLineEdit::cOutsideProcess)
      types << "'O'";

    if (pType & ItemLineEdit::cPlanning)
      types << "'L'";

    if (pType & ItemLineEdit::cKit)
      types << "'K'";

    if (!types.isEmpty())
      clauses << QString("(item_type IN (" + types.join(",") + "))");
  }

  if (pType & (ItemLineEdit::cLocationControlled | ItemLineEdit::cLotSerialControlled | ItemLineEdit::cDefaultLocation))
  {
    QString sub;
    if (pType & ItemLineEdit::cLocationControlled)
      sub = "(itemsite_loccntrl)";

    if (pType & ItemLineEdit::cLotSerialControlled)
    {
      if (!sub.isEmpty())
        sub += " OR ";
      sub += "(itemsite_controlmethod IN ('L', 'S'))";
    }

    if (pType & ItemLineEdit::cDefaultLocation)
    {
      if (!sub.isEmpty())
        sub += " OR ";
      sub += "(useDefaultLocation(itemsite_id))";
    }

    clauses << QString("( " + sub + " )");
  }

  if (pType & ItemLineEdit::cSold)
    clauses << "(item_sold)";

  if (pType & (ItemLineEdit::cActive | ItemLineEdit::cItemActive))
    clauses << "(item_active)";

  if (!clauses.isEmpty())
    sql += " WHERE (" + clauses.join(" AND ") + ")";

  if (!pPost.isEmpty())
    sql += " " + pPost;

  sql += ";";

  return sql;
}

QString buildItemLineEditTitle(const unsigned int pType, const QString pPost)
{
  QString caption;
  unsigned int items = (0xFFFF & pType); // mask so this value only has the individual items

  if (pType & ItemLineEdit::cSold)
    caption += ItemLineEdit::tr("Sold ");

  if (pType & ItemLineEdit::cActive)
    caption = ItemLineEdit::tr("Active ");

  if (pType & ItemLineEdit::cLocationControlled)
    caption += ItemLineEdit::tr("Multiply Located ");
  else if (pType & ItemLineEdit::cLotSerialControlled)
    caption += ItemLineEdit::tr("Lot/Serial # Controlled ");

  else if (items == (ItemLineEdit::cGeneralPurchased | ItemLineEdit::cGeneralManufactured))
    caption += ItemLineEdit::tr("Purchased and Manufactured ");
  else if ((items == ItemLineEdit::cGeneralPurchased) || (items == ItemLineEdit::cPurchased) )
    caption += ItemLineEdit::tr("Purchased ");
  else if ( (items == ItemLineEdit::cGeneralManufactured) || (items == ItemLineEdit::cManufactured) )
    caption += ItemLineEdit::tr("Manufactured ");
  else if (items == ItemLineEdit::cBreeder)
    caption += ItemLineEdit::tr("Breeder ");
  else if (items == (ItemLineEdit::cCoProduct | ItemLineEdit::cByProduct))
    caption += ItemLineEdit::tr("Co-Product and By-Product ");
  else if (items == ItemLineEdit::cCosting)
    caption += ItemLineEdit::tr("Costing ");
  else if (items == ItemLineEdit::cGeneralComponents)
    caption += ItemLineEdit::tr("Component ");
  else if (items == ItemLineEdit::cKitComponents)
    caption += ItemLineEdit::tr("Kit Components ");

  if(!pPost.isEmpty())
    caption += pPost;

  return caption;
}


ItemLineEditDelegate::ItemLineEditDelegate(QObject *parent)
  : QItemDelegate(parent)
{
  _template = qobject_cast<ItemLineEdit *>(parent);
}

ItemLineEditDelegate::~ItemLineEditDelegate()
{
}

QWidget *ItemLineEditDelegate::createEditor(QWidget *parent,
                                            const QStyleOptionViewItem &/*option*/,
                                            const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    ItemLineEdit *widget = new ItemLineEdit(parent);
    // Make editor share certain properties with template
    widget->setType(_template->type());
    QStringList clauses = _template->getExtraClauseList();
    for (int i = 0; i < clauses.size(); i++)
        widget->addExtraClause(clauses.at(i));

    return widget;
}

void ItemLineEditDelegate::setEditorData(QWidget *editor,
                                           const QModelIndex &index) const
{
  const XSqlTableModel *model = qobject_cast<const XSqlTableModel *>(index.model());
  if (model) {
    ItemLineEdit *widget = qobject_cast<ItemLineEdit *>(editor);
    if (widget)
      widget->setItemNumber(model->data(index).toString());
  }
}

void ItemLineEditDelegate::setModelData(QWidget *editor,
                                          QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
  if (!index.isValid())
    return;

//  XSqlTableModel *sqlModel = qobject_cast<XSqlTableModel *>(model);
  if (model) {
    ItemLineEdit *widget = qobject_cast<ItemLineEdit *>(editor);
    if (widget)
      model->setData(index, widget->itemNumber(), Qt::EditRole);
  }
}
ItemLineEdit::ItemLineEdit(QWidget* pParent, const char* pName) :
    VirtualClusterLineEdit(pParent, "item", "item_id", "item_number", "(item_descrip1 || ' ' || item_descrip2) ", "item_upccode", 0, pName)
{
  setTitles(tr("Item"), tr("Items"));
  /*
  setUiName("item");
  setEditPriv("MaintainItems");
  setViewPriv("ViewItems");
 */
  setAcceptDrops(TRUE);
  
  _type = cUndefined;
  _defaultType = cUndefined;
  _useQuery = FALSE;
  _useValidationQuery = FALSE;
  _itemNumber = "";
  _uom = "";
  _itemType = "";
  _id = -1;
  _configured = FALSE;
  _delegate = new ItemLineEditDelegate(this);

  connect(this, SIGNAL(requestAlias()), this, SLOT(sAlias()));
  connect(_aliasAct, SIGNAL(triggered()), this, SLOT(sAlias()));

  // Remove info and separator since not implemented here for now
  if (_x_preferences)
  {
    if (!_x_preferences->boolean("ClusterButtons"))
    {
      menu()->removeAction(_infoAct);
      menu()->removeAction(menu()->actions().at(2));
      menu()->addAction(_aliasAct);
    }
  }
}

void ItemLineEdit::setItemNumber(const QString& pNumber)
{
  XSqlQuery item;
  bool      found = FALSE;

  _parsed = TRUE;

  if (pNumber == text())
    return;

  if (!pNumber.isEmpty())
  {
    if (_useValidationQuery)
    {
      item.prepare(_validationSql);
      item.bindValue(":item_number", pNumber);
      item.exec();
      if (item.first())
        found = TRUE;
    }
    else if (_useQuery)
    {
      item.prepare(_sql);
      item.exec();
      found = (item.findFirst("item_number", pNumber) != -1);
    }
    else if (pNumber != QString::Null())
    {
      QString pre( "SELECT DISTINCT item_id, item_number, item_descrip1, item_descrip2,"
                   "                uom_name, item_type, item_config, item_upccode");

      QStringList clauses;
      clauses = _extraClauses;
      clauses << "(item_number=:item_number OR item_upccode=:item_number)";

      item.prepare(buildItemLineEditQuery(pre, clauses, QString::null, _type));
      item.bindValue(":item_number", pNumber);
      item.exec();
      
      if (item.size() > 1)
      { 
        ParameterList params;
        params.append("search", pNumber);
        params.append("searchNumber");
        params.append("searchUpc");
        sSearch(params);
        return;
      }
      else
        found = item.first();
    }
  }
  if (found)
  {
    _itemNumber = pNumber;
    _uom        = item.value("uom_name").toString();
    _itemType   = item.value("item_type").toString();
    _configured = item.value("item_config").toBool();
    _id         = item.value("item_id").toInt();
    _upc        = item.value("item_upccode").toInt();
    _valid      = TRUE;

    setText(item.value("item_number").toString());

    emit aliasChanged("");
    emit typeChanged(_itemType);
    emit descrip1Changed(item.value("item_descrip1").toString());
    emit descrip2Changed(item.value("item_descrip2").toString());
    emit uomChanged(item.value("uom_name").toString());
    emit configured(item.value("item_config").toBool());
    emit upcChanged(item.value("item_upccode").toString());
    
    emit valid(TRUE);
  }
  else
  {
    _itemNumber = "";
    _uom        = "";
    _itemType   = "";
    _id         = -1;
    _valid      = FALSE;
    _upc        = "";

    setText("");

    emit aliasChanged("");
    emit typeChanged("");
    emit descrip1Changed("");
    emit descrip2Changed("");
    emit uomChanged("");
    emit configured(FALSE);
    emit upcChanged("");

    emit valid(FALSE);
  }
}

void ItemLineEdit::silentSetId(int pId)
{
  XSqlQuery item;
  bool      found = FALSE;

  _parsed = TRUE;

  if (_useValidationQuery)
  {
    item.prepare(_validationSql);
    item.bindValue(":item_id", pId);
    item.exec();
    if (item.first())
      found = TRUE;
  }
  else if (_useQuery)
  {
    item.prepare(_sql);
    item.exec();
    found = (item.findFirst("item_id", pId) != -1);
  }
  else if (pId != -1)
  {
    QString pre( "SELECT DISTINCT item_number, item_descrip1, item_descrip2,"
                 "                uom_name, item_type, item_config, item_upccode");

    QStringList clauses;
    clauses = _extraClauses;
    clauses << "(item_id=:item_id)";

    item.prepare(buildItemLineEditQuery(pre, clauses, QString::null, _type));
    item.bindValue(":item_id", pId);
    item.exec();

    found = item.first();
  }

  if (found)
  {
    if (completer())
    {
      disconnect(this, SIGNAL(textChanged(QString)), this, SLOT(sHandleCompleter()));
      static_cast<QSqlQueryModel* >(completer()->model())->setQuery(QSqlQuery());
    }

    _itemNumber = item.value("item_number").toString();
    _uom        = item.value("uom_name").toString();
    _itemType   = item.value("item_type").toString();
    _configured = item.value("item_config").toBool();
    _upc        = item.value("item_upccode").toString();
    _id         = pId;
    _valid      = TRUE;

    setText(item.value("item_number").toString());
    emit aliasChanged("");
    emit typeChanged(_itemType);
    emit descrip1Changed(item.value("item_descrip1").toString());
    emit descrip2Changed(item.value("item_descrip2").toString());
    emit uomChanged(item.value("uom_name").toString());
    emit configured(item.value("item_config").toBool());
    emit upcChanged(item.value("item_upccode").toString());
    
    emit valid(TRUE);

    if (completer())
      connect(this, SIGNAL(textChanged(QString)), this, SLOT(sHandleCompleter()));
  }
  else
  {
    _itemNumber = "";
    _uom        = "";
    _itemType   = "";
    _id         = -1;
    _upc        = "";
    _valid      = FALSE;

    setText("");

    emit aliasChanged("");
    emit typeChanged("");
    emit descrip1Changed("");
    emit descrip2Changed("");
    emit uomChanged("");
    emit configured(FALSE);
    emit upcChanged("");

    emit valid(FALSE);
  }
} 

void ItemLineEdit::setId(int pId)
{
  if (pId != _id)
  {
    silentSetId(pId);
    emit privateIdChanged(_id);
    emit newId(_id);
  }
}

void ItemLineEdit::setItemsiteid(int pItemsiteid)
{
  XSqlQuery itemsite;
  itemsite.prepare( "SELECT itemsite_item_id, itemsite_warehous_id "
                    "FROM itemsite "
                    "WHERE (itemsite_id=:itemsite_id);" );
  itemsite.bindValue(":itemsite_id", pItemsiteid);
  itemsite.exec();
  if (itemsite.first())
  {
    setId(itemsite.value("itemsite_item_id").toInt());
    emit warehouseIdChanged(itemsite.value("itemsite_warehous_id").toInt());
  }
  else
  {
    setId(-1);
    emit warehouseIdChanged(-1);
  }
}

void ItemLineEdit::sList()
{
  ParameterList params;
  params.append("item_id", _id);

  if (queryUsed())
    params.append("sql", _sql);
  else
    params.append("itemType", _defaultType);

  if (!_extraClauses.isEmpty())
    params.append("extraClauses", _extraClauses);

  itemList* newdlg = listFactory();
  newdlg->set(params);

  int id;
  if ((id = newdlg->exec())!= _id)
    setId(id);
}

void ItemLineEdit::sSearch()
{
  ParameterList params;
  sSearch(params);
}

void ItemLineEdit::sSearch(ParameterList params)
{
  params.append("item_id", _id);

  if (queryUsed())
    params.append("sql", _sql);
  else
    params.append("itemType", _type);

  if (!_extraClauses.isEmpty())
    params.append("extraClauses", _extraClauses);

  itemSearch* newdlg = searchFactory();
  newdlg->setSearchText(text());
  newdlg->set(params);
  int id;
  if ((id = newdlg->exec()) != QDialog::Rejected)
    setId(id);
}

itemList* ItemLineEdit::listFactory()
{
  return new itemList(this);
}

itemSearch* ItemLineEdit::searchFactory()
{
  return new itemSearch(this);
}

void ItemLineEdit::sAlias()
{
  ParameterList params;

  if (queryUsed())
    params.append("sql", _sql);
  else
    params.append("itemType", _type);

  if (!_extraClauses.isEmpty())
    params.append("extraClauses", _extraClauses);

  itemAliasList newdlg(parentWidget(), "", TRUE);
  newdlg.set(params);

  int itemaliasid;
  if ((itemaliasid = newdlg.exec()) != QDialog::Rejected)
  {
    XSqlQuery itemalias;
    itemalias.prepare( "SELECT itemalias_number, itemalias_item_id "
                       "FROM itemalias "
                       "WHERE (itemalias_id=:itemalias_id);" );
    itemalias.bindValue(":itemalias_id", itemaliasid);
    itemalias.exec();
    if (itemalias.first())
    {
      setId(itemalias.value("itemalias_item_id").toInt());
      emit aliasChanged(itemalias.value("itemalias_number").toString());
      focusNextPrevChild(TRUE);
    }
  }
}

QString ItemLineEdit::itemNumber()
{
  sParse();
  return _itemNumber;
}

QString ItemLineEdit::uom()
{
  sParse();
  return _uom;
}

QString ItemLineEdit::upc()
{
  sParse();
  return _upc;
}

QString ItemLineEdit::itemType()
{
  sParse();
  return _itemType;
}

bool ItemLineEdit::isConfigured()
{
  sParse();
  return _configured;
}

void ItemLineEdit::mousePressEvent(QMouseEvent *pEvent)
{
  dragStartPosition = pEvent->pos();
  QLineEdit::mousePressEvent(pEvent);
}

void ItemLineEdit::mouseMoveEvent(QMouseEvent * event)
{
  if (!_valid)
    return;
  if (!(event->buttons() & Qt::LeftButton))
    return;
  if ((event->pos() - dragStartPosition).manhattanLength()
        < QApplication::startDragDistance())
    return;

  QString dragData = QString("itemid=%1").arg(_id);
  
  QDrag *drag = new QDrag(this);
  QMimeData * mimeData = new QMimeData;

  mimeData->setText(dragData);
  drag->setMimeData(mimeData);

  /*Qt::DropAction dropAction =*/ drag->start(Qt::CopyAction);
}

void ItemLineEdit::dragEnterEvent(QDragEnterEvent *pEvent)
{
  const QMimeData * mimeData = pEvent->mimeData();
  if (mimeData->hasFormat("text/plain"))
  {
    if (mimeData->text().contains("itemid=") || mimeData->text().contains("itemsiteid="))
      pEvent->acceptProposedAction();
  }
}

void ItemLineEdit::dropEvent(QDropEvent *pEvent)
{
  QString dropData;

  if (pEvent->mimeData()->hasFormat("text/plain"))
  {
    dropData = pEvent->mimeData()->text();
    if (dropData.contains("itemid="))
    {
      QString target = dropData.mid((dropData.find("itemid=") + 7), (dropData.length() - 7));

      if (target.contains(","))
        target = target.left(target.find(","));

      pEvent->acceptProposedAction();
      pEvent->accept();
      setId(target.toInt());
    }
    else if (dropData.contains("itemsiteid="))
    {
      QString target = dropData.mid((dropData.find("itemsiteid=") + 11), (dropData.length() - 11));

      if (target.contains(","))
        target = target.left(target.find(","));

      pEvent->acceptProposedAction();
      pEvent->accept();
      setItemsiteid(target.toInt());
    }
  }
}


void ItemLineEdit::sParse()
{
  if (!_parsed)
  {
    _parsed = TRUE;

    if (text().length() == 0)
    {
      setId(-1);
      return;
    }

    else if (_useValidationQuery)
    {
      XSqlQuery item;
      item.prepare("SELECT item_id FROM item WHERE (item_number = :searchString OR item_upccode = :searchString);");
      item.bindValue(":searchString", text().trimmed().toUpper());
      item.exec();
      if (item.first())
      {
        int itemid = item.value("item_id").toInt();
        item.prepare(_validationSql);
        item.bindValue(":item_id", itemid);
        item.exec();
        if (item.size() > 1)
        {
          ParameterList params;
          params.append("search", text().trimmed().toUpper());
          params.append("searchNumber");
          params.append("searchUpc");
          sSearch(params);
          return;
        }
        else if (item.first())
        {
          setId(itemid);
          return;
        }
      }
    }
    else if (_useQuery)
    {
      XSqlQuery item;
      item.prepare(_sql);
      item.exec();
      if (item.findFirst("item_number", text().trimmed().toUpper()) != -1)
      {
        setId(item.value("item_id").toInt());
        return;
      }
    }
    else
    {
      XSqlQuery item;

      QString pre( "SELECT DISTINCT item_id, item_number AS number, "
                   "(item_descrip1 || ' ' || item_descrip2) AS name, "
                   "item_upccode AS description " );

      QStringList clauses;
      clauses = _extraClauses;
      clauses << "(item_number ~* :searchString OR item_upccode ~* :searchString)";
      item.prepare(buildItemLineEditQuery(pre, clauses, QString::null, _type));
      item.bindValue(":searchString", QString(text().trimmed().toUpper()).prepend("^"));
      item.exec();
      if (item.size() > 1)
      {
        itemSearch* newdlg = searchFactory();
        if (newdlg)
        {
          newdlg->setQuery(item);
          newdlg->setSearchText(text());
          int oldid = _id;
          int newid = newdlg->exec();
          silentSetId(newid); //Force refresh
          if (newid != oldid)
          {
            emit newId(newid);
            emit privateIdChanged(newid);
          }
          return;
        }
      }
      else if (item.first())
      {
        setId(item.value("item_id").toInt());
        return;
      }
    }
    setId(-1);
  }
}

void ItemLineEdit::addExtraClause(const QString & pClause)
{
  _extraClauses << pClause;
}

ItemCluster::ItemCluster(QWidget* pParent, const char* pName) :
    VirtualCluster(pParent, pName)
{
  setObjectName(pName);
  addNumberWidget(new ItemLineEdit(this, pName));
  _label->setText(tr("Item Number:"));

//  Create the component Widgets
  QLabel *_uomLit = new QLabel(tr("UOM:"), this, "_uomLit");
  _uomLit->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
  _uom = new QLabel(this, "_uom");
  _uom->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  _uom->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  _uom->setMinimumWidth(50);
  if (_x_preferences)
  {
    if (_x_preferences->boolean("ClusterButtons"))
    {
      _info->hide();
      _grid->addWidget(_uomLit, 0, 3);
      _grid->addWidget(_uom, 0, 4);
    }
    else
    {
      _grid->addWidget(_uomLit, 0, 2);
      _grid->addWidget(_uom, 0, 3);
    }
  }
  else
  {
    _uomLit->hide();
    _uom->hide();
  }


  _descrip2 = new QLabel(this, "_descrip2");
  _descrip2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  _grid->addWidget(_descrip2, 3, 1, 1, -1);
  //setLayout(mainLayout);

//  Make some internal connections
  ItemLineEdit* itemNumber = static_cast<ItemLineEdit* >(_number);
  connect(itemNumber, SIGNAL(aliasChanged(const QString &)), this, SIGNAL(aliasChanged(const QString &)));
  connect(itemNumber, SIGNAL(privateIdChanged(int)), this, SIGNAL(privateIdChanged(int)));
  connect(itemNumber, SIGNAL(newId(int)), this, SIGNAL(newId(int)));
  connect(itemNumber, SIGNAL(uomChanged(const QString &)), this, SIGNAL(uomChanged(const QString &)));
  connect(itemNumber, SIGNAL(descrip1Changed(const QString &)), this, SIGNAL(descrip1Changed(const QString &)));
  connect(itemNumber, SIGNAL(descrip2Changed(const QString &)), this, SIGNAL(descrip2Changed(const QString &)));
  connect(itemNumber, SIGNAL(valid(bool)), this, SIGNAL(valid(bool)));
  connect(itemNumber, SIGNAL(warehouseIdChanged(int)), this, SIGNAL(warehouseIdChanged(int)));
  connect(itemNumber, SIGNAL(typeChanged(const QString &)), this, SIGNAL(typeChanged(const QString &)));
  connect(itemNumber, SIGNAL(upcChanged(const QString &)), this, SIGNAL(upcChanged(const QString &)));
  connect(itemNumber, SIGNAL(configured(bool)), this, SIGNAL(configured(bool)));

  connect(itemNumber, SIGNAL(uomChanged(const QString &)), _uom, SLOT(setText(const QString &)));
  connect(itemNumber, SIGNAL(descrip1Changed(const QString &)), _description, SLOT(setText(const QString &)));
  connect(itemNumber, SIGNAL(descrip2Changed(const QString &)), _descrip2, SLOT(setText(const QString &)));
}

void ItemCluster::addNumberWidget(ItemLineEdit* pNumberWidget)
{
    _number = pNumberWidget;
    if (! _number)
      return;

    _grid->addWidget(_number, 0, 1);
    setFocusProxy(pNumberWidget);

    connect(_list,      SIGNAL(clicked()),      this, SLOT(sEllipses()));
    connect(_info,      SIGNAL(clicked()),      this, SLOT(sInfo()));
    connect(_number,	SIGNAL(newId(int)),	this,	 SIGNAL(newId(int)));
    connect(_number,	SIGNAL(parsed()), 	this, 	 SLOT(sRefresh()));
    connect(_number,	SIGNAL(valid(bool)),	this,	 SIGNAL(valid(bool)));
}

void ItemCluster::setReadOnly(bool pReadOnly)
{
  if (pReadOnly)
  {
    _number->setEnabled(FALSE);
    _list->hide();
  }
  else
  {
    _number->setEnabled(TRUE);
    if (_x_preferences)
      _list->setVisible(_x_preferences->boolean("ClusterButtons"));
  }
}

void ItemCluster::setEnabled(bool pEnabled)
{
  setReadOnly(!pEnabled);
}

void ItemCluster::setDisabled(bool pDisabled)
{
  setReadOnly(pDisabled);
}

void ItemCluster::setId(int pId)
{
  _number->setId(pId);
}

void ItemCluster::setItemNumber(QString pNumber)
{
  static_cast<ItemLineEdit* >(_number)->setItemNumber(pNumber);
}

void ItemCluster::setItemsiteid(int intPItemsiteid)
{
  static_cast<ItemLineEdit* >(_number)->setItemsiteid(intPItemsiteid);
}

////////////////////////////////////////////////////////////////////////

itemList::itemList(QWidget* pParent, Qt::WindowFlags pFlags ) :
    VirtualList(pParent, pFlags)
{
  setObjectName( "itemList" );
  setAttribute(Qt::WA_DeleteOnClose);
  setMinimumWidth(600);

  _itemid = -1;
  _itemType = ItemLineEdit::cUndefined;
  _useQuery = FALSE;

  setWindowTitle(tr("Item List"));

  _showInactive = new QCheckBox(tr("Show &Inactive Items"), this, "_showInactive");
  _dialogLyt->insertWidget(1, _showInactive );
  _showMake = new QCheckBox(tr("&Make Items Only"), this, "_showMake");
  _showBuy = new QCheckBox(tr("&Buy Items Only"), this, "_showBuy");
  _dialogLyt->insertWidget(2, _showMake );
  _dialogLyt->insertWidget(3, _showBuy );

  connect( _showInactive, SIGNAL( clicked() ), this, SLOT( sFillList() ) );
  connect( _showMake, SIGNAL( clicked() ), this, SLOT( sFillList() ) );
  connect( _showBuy, SIGNAL( clicked() ), this, SLOT( sFillList() ) );

  _listTab->setColumnCount(0);
  _listTab->addColumn(tr("Item Number"), 100, Qt::AlignLeft, true);
  _listTab->addColumn(tr("Description"),  -1, Qt::AlignLeft, true);
  _listTab->addColumn(tr("Bar Code"),    100, Qt::AlignLeft, true);
}

void itemList::set(ParameterList &pParams)
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
    _showMake->setChecked(_itemType & ItemLineEdit::cGeneralManufactured);
    _showBuy->setChecked(_itemType & ItemLineEdit::cGeneralPurchased);
  }
  else
  {
    _itemType = ItemLineEdit::cUndefined;
    _showMake->hide();
    _showBuy->hide();
  }

  param = pParams.value("extraClauses", &valid);
  if (valid)
    _extraClauses = param.toStringList();

  _showInactive->setChecked(FALSE);
  _showInactive->setEnabled(!(_itemType & ItemLineEdit::cActive));
  if(!_showInactive->isEnabled())
    _showInactive->hide();

  param = pParams.value("caption", &valid);
  if (valid)
    setWindowTitle(param.toString());

  sFillList();
}

void itemList::sSearch(const QString &pTarget)
{
  if (_listTab->currentItem())
    _listTab->setCurrentItem(0);

  _listTab->clearSelection();

  int i;
  for (i = 0; i < _listTab->topLevelItemCount(); i++)
    if (_listTab->topLevelItem(i)->text(0).startsWith(pTarget, Qt::CaseInsensitive) ||
        _listTab->topLevelItem(i)->text(1).startsWith(pTarget, Qt::CaseInsensitive) ||
        _listTab->topLevelItem(i)->text(2).startsWith(pTarget, Qt::CaseInsensitive))
      break;

  if (i < _listTab->topLevelItemCount())
  {
    _listTab->setCurrentItem(_listTab->topLevelItem(i));
    _listTab->scrollToItem(_listTab->topLevelItem(i));
  }
}

void itemList::sFillList()
{
  if (_useQuery)
  {
    _listTab->populate(_sql, _itemid);
  }
  else
  {
      QString pre;
      QString post;
      if(_x_preferences && _x_preferences->boolean("ListNumericItemNumbersFirst"))
      {
        pre =  "SELECT DISTINCT ON (toNumeric(item_number, 999999999999999), item_number) item_id, item_number,"
               "(item_descrip1 || ' ' || item_descrip2) AS itemdescrip, item_upccode ";
        post = "ORDER BY toNumeric(item_number, 999999999999999), item_number, item_upccode ";
      }
      else
      {
        pre =  "SELECT DISTINCT item_id, item_number,"
               "(item_descrip1 || ' ' || item_descrip2) AS itemdescrip, item_upccode ";
        post = "ORDER BY item_number";
      }

      QStringList clauses;
      clauses = _extraClauses;
      if(!(_itemType & ItemLineEdit::cActive) && !_showInactive->isChecked())
        clauses << "(item_active)";

      if (_showMake->isChecked())
            _itemType = (_itemType | ItemLineEdit::cGeneralManufactured);
      else if (_itemType & ItemLineEdit::cGeneralManufactured)
            _itemType = (_itemType ^ ItemLineEdit::cGeneralManufactured);

      if (_showBuy->isChecked())
            _itemType = (_itemType | ItemLineEdit::cGeneralPurchased);
      else if (_itemType & ItemLineEdit::cGeneralPurchased)
            _itemType = (_itemType ^ ItemLineEdit::cGeneralPurchased);

      setWindowTitle(buildItemLineEditTitle(_itemType, tr("Items")));

      _listTab->populate(buildItemLineEditQuery(pre, clauses, post, _itemType), _itemid);
  }
}

void itemList::reject()
{
  done(_itemid);
}

//////////////////////////////////////////////////////////////////////

itemSearch::itemSearch(QWidget* pParent, Qt::WindowFlags pFlags)
    : VirtualSearch(pParent, pFlags)
{
  setAttribute(Qt::WA_DeleteOnClose);
  setObjectName( "itemSearch" );
  setMinimumWidth(800);

  _itemid = -1;
  _itemType = ItemLineEdit::cUndefined;
  _useQuery = FALSE;

  setWindowTitle( tr( "Search for Item" ) );

  _searchDescrip->setText(tr("Search through Description 1"));

  _searchDescrip2 = new XCheckBox(tr("Search through Description 2"));
  _searchDescrip2->setChecked( TRUE );
  _searchDescrip2->setObjectName("_searchDescrip2");
  selectorsLyt->addWidget( _searchDescrip2 );

  _searchUpc = new XCheckBox(tr("Search through Bar Code"));
  _searchUpc->setChecked( TRUE );
  _searchUpc->setObjectName("_searchUpc");
  selectorsLyt->addWidget(_searchUpc);

  _showInactive = new XCheckBox(tr("Show &Inactive Items"));
  _showInactive->setObjectName("_showInactive");
  selectorsLyt->addWidget(_showInactive);

  // signals and slots connections
  connect( _showInactive, SIGNAL( clicked() ), this, SLOT( sFillList() ) );
  connect( _searchDescrip2, SIGNAL( clicked() ), this, SLOT( sFillList() ) );
  connect( _searchUpc, SIGNAL( clicked() ), this, SLOT( sFillList() ) );

  _listTab->setColumnCount(0);
  _listTab->addColumn(tr("Item Number"), 100,  Qt::AlignLeft, true );
  _listTab->addColumn(tr("Description"), -1,   Qt::AlignLeft, true );
  _listTab->addColumn(tr("Bar Code"),    100,  Qt::AlignLeft, true );
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

    if (_searchDescrip->isChecked())
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
         (!_searchDescrip->isChecked()) &&
         (!_searchDescrip2->isChecked()) &&
         (!_searchUpc->isChecked()) )
    {
      _listTab->clear();
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
      pre =  "SELECT DISTINCT item_id, item_number AS number, "
             "(item_descrip1 || ' ' || item_descrip2) AS name, "
             "item_upccode AS description ";
      post = "ORDER BY item_number";
    }

    QStringList clauses;
    clauses = _extraClauses;
    if(!(_itemType & ItemLineEdit::cActive) && !_showInactive->isChecked())
      clauses << "(item_active)";

    QStringList subClauses;
    if (_searchNumber->isChecked())
      subClauses << "(item_number ~* :searchString)";

    if (_searchDescrip->isChecked())
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
  _listTab->populate(search, _itemid);
}
