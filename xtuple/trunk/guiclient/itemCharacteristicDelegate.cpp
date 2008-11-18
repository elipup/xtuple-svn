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

#include <QtGui>
#include <QComboBox>

#include "itemCharacteristicDelegate.h"
#include "guiclient.h"

ItemCharacteristicDelegate::ItemCharacteristicDelegate(QObject *parent)
  : QItemDelegate(parent)
{
}

QWidget *ItemCharacteristicDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem & /*style*/,
    const QModelIndex & index) const
{
  if(index.column()!=1)
    return 0;

  QModelIndex idx = index.sibling(index.row(), CHAR);
  q.prepare("SELECT charass_value"
            "  FROM charass, char"
            " WHERE ((charass_char_id=char_id)"
            "   AND  (charass_target_type='I')"
            "   AND  (charass_target_id=:item_id)"
            "   AND  (char_id=:char_id) );");
  q.bindValue(":char_id", idx.model()->data(idx, Qt::UserRole));
  q.bindValue(":item_id", index.model()->data(index, Qt::UserRole));
  q.exec();

  QComboBox *editor = new QComboBox(parent);
  editor->setEditable(true);


#ifdef Q_WS_MAC
  QFont boxfont = editor->font();
  boxfont.setPointSize((boxfont.pointSize() == -1) ? boxfont.pixelSize() - 3 : boxfont.pointSize() - 3);
  editor->setFont(boxfont);
#endif

  while(q.next())
    editor->addItem(q.value("charass_value").toString());
  editor->installEventFilter(const_cast<ItemCharacteristicDelegate*>(this));

  return editor;
}

void ItemCharacteristicDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
  QString value = index.model()->data(index, Qt::DisplayRole).toString();

  QComboBox *comboBox = static_cast<QComboBox*>(editor);
  int curIdx = comboBox->findText(value);

  if(curIdx != -1)
    comboBox->setCurrentIndex(curIdx);
  else
    comboBox->setEditText(value);
}

void ItemCharacteristicDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
  QComboBox *comboBox = static_cast<QComboBox*>(editor);
  QModelIndex charidx = index.sibling(index.row(), CHAR);
  QModelIndex priceidx = index.sibling(index.row(), PRICE);
  QVariant charVars;
  QVariantList listVars;
  if (priceidx.model())
    charVars.setValue(priceidx.model()->data(priceidx, Qt::UserRole).toList());
  listVars=charVars.toList();
  
  if (listVars.value(CUST_ID).toInt())
  {
    q.prepare("SELECT formatprice(itemcharprice(:item_id,:char_id,:value,:cust_id,:shipto_id,:qty,:curr_id,:effective)) AS price;");
    
    q.bindValue(":item_id"  , listVars.value(ITEM_ID).toInt());
    q.bindValue(":char_id"  , charidx.model()->data(charidx, Qt::UserRole));
    q.bindValue(":value"    , comboBox->currentText());
    q.bindValue(":cust_id"  , listVars.value(CUST_ID));
    q.bindValue(":shipto_id", listVars.value(SHIPTO_ID));
    q.bindValue(":qty"      , listVars.value(QTY));
    q.bindValue(":curr_id"  , listVars.value(CURR_ID));
    q.bindValue(":effective", listVars.value(EFFECTIVE));
    q.exec();
    if (q.first())
      model->setData(priceidx, q.value("price").toString());
    model->setData(index, comboBox->currentText());
  }
}

void ItemCharacteristicDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
}



