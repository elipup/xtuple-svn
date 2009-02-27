/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSpinBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <Q3TextStream>
#include <QDomDocument>
#include <Q3FileDialog>
#include <QCloseEvent>
#include <Q3SqlRecordInfo>
#include <Q3ValueList>

#include <missingfield.h>

#include "rowcontroller.h"

void CSVAtlasWindow::fileNew()
{
  _map->clear();
  _filename = QString::null;
  sMapChanged(0);
  if(_atlas)
  {
    delete _atlas;
    _atlas = 0;
  }
  _atlas = new CSVAtlas();
}


void CSVAtlasWindow::fileOpen()
{
  QString filename = Q3FileDialog::getOpenFileName(_filename, QString::null, this);
  if(filename.isNull())
    return;

  _map->clear();
  sMapChanged(0);
  if(_atlas)
  {
    delete _atlas;
    _atlas = 0;
  }

  QFile file(filename);

  QDomDocument doc = QDomDocument();
  QString errMsg;
  int errLine, errCol;
  if(doc.setContent(&file, &errMsg, &errLine, &errCol))
  {  
    _atlas = new CSVAtlas(doc.documentElement());
    QStringList list = _atlas->mapList();
    list.sort();
    _map->insertStringList(list);
    sMapChanged(0);
    _filename = filename;
  }
  else
    QMessageBox::warning(this, tr("Error Reading File"), tr("An error was encountered while trying to read the file %1.\n%2\nLine %3, Column %4").arg(filename).arg(errMsg).arg(errLine).arg(errCol));

  if(!_atlas)
    _atlas = new CSVAtlas();
}


void CSVAtlasWindow::fileSave()
{
  if(_filename.isEmpty())
  {
    fileSaveAs();
    if(_filename.isEmpty())
      return;
  }
  sMapChanged(_map->currentItem());

  QDomDocument doc = QDomDocument("openCSVAtlasDef");
  doc.appendChild(_atlas->createElement(doc));

  QFile file(_filename);
  if(file.open(QIODevice::WriteOnly))
  {
    Q3TextStream ts(&file);
    ts << doc.toString();
    file.close();
  }
  else
    QMessageBox::warning(this, tr("Error Opening File"), tr("Could not open the file %1 for writing to.").arg(_filename));
}


void CSVAtlasWindow::fileSaveAs()
{
  QString filename = Q3FileDialog::getSaveFileName(_filename, QString::null, this);
  if(filename.isEmpty())
    return;

  _filename = filename;
  fileSave();
}


void CSVAtlasWindow::filePrint()
{
  QMessageBox::information(this, tr("Not yet implimented"), tr("This feature has not yet been implimented."));
}



void CSVAtlasWindow::helpIndex()
{
  QMessageBox::information(this, tr("Not yet implimented"), tr("This feature has not yet been implimented."));
}


void CSVAtlasWindow::helpContents()
{
  QMessageBox::information(this, tr("Not yet implimented"), tr("This feature has not yet been implimented."));
}


void CSVAtlasWindow::helpAbout()
{
  QMessageBox::information(this, tr("Not yet implimented"), tr("This feature has not yet been implimented."));
}


void CSVAtlasWindow::sRenameMap()
{
  QMessageBox::information(this, tr("Not yet implimented"), tr("This feature has not yet been implimented."));
}


