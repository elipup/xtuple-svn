/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "xtreeview.h"
#include "xsqltablemodel.h"

#include <QBuffer>
#include <QHeaderView>
#include <QMessageBox>
#include <QModelIndex>
#include <QScriptEngine>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlIndex>
#include <QStack>
#include <QTreeWidgetItem>

#define DEBUG false

XTreeView::XTreeView(QWidget *parent) : 
  QTreeView(parent)
{
  _keyColumns        = 1;
  _resizingInProcess = false;
  _settingsLoaded    = false;
  _forgetful         = false;

  _menu = new QMenu(this);
  _menu->setObjectName("_menu");

  setContextMenuPolicy(Qt::CustomContextMenu);
  header()->setStretchLastSection(false);
  header()->setClickable(true);
  header()->setContextMenuPolicy(Qt::CustomContextMenu);
  
  setAlternatingRowColors(true);

  connect(header(), SIGNAL(customContextMenuRequested(const QPoint &)),
                    SLOT(sShowHeaderMenu(const QPoint &)));
  connect(header(), SIGNAL(sectionResized(int, int, int)),
          this,     SLOT(sColumnSizeChanged(int, int, int)));

  _mapper = new XDataWidgetMapper(this);
  _model.setEditStrategy(QSqlTableModel::OnManualSubmit);
}

XTreeView::~XTreeView()
{
  QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenMFG");
  settings.setValue(_settingsName + "/isForgetful", _forgetful);
  QString savedString;
  if(!_forgetful)
  {
    savedString = "";
    for(int i = 0; i < header()->count(); i++)
    {
      int w = -1;
      ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(i), 0);
      if (cp)
      {
        if (!cp->stretchy)
          w = cp->savedWidth;
        _columnByName.remove(cp->columnName);
        delete cp;
      }
      if (w >= 0 && ! header()->isSectionHidden(i))
        savedString.append(QString::number(i) + "," +
                           QString::number(header()->sectionSize(i)) + "|");
    }
    if (DEBUG) qDebug("saving %s", qPrintable(savedString));
    settings.setValue(_settingsName + "/columnWidths", savedString);
  }
  if(_x_preferences)
  {
    savedString = "";
    for(int i = 0; i < header()->count(); i++)
    {
      savedString.append(QString::number(i) + "," + (header()->isSectionHidden(i)?"off":"on") + "|");
    }
    if(!savedString.isEmpty())
      _x_preferences->set(_settingsName + "/columnsShown", savedString);
    else
      _x_preferences->remove(_settingsName + "/columnsShown");
  }
}

QString XTreeView::columnNameFromLogicalIndex(const int logicalIndex) const
{
  for (QMap<QString, ColumnProps*>::const_iterator it = _columnByName.constBegin();
       it != _columnByName.constEnd(); it++)
  {
    if (it.value()->logicalIndex == logicalIndex)
      return it.value()->columnName;
  }
  return QString();
}

void XTreeView::sShowHeaderMenu(const QPoint &pntThis)
{
  _menu->clear();

  int logicalIndex = header()->logicalIndexAt(pntThis);
  int currentSize = header()->sectionSize(logicalIndex);
// If we have a default value and the current size is not equal to that default value
// then we want to show the menu items for resetting those values back to default
  ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(logicalIndex), 0);
  if (cp && (cp->defaultWidth > 0) && (cp->defaultWidth != currentSize) )
  {
    _resetWhichWidth = logicalIndex;
    _menu->insertItem(tr("Reset this Width"), this, SLOT(sResetWidth()));
  }

  _menu->insertItem(tr("Reset all Widths"), this, SLOT(sResetAllWidths()));
  _menu->insertSeparator();
  if(_forgetful)
    _menu->insertItem(tr("Remember Widths"), this, SLOT(sToggleForgetfulness()));
  else
    _menu->insertItem(tr("Do Not Remember Widths"), this, SLOT(sToggleForgetfulness()));

  _menu->insertSeparator();

  for(int i = 0; i < header()->count(); i++)
  {
    QAction *act = _menu->addAction(QTreeView::model()->headerData(i, Qt::Horizontal).toString());
    act->setCheckable(true);
    act->setChecked(! header()->isSectionHidden(i));

    ColumnProps *tmp = _columnByName.value(columnNameFromLogicalIndex(i), 0);
    act->setEnabled(tmp ?  ! tmp->locked : true);

    QMap<QString,QVariant> m;
    m.insert("command", QVariant("toggleColumnHidden"));
    m.insert("column", QVariant(i));
    act->setData(m);
    connect(_menu, SIGNAL(triggered(QAction*)), this, SLOT(popupMenuActionTriggered(QAction*)));
  }

  if(_menu->count())
    _menu->popup(mapToGlobal(pntThis));
}

