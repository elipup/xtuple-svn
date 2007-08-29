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

#include "dspItemSourcesByItem.h"

#include <QVariant>
#include <QStatusBar>
#include <QWorkspace>
#include <parameter.h>
#include "itemSource.h"
#include "buyCard.h"
#include "dspPoItemsByItem.h"
#include "rptItemSourcesByItem.h"
#include "OpenMFGGUIClient.h"

/*
 *  Constructs a dspItemSourcesByItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspItemSourcesByItem::dspItemSourcesByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : QMainWindow(parent, name, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(_item, SIGNAL(newId(int)), this, SLOT(sFillList()));
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
    connect(_itemsrc, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspItemSourcesByItem::~dspItemSourcesByItem()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspItemSourcesByItem::languageChange()
{
    retranslateUi(this);
}

//Added by qt3to4:
#include <QMenu>

void dspItemSourcesByItem::init()
{
  statusBar()->hide();

  _item->setType(ItemLineEdit::cGeneralPurchased);

  _itemsrc->addColumn(tr("Vendor"),      -1,          Qt::AlignLeft   );
  _itemsrc->addColumn(tr("Item Number"), _itemColumn, Qt::AlignLeft   );
  _itemsrc->addColumn(tr("UOM"),         _uomColumn,  Qt::AlignCenter );
  _itemsrc->addColumn(tr("UOM Ratio"),   _qtyColumn,  Qt::AlignRight  );

  _item->setFocus();
}

void dspItemSourcesByItem::sPrint()
{
  ParameterList params;
  params.append("item_id", _item->id());
  params.append("print");

  rptItemSourcesByItem newdlg(this, "", TRUE);
  newdlg.set(params);
}

void dspItemSourcesByItem::sPopulateMenu(QMenu *menuThis)
{
  menuThis->insertItem("Edit...",           this, SLOT(sEdit()),     0 );
  menuThis->insertItem("View Buy Card...",  this, SLOT(sBuyCard()),  0 );
  menuThis->insertItem("View P/Os...",      this, SLOT(sViewPOs()),  0 );
}

void dspItemSourcesByItem::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("itemsrc_id", _itemsrc->id());

  itemSource newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != QDialog::Rejected)
    sFillList();
}

void dspItemSourcesByItem::sBuyCard()
{
  ParameterList params;
  params.append("itemsrc_id", _itemsrc->id());

  buyCard *newdlg = new buyCard();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspItemSourcesByItem::sViewPOs()
{
  ParameterList params;
  params.append("itemsrc_id", _itemsrc->id());
  params.append("run");
  
  dspPoItemsByItem *newdlg = new dspPoItemsByItem();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspItemSourcesByItem::sFillList()
{
  if (_item->isValid())
  {
    q.prepare( "SELECT itemsrc_id, vend_name, itemsrc_vend_item_number, itemsrc_vend_uom,"
               "       formatQty(itemsrc_invvendoruomratio) "
               "FROM itemsrc, vend "
               "WHERE ( (itemsrc_vend_id=vend_id)"
               " AND (itemsrc_item_id=:item_id) );" );
    q.bindValue(":item_id", _item->id());
    q.exec();
    _itemsrc->populate(q);
  }
  else
    _itemsrc->clear();
}