void CSVAtlasWindow::sAddMap()
{
  QSqlDatabase db = QSqlDatabase::database();
  if(db.isValid())
  {
    bool ok = FALSE;
    QStringList mList;
    QString name = QString::null;
    while(true)
    {
      name = QInputDialog::getText(tr("Map Name"), tr("Map Name:"), QLineEdit::Normal, name, &ok, this);
      if(ok)
      {
        if(name.isEmpty())
        {
          QMessageBox::warning(this, tr("Must enter a value"), tr("You must enter a value"));
          continue;
        }

        mList = _atlas->mapList();
        if(mList.contains(name))
        {
          QMessageBox::warning(this, tr("Must enter unique name"), tr("The new map name you entered already exists. Please enter in a unique map name."));
          continue;
        }

        break;
      }
      else
        return;
    }

    QStringList tables;
    QSqlQuery qry;
    qry.exec( "SELECT tablename FROM ( "
              "SELECT schemaname || '.' || viewname AS tablename from pg_views "
              "WHERE schemaname IN ('api','public') "
              "UNION "
              "SELECT schemaname || '.' || tablename AS tablename from pg_tables "
              "WHERE schemaname IN ('api','public') ) "
              "AS data "
              "ORDER by tablename; " );
    while(qry.next())
      tables.append(qry.value(0).toString());

    QString table = QInputDialog::getItem(tr("Select Table"), tr("Table:"), tables, 0, FALSE, &ok, this);
    if(!ok)
      return;

    CSVMap map(name);
    map.setTable(table);
    _atlas->setMap(map);

    mList = _atlas->mapList();
    mList.sort();
    _map->clear();
    _map->insertStringList(mList);
    _map->setCurrentItem(mList.findIndex(name));
    sMapChanged(_map->currentItem());
  }
  else
    QMessageBox::critical(this, tr("No Database"), tr("Could not get the database connection."));
}


void CSVAtlasWindow::sDeleteMap()
{
  _selectedMap = QString::null;
  int pos = _map->currentItem();
  QString name = _map->currentText();
  _atlas->removeMap(name);
  QStringList mList = _atlas->mapList();
  mList.sort();
  _map->clear();
  _map->insertStringList(mList);
  if(pos >= mList.count())
    pos = QMAX(mList.count() - 1, 0);
  _map->setCurrentItem(pos);
  sMapChanged(_map->currentItem());
}