bool XTreeView::isRowHidden(int row)
{
  return QTreeView::isRowHidden(row,model()->parent(model()->index(row,0)));
}

bool XTreeView::throwScriptException(const QString &message)
{
   QObject *ancestor = this;
   QScriptEngine *engine = 0;
   for ( ; ancestor; ancestor = ancestor->parent())
   {
     engine = ancestor->findChild<QScriptEngine*>();
     if (engine)
       break;
   } 
   if (engine)
   {
      QScriptContext *ctx = engine->currentContext();
      ctx->throwError(message);
      return true;
   }
   return false;
}

int XTreeView::rowCount()
{
  return _model.rowCount();
}

int XTreeView::rowCountVisible()
{
  int counter = 0;
  for (int i = 0; i < _model.rowCount(); i++)
  {
    if (!isRowHidden(i))
      counter++;
  }
  return counter;
}

QVariant XTreeView::value(int row, int column)
{
  return model()->data(model()->index(row,column));
}

QVariant XTreeView::selectedValue(int column)
{
  QModelIndexList selected=selectedIndexes();
  return value(selected.first().row(),column);
}

void XTreeView::select()
{
  setTable();
  _model.select();
  emit valid(FALSE);
}

void XTreeView::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
  if (!selected.indexes().isEmpty())
  {
    emit rowSelected(selected.indexes().first().row());
    emit valid(true);
  }
  else
    emit valid(FALSE);
  QTreeView::selectionChanged(selected, deselected);
}

void XTreeView::insert()
{ 
  int row=_model.rowCount();
  _model.insertRows(row,1);
  //Set default values for foreign keys
  for (int i = 0; i < _idx.count(); ++i)
    _model.setData(_model.index(row,i),_idx.value(i));
  selectRow(row);
}

void XTreeView::populate(int p)
{   
  XSqlTableModel *t=static_cast<XSqlTableModel*>(_mapper->model());
  if (t)
  {
    _idx=t->primaryKey();

    for (int i = 0; i < _idx.count(); ++i)
    {
      _idx.setValue(i, t->data(t->index(p,i)));
      setColumnHidden(i,true);
    }
    
    QString clause = QString(_model.database().driver()->sqlStatement(QSqlDriver::WhereStatement, t->tableName(),_idx, false)).mid(6);
    _model.setFilter(clause);
    if (!_model.query().isActive())
      _model.select();
  }
}

void XTreeView::removeSelected()
{
  QModelIndexList selected=selectedIndexes();

  for (int i = selected.count() -1 ; i > -1; i--)
  {
    if (selected.value(i).column() == 1) // Only once per row
    {
      setRowHidden(selected.value(i).row(),selected.value(i).parent(),true);
      _model.removeRow(selected.value(i).row());
    }
  }
}

void XTreeView::revertAll()
{
  _model.revertAll();
  if (_mapper)
    populate(_mapper->currentIndex());
}

void XTreeView::save()
{ 
  if(!_model.submitAll())
  {
    if(!throwScriptException(_model.lastError().databaseText()))
          QMessageBox::critical(this, tr("Error Saving %1").arg(_tableName),
                            tr("Error saving %1 at %2::%3\n\n%4")
                            .arg(_tableName).arg(__FILE__).arg(__LINE__)
                            .arg(_model.lastError().databaseText()));
  }
  else
    emit saved();
}

void XTreeView::selectRow(int index)
{
  selectionModel()->select(QItemSelection(model()->index(index,0),QTreeView::model()->index(index,0)),
                                        QItemSelectionModel::ClearAndSelect |
                                        QItemSelectionModel::Rows);
}

void XTreeView::setDataWidgetMap(XDataWidgetMapper* mapper)
{  
  setTable();
  _mapper=mapper; 
}

void XTreeView::setObjectName(const QString &name)
{
  _settingsName = window()->objectName() + "/" + name;
  QObject::setObjectName(name);
}

void XTreeView::setTable()
{
  if (_model.tableName() == _tableName)
    return;
      
  if (!_tableName.isEmpty())
  {
    QString tablename=_tableName;
    if (!_schemaName.isEmpty())
      tablename = _schemaName + "." + tablename;
    _model.setTable(tablename,_keyColumns);
    
    setModel(&_model);
  }
}

