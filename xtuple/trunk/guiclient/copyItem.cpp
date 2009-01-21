/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "copyItem.h"

#include <QMessageBox>
#include <QVariant>

#include "itemSite.h"

copyItem::copyItem(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_copy, SIGNAL(clicked()), this, SLOT(sCopy()));
  connect(_source, SIGNAL(typeChanged(const QString&)), this, SLOT(sHandleItemType(const QString&)));

  _captive = FALSE;
}

copyItem::~copyItem()
{
  // no need to delete child widgets, Qt does it all for us
}

void copyItem::languageChange()
{
  retranslateUi(this);
}

enum SetResponse copyItem::set(const ParameterList &pParams)
{
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
  {
    _source->setId(param.toInt());
    _source->setEnabled(FALSE);
    _targetItemNumber->setFocus();
  }

  return NoError;
}

void copyItem::sHandleItemType(const QString &pItemType)
{
  if  (pItemType == "M" || pItemType == "B" ||
       pItemType == "F" || pItemType == "K" ||
       pItemType == "P" || pItemType == "O" ||
       pItemType == "L" || pItemType == "J")
  {
    _copyBOM->setChecked(TRUE);
    _copyBOM->setEnabled(TRUE);
    if (_metrics->boolean("Routings"))
    {
      _copyBOO->setChecked(TRUE);
      _copyUsedAt->setChecked(TRUE);
      _copyBOO->setEnabled(TRUE);
      _copyUsedAt->setEnabled(TRUE);
    }
    else
    {
      _copyBOO->setChecked(FALSE);
      _copyUsedAt->setChecked(FALSE);
      _copyBOO->hide();
      _copyUsedAt->hide();
    }
  }
  else
  {
    _copyBOM->setChecked(FALSE);
    _copyBOM->setEnabled(FALSE);
    _copyBOO->setChecked(FALSE);
    _copyUsedAt->setChecked(FALSE);
    if (_metrics->boolean("Routings"))
    {
      _copyBOO->setEnabled(FALSE);
      _copyUsedAt->setEnabled(FALSE);
    }
    else
    {
      _copyBOO->hide();
      _copyUsedAt->hide();
    }
  }
}

void copyItem::sCopy()
{
  _targetItemNumber->setText(_targetItemNumber->text().trimmed().toUpper());

  if (_targetItemNumber->text().length() == 0)
  {
    QMessageBox::warning( this, tr("Enter Item Number"),
                          tr( "You must enter a valid target Item Number before\n"
                              "attempting to copy the selected Item.\n"
                              "Please enter a valid Item Number before continuing.") );
    _targetItemNumber->setFocus();
    return;
  }

  q.prepare( "SELECT item_number "
             "FROM item "
             "WHERE item_number=:item_number;" );
  q.bindValue(":item_number", _targetItemNumber->text());
  q.exec();
  if (q.first())
  {
    QMessageBox::critical(  this, tr("Item Number Exists"),
                            tr( "An Item with the item number '%1' already exists.\n"
                                "You may not copy over an existing item." )
                            .arg(_targetItemNumber->text()) );

    _targetItemNumber->clear();
    _targetItemNumber->setFocus();
    return;
  }

  int itemid = -1;

  q.prepare("SELECT copyItem(:source_item_id, :newItemNumber, :copyBOM, :copyBOO, :copyItemCosts, :copyUsedAt) AS itemid;");
  q.bindValue(":source_item_id", _source->id());
  q.bindValue(":newItemNumber", _targetItemNumber->text());
  q.bindValue(":copyBOM",       QVariant(_copyBOM->isChecked()));
  q.bindValue(":copyBOO",       QVariant(_copyBOO->isChecked()));
  q.bindValue(":copyItemCosts", QVariant(_copyCosts->isChecked()));
  q.bindValue(":copyUsedAt",    QVariant(_copyUsedAt->isChecked()));
  q.exec();
  if (q.first())
  {
    itemid = q.value("itemid").toInt();

    omfgThis->sItemsUpdated(itemid, TRUE);

    if (_copyBOM->isChecked())
      omfgThis->sBOMsUpdated(itemid, TRUE);

    if (_copyBOO->isChecked())
      omfgThis->sBOOsUpdated(itemid, TRUE);

    if (QMessageBox::information( this, tr("Create New Item Sites"),
                                  tr("Would you like to create new Item Sites for the newly created Item?"),
                                  tr("&Yes"), tr("&No"), QString::null, 0, 1) == 0)
    {
      ParameterList params;
      params.append("mode", "new");
      params.append("item_id", itemid);
      
      itemSite newdlg(this, "", TRUE);
      newdlg.set(params);
      newdlg.exec();
    }
  }
//  ToDo

  if (_captive)
    done(itemid);
  else
  {
    _source->setId(-1);
    _targetItemNumber->clear();
    _source->setFocus();
    _copyBOM->setEnabled(TRUE);
    _copyBOO->setEnabled(TRUE);
    _copyUsedAt->setEnabled(TRUE);
    _close->setText(tr("&Close"));
  }
}