void CSVAtlasWindow::sMapChanged( int )
{
  CSVMap map;
  if(!_selectedMap.isEmpty())
  {
    map = _atlas->map(_selectedMap);
    if(tr("Insert") == _action->currentText())
      map.setAction(CSVMap::Insert);
    else if(tr("Update") == _action->currentText())
      map.setAction(CSVMap::Update);
    else if(tr("Append") == _action->currentText())
      map.setAction(CSVMap::Append);
    map.setDescription(_description->text());
    map.setSqlPre(_preSql->text().stripWhiteSpace());
    map.setSqlPreContinueOnError(_sqlPreContinueOnError->isChecked());
    map.setSqlPost(_postSql->text().stripWhiteSpace());
    for(int r = 0; r < _fields->numRows(); r++)
    {
      CSVMapField field = map.field(_fields->text(r, 1));
      field.setName(_fields->text(r, 1));
      Q3TableItem * item = _fields->item(r, 0);
      if(item && item->rtti() == 2)
        field.setIsKey(((Q3CheckTableItem*)item)->isChecked());
      else
        field.setIsKey(FALSE);
      field.setType(QVariant::nameToType(_fields->text(r, 2)));
      item = _fields->item(r, 4);
      if(item && item->rtti() == 1)
        field.setAction(CSVMapField::nameToAction(((Q3ComboTableItem*)item)->currentText()));
      else
        field.setAction(CSVMapField::Action_Default);
      QWidget * w = _fields->cellWidget(r, 5);
      if(w)
        field.setColumn(((QSpinBox*)w)->value());
      else
        field.setColumn(0);
      item = _fields->item(r, 6);
      if(item && item->rtti() == 1)
        field.setIfNullAction(CSVMapField::nameToIfNull(((Q3ComboTableItem*)item)->currentText()));
      else
        field.setIfNullAction(CSVMapField::Nothing);
      w = _fields->cellWidget(r, 7);
      if(w)
        field.setColumnAlt(((QSpinBox*)w)->value());
      else
        field.setColumnAlt(1);
      item = _fields->item(r, 8);
      if(item && item->rtti() == 1)
        field.setIfNullActionAlt(CSVMapField::nameToIfNull(((Q3ComboTableItem*)item)->currentText()));
      else
        field.setIfNullActionAlt(CSVMapField::Nothing);
      field.setValueAlt(_fields->text(r, 9));
      map.setField(field);
    }
    map.simplify();
    _atlas->setMap(map);
  }
 
  QSqlDatabase db = QSqlDatabase::database();
  if(db.isValid())
  {
    _fields->setNumRows(0);
    if(_map->count())
    {
      // CODE TO SELECT MAP
      _selectedMap = _map->currentText();
      map = _atlas->map(_selectedMap);

      _table->setTitle(tr("Table: ") + map.table());
      _table->setEnabled(TRUE);

      _action->setCurrentItem(map.action());
      _description->setText(map.description());
      _preSql->setText(map.sqlPre());
      _sqlPreContinueOnError->setChecked(map.sqlPreContinueOnError());
      _postSql->setText(map.sqlPost());

      //QStringList tList = db.tables(QSql::AllTables);
      QString tList;
      QStringList tables;
      QSqlQuery qry;
      qry.exec( "SELECT tablename FROM ( "
                "SELECT schemaname || '.' || viewname AS tablename from pg_views "
                "WHERE schemaname IN ('api','public') "
                "UNION "
                "SELECT schemaname || '.' || tablename AS tablename from pg_tables "
                "WHERE schemaname IN ('api','public')  "
                "UNION "
                "SELECT tablename AS tablename from pg_tables "
                "WHERE (schemaname = 'public') ) "
                "AS data "
                "ORDER by tablename; " );
      while(qry.next())
        tList.append(qry.value(0).toString());
      if(tList.contains(map.table()))
      {
        Q3SqlRecordInfo fields = db.recordInfo(map.table());
        _fields->setNumRows(fields.count());

        QStringList fList = map.fieldList();
     //   Q3ValueList<QString>::iterator it;
        for(int i = 0; i < fList.size(); ++i)
        {
          if(!fields.contains(fList.at(i)))
          {
            CSVMapField f = map.field(fList.at(i));
            map.removeField(fList.at(i));
            MissingField diag(this);
            diag.init(f.name(), fields);
            if(diag.exec() == QDialog::Accepted)
            {
              f.setName(diag._fields->currentText());
              map.setField(f);
            }
            _atlas->setMap(map);
          }
        }

        Q3SqlRecordInfo::iterator field;
        int row;
        for(row = 0, field = fields.begin(); field != fields.end(); ++field, ++row)
        {
          CSVMapField mf = map.field((*field).name());
          Q3CheckTableItem * check = new Q3CheckTableItem(_fields, QString(""));
          if(!mf.isEmpty())
            check->setChecked(mf.isKey());
          _fields->setItem(row, 0, check);
          _fields->setText(row, 1, (*field).name());
          _fields->setText(row, 2, QVariant::typeToName((*field).type()));
          _fields->setText(row, 3, ((*field).isRequired()>0?tr("Yes"):(*field).isRequired()==0?tr("No"):tr("Unknown")));
          Q3ComboTableItem * combo = new Q3ComboTableItem(_fields, CSVMapField::actionList());
          combo->setCurrentItem(CSVMapField::actionToName(mf.action()));
          _fields->setItem(row, 4, combo);
          QSpinBox * spinner = new QSpinBox(1, 999, 1, _fields);
          RowController * control = new RowController(_fields, row, spinner);
          control->setAction(combo);
          //spinner->setSpecialValueText(tr("default"));
          spinner->setPrefix(tr("Column "));
          if(!mf.isEmpty())
            spinner->setValue(mf.column());
          //connect(spinner, SIGNAL(valueChanged(int)), this, SLOT(sUpdated()));
          control->setColumn(spinner);
          _fields->setCellWidget(row, 5, spinner);
          combo = new Q3ComboTableItem(_fields, CSVMapField::ifNullList());
          combo->setCurrentItem(CSVMapField::ifNullToName(mf.ifNullAction()));
          control->setIfNull(combo);
          _fields->setItem(row, 6, combo);
          spinner = new QSpinBox(1, 999, 1, _fields);
          spinner->setPrefix(tr("Column "));
          if(!mf.isEmpty())
            spinner->setValue(mf.columnAlt());
          //connect(spinner, SIGNAL(valueChanged(int)), this, SLOT(sUpdated()));
          control->setAltColumn(spinner);
          _fields->setCellWidget(row, 7, spinner);
          combo = new Q3ComboTableItem(_fields, CSVMapField::ifNullList(TRUE));
          combo->setCurrentItem(CSVMapField::ifNullToName(mf.ifNullActionAlt()));
          control->setAltIfNull(combo);
          _fields->setItem(row, 8, combo);
          Q3TableItem * titem = new Q3TableItem(_fields, Q3TableItem::OnTyping, mf.valueAlt());
          control->setAltValue(titem);
          _fields->setItem(row, 9, titem);
          control->finishSetup();
        }
      }
      else
      {
        QMessageBox::warning(this, tr("No Existing Table"), tr("the table %1 does not exist on this database.\nYou may continue to use and edit this map however only those fields that are know will be shown.").arg(map.table()));


        Q3ValueList<CSVMapField> fields = map.fields();
        _fields->setNumRows(fields.count());
        Q3ValueList<CSVMapField>::iterator field;
        int row;
        for(row = 0, field = fields.begin(); field != fields.end(); ++field, ++row)
        {
          CSVMapField mf = (*field);
          Q3CheckTableItem * check = new Q3CheckTableItem(_fields, QString(""));
          if(!mf.isEmpty())
            check->setChecked(mf.isKey());
          _fields->setItem(row, 0, check);
          _fields->setText(row, 1, (*field).name());
          _fields->setText(row, 2, QVariant::typeToName((*field).type()));
          _fields->setText(row, 3, tr("Unknown"));
          Q3ComboTableItem * combo = new Q3ComboTableItem(_fields, CSVMapField::actionList());
          combo->setCurrentItem(CSVMapField::actionToName(mf.action()));
          _fields->setItem(row, 4, combo);
          QSpinBox * spinner = new QSpinBox(1, 999, 1, _fields);
          RowController * control = new RowController(_fields, row, spinner);
          control->setAction(combo);
          //spinner->setSpecialValueText(tr("default"));
          spinner->setPrefix(tr("Column "));
          if(!mf.isEmpty())
            spinner->setValue(mf.column());
          //connect(spinner, SIGNAL(valueChanged(int)), this, SLOT(sUpdated()));
          control->setColumn(spinner);
          _fields->setCellWidget(row, 5, spinner);
          combo = new Q3ComboTableItem(_fields, CSVMapField::ifNullList());
          combo->setCurrentItem(CSVMapField::ifNullToName(mf.ifNullAction()));
          control->setIfNull(combo);
          _fields->setItem(row, 6, combo);
          spinner = new QSpinBox(1, 999, 1, _fields);
          spinner->setPrefix(tr("Column "));
          if(!mf.isEmpty())
            spinner->setValue(mf.columnAlt());
          //connect(spinner, SIGNAL(valueChanged(int)), this, SLOT(sUpdated()));
          control->setAltColumn(spinner);
          _fields->setCellWidget(row, 7, spinner);
          combo = new Q3ComboTableItem(_fields, CSVMapField::ifNullList(TRUE));
          combo->setCurrentItem(CSVMapField::ifNullToName(mf.ifNullActionAlt()));
          control->setAltIfNull(combo);
          _fields->setItem(row, 8, combo);
          Q3TableItem * titem = new Q3TableItem(_fields, Q3TableItem::OnTyping, mf.valueAlt());
          control->setAltValue(titem);
          _fields->setItem(row, 9, titem);
          control->finishSetup();
        }
      }
    }
    else
    {
      _selectedMap = QString::null;
      _table->setTitle(tr("Table: "));
      _table->setEnabled(FALSE);
    }
  }
  else
    QMessageBox::critical(this, tr("No Database"), tr("Could not get the database connection."));
}


void CSVAtlasWindow::init()
{
  _filename = QString::null;
  _atlas = new CSVAtlas();
  _selectedMap = QString::null;
  sMapChanged(0);
}


void CSVAtlasWindow::closeEvent( QCloseEvent * e)
{
  sMapChanged(_map->currentItem());
  e->accept();
}


CSVAtlas* CSVAtlasWindow::getAtlas()
{
  sMapChanged(_map->currentItem());
  return _atlas;
}