void XTreeView::setModel(XSqlTableModel * model)
{
  QTreeView::setModel(model);

  for (int i = 0; i < QTreeView::model()->columnCount(); ++i)
  {
    QString h = QTreeView::model()->headerData(i, Qt::Horizontal,
                                               Qt::DisplayRole).toString();
    QTreeView::model()->setHeaderData(i, Qt::Horizontal, h, Qt::UserRole);

    ColumnProps *cp = _columnByName.value(h, 0);

    if (! cp)
    {
      cp = new ColumnProps;
      cp->columnName   = h;
      cp->logicalIndex = i;
      cp->defaultWidth = -1;
      cp->savedWidth   = -1;
      cp->stretchy     = true;
      cp->locked       = false;
      cp->visible      = true;
      cp->label        = h;
      cp->fromSettings = false;
      cp->alignment    = QTreeView::model()->headerData(i, Qt::Horizontal,
                                                Qt::TextAlignmentRole).toInt();

      cp->label.replace(0, 1, cp->label.left(1).toUpper());
      for (int j = 0; (j = cp->label.indexOf("_", j)) >= 0; /* empty */)
      {
        cp->label.replace(j,     1, " ");
        cp->label.replace(j + 1, 1, cp->label.mid(j + 1, 1).toUpper());
      }

      _columnByName.insert(cp->columnName, cp);
    }

    QTreeView::model()->setHeaderData(i, Qt::Horizontal, cp->label,
                                      Qt::DisplayRole);
    QTreeView::model()->setHeaderData(i, Qt::Horizontal, cp->alignment,
                                      Qt::TextAlignmentRole);
  }

  if (! _settingsLoaded)
  {
    _settingsLoaded = true;

    QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenMFG");
    _forgetful = settings.value(_settingsName + "/isForgetful").toBool();
    QString savedString;
    QStringList savedParts;
    QString part, key, val;
    bool b1 = false, b2 = false;
    if(!_forgetful)
    {
      savedString = settings.value(_settingsName + "/columnWidths").toString();
      savedParts = savedString.split("|", QString::SkipEmptyParts);
      for(int i = 0; i < savedParts.size(); i++)
      {
        part = savedParts.at(i);
        key = part.left(part.indexOf(","));
        val = part.right(part.length() - part.indexOf(",") - 1);
        b1 = false;
        b2 = false;
        int k = key.toInt(&b1);
        int v = val.toInt(&b2);
        if(b1 && b2)
        {
          if (DEBUG) qDebug("width of %d from settings = %d", k, v);
          ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(k), 0);
          if (cp)
          {
            cp->savedWidth = v;
            cp->stretchy   = false;
            cp->fromSettings = true;
          }
          header()->resizeSection(k, v);
        }
      }
    }

    // Load any previously saved column hidden/visible information
    if(_x_preferences)
    {
      savedString = _x_preferences->value(_settingsName + "/columnsShown");
      savedParts = savedString.split("|", QString::SkipEmptyParts);
      for(int i = 0; i < savedParts.size(); i++)
      {
        part = savedParts.at(i);
        key = part.left(part.indexOf(","));
        val = part.right(part.length() - part.indexOf(",") - 1);
        int c = key.toInt(&b1);
        if(b1 && (val == "on" || val == "off"))
        {
          if (DEBUG) qDebug("visibility of %d from settings = %s",
                            c, qPrintable(val));
          ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(c), 0);
          if (cp)
          {
            cp->visible = (val == "on");
            cp->fromSettings = true;
          }
          header()->setSectionHidden(c, (val != "on"));
        }
      }
    }
  }

  emit newModel(model);
}

void XTreeView::setValue(int row, int column, QVariant value)
{
  _model.setData(_model.index(row,column), value);
}

void XTreeView::resizeEvent(QResizeEvent * e)
{
  QTreeView::resizeEvent(e);

  sColumnSizeChanged(-1, 0, 0);
}

void XTreeView::sResetWidth()
{
  ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(_resetWhichWidth), 0);

  int w = cp ? cp->defaultWidth : -1;

  if(w >= 0)
    header()->resizeSection(_resetWhichWidth, w);
  else
    sColumnSizeChanged(-1, 0, 0);
}

void XTreeView::sResetAllWidths()
{
  bool autoSections = false;
  for (int i = 0; i < header()->length(); i++)
  {
    ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(i), 0);
    int width = cp ? cp->defaultWidth : -1;
    if (width >= 0)
      header()->resizeSection(i, width);
    else
    {
    if (cp)
        cp->stretchy = true;
      autoSections = true;
    }
  }
  if(autoSections)
    sColumnSizeChanged(-1, 0, 0);
}

void XTreeView::sToggleForgetfulness()
{
  _forgetful = !_forgetful;
}


void XTreeView::sColumnSizeChanged(int logicalIndex, int oldSize, int newSize)
{
  if (DEBUG) qDebug("sColumnSizeChanged(%d, %d, %d)",
                    logicalIndex, oldSize, newSize);

  ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(logicalIndex), 0);
  if (cp)
  {
    cp->savedWidth = newSize;
    if (newSize < 0)
      cp->stretchy = true;
    else if (! _resizingInProcess && cp->stretchy)
      cp->stretchy = false;
  }

  if (_resizingInProcess)
    return;

  _resizingInProcess = true;

  int usedSpace = 0;
  QVector<int> stretch;

  for(int i = 0; i < header()->count(); i++)
  {
    ColumnProps *tmpcp = _columnByName.value(columnNameFromLogicalIndex(i), 0);

    if (header()->isSectionHidden(i))
      continue;
    else if (logicalIndex == i && newSize < 0)
      stretch.append(i);
    else if (logicalIndex == i)
      usedSpace += newSize;
    else if (tmpcp && ! tmpcp->stretchy)
      usedSpace += header()->sectionSize(i);
    else
      stretch.append(i);
  }
  if (DEBUG) qDebug("used space = %d, stretch %d cols in the rest",
                    usedSpace, stretch.size());

  if(stretch.size() > 0)
  {
    int leftover = (viewport()->width() - usedSpace) / stretch.size();

    if(leftover < 50)
      leftover = 50;

    for (int n = 0; n < stretch.size(); n++)
      header()->resizeSection(stretch.at(n), leftover);
  }

  _resizingInProcess = false;
}

/* this is intended to be similar to XTreeWidget::addColumn(...)
   but differs in several important ways:
   - this doesn't actually add columns to the tree view, it only defines how
     they should appear by default if the model contains columns with the given
     colname.
   - as a result it has the opposite effect on visibility - any column not
     explicitly mentioned in a setColumn call /is visible/ in an XTreeView
   - the alignment parameter only affects the header
TODO: figure out how to set the alignment of individual elements
 */
void XTreeView::setColumn(const QString &label, int width, int alignment, bool visible, const QString &colname)
{
  if (DEBUG)
    qDebug("setColumn(%s, %d, %d, %d, %s)",
           qPrintable(label), width, alignment, visible, qPrintable(colname));

  ColumnProps *cp = _columnByName.value(colname, 0);

  if (! cp)
  {
    cp = new ColumnProps();
    cp->columnName = colname;
    cp->fromSettings = false;
    _columnByName.insert(cp->columnName, cp);
  }

  cp->defaultWidth = width;
  cp->locked       = false;
  cp->visible      = visible;
  cp->label        = label;
  cp->alignment    = alignment;

  if (cp->fromSettings)
    cp->stretchy   = (width == -1 && cp->savedWidth == -1);
  else
  {
    cp->savedWidth = -1;
    cp->stretchy   = (width == -1);
  }

  cp->logicalIndex = -1;
  int colnum = -1;
  for (colnum = 0; colnum < header()->count(); colnum++)
  {
    if (colname == QTreeView::model()->headerData(colnum, Qt::Horizontal,
                                                  Qt::UserRole).toString())
    {
      cp->logicalIndex = colnum;
      QTreeView::model()->setHeaderData(colnum, Qt::Horizontal, colname,
                                        Qt::UserRole);
      QTreeView::model()->setHeaderData(colnum, Qt::Horizontal, label,
                                        Qt::DisplayRole);
      QTreeView::model()->setHeaderData(colnum, Qt::Horizontal, alignment,
                                        Qt::TextAlignmentRole);
      /* setData below always returns false => failure. why?
      for (int i = 0; i < QTreeView::model()->rowCount(); i++)
        bool test = QTreeView::model()->setData(QTreeView::model()->index(i, colnum),
                                                alignment, Qt::TextAlignmentRole);
      */
      break;
    }
  }

  header()->setResizeMode(colnum, QHeaderView::Interactive);

  if (! cp->fromSettings)
    setColumnVisible(colnum, visible);

  if (_forgetful || ! cp->fromSettings)
    header()->resizeSection(colnum, width);
}

void XTreeView::setColumnLocked(const QString &pColname, bool pLocked)
{
  if (_columnByName.value(pColname))
    _columnByName.value(pColname)->locked = pLocked;
}

void XTreeView::setColumnLocked(int pColumn, bool pLocked)
{
  ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(pColumn), 0);
  if (cp)
    cp->locked = pLocked;
}

void XTreeView::popupMenuActionTriggered(QAction * pAction)
{
  QMap<QString, QVariant> m = pAction->data().toMap();
  QString command = m.value("command").toString();
  if("toggleColumnHidden" == command)
    setColumnVisible(m.value("column").toInt(), pAction->isChecked());
  //else if (some other command to handle)
}

void XTreeView::setColumnVisible(int pColumn, bool pVisible)
{
  if(pVisible)
    header()->showSection(pColumn);
  else
    header()->hideSection(pColumn);

  ColumnProps *cp = _columnByName.value(columnNameFromLogicalIndex(pColumn), 0);
  if (cp)
    cp->visible = pVisible;
}
