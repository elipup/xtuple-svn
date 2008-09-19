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

#include "dspWoMaterialsByItem.h"

#include <QVariant>
//#include <QStatusBar>
#include <openreports.h>
#include <parameter.h>
#include "inputManager.h"

/*
 *  Constructs a dspWoMaterialsByItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspWoMaterialsByItem::dspWoMaterialsByItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_item, SIGNAL(valid(bool)), _query, SLOT(setEnabled(bool)));
  connect(_item, SIGNAL(valid(bool)), _print, SLOT(setEnabled(bool)));

  omfgThis->inputManager()->notify(cBCItem, this, _item, SLOT(setItemid(int)));
  omfgThis->inputManager()->notify(cBCItemSite, this, _item, SLOT(setItemsiteid(int)));

  _womatl->addColumn(tr("W/O #"),         _orderColumn, Qt::AlignLeft   );
  _womatl->addColumn(tr("Parent Item #"), _itemColumn,  Qt::AlignLeft   );
  _womatl->addColumn(tr("Oper. #"),       _dateColumn,  Qt::AlignCenter );
  _womatl->addColumn(tr("Iss. Meth."),    _dateColumn,  Qt::AlignCenter );
  _womatl->addColumn(tr("Iss. UOM"),      _uomColumn,   Qt::AlignLeft   );
  _womatl->addColumn(tr("Qty. Per"),      _qtyColumn,   Qt::AlignRight  );
  _womatl->addColumn(tr("Scrap %"),       _prcntColumn, Qt::AlignRight  );
  _womatl->addColumn(tr("Required"),      _qtyColumn,   Qt::AlignRight  );
  _womatl->addColumn(tr("Issued"),        _qtyColumn,   Qt::AlignRight  );
  _womatl->addColumn(tr("Scrapped"),      _qtyColumn,   Qt::AlignRight  );
  _womatl->addColumn(tr("Balance"),       _qtyColumn,   Qt::AlignRight  );
  _womatl->addColumn(tr("Due Date"),      _dateColumn,  Qt::AlignCenter );
}

/*
 *  Destroys the object and frees any allocated resources
 */
dspWoMaterialsByItem::~dspWoMaterialsByItem()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspWoMaterialsByItem::languageChange()
{
  retranslateUi(this);
}

void dspWoMaterialsByItem::sPrint()
{
  ParameterList params;

  params.append("item_id", _item->id());
  _warehouse->appendValue(params);

  orReport report("WOMaterialRequirementsByComponentItem", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspWoMaterialsByItem::sFillList()
{
  if (!checkParameters())
    return;

  _womatl->clear();

  QString sql( "SELECT womatl_id,"
               "       formatWONumber(wo_id) AS wonumber,"
               "       item_number,"
               "       formatwooperseq(womatl_wooper_id) AS wooperseq, "
               "       CASE WHEN (womatl_issuemethod = 'S') THEN :push"
               "            WHEN (womatl_issuemethod = 'L') THEN :pull"
               "            WHEN (womatl_issuemethod = 'M') THEN :mixed"
               "            ELSE :error"
               "       END AS issuemethod,"
               "       uom_name,"
               "       formatQty(womatl_qtyper) as qtyper,"
               "       formatScrap(womatl_scrap) AS scrap,"
               "       formatQty(womatl_qtyreq) AS qtyreq,"
               "       formatQty(womatl_qtyiss) AS qtyiss,"
               "       formatQty(womatl_qtywipscrap) AS scrapped,"
               "       formatQty(noNeg(womatl_qtyreq - womatl_qtyiss)) AS f_balance,"
               "       noNeg(womatl_qtyreq - womatl_qtyiss) AS balance,"
               "       formatDate(womatl_duedate) AS duedate,"
               "       (womatl_duedate <= CURRENT_DATE) as latedue "
               "FROM wo, womatl, itemsite AS parentsite, itemsite AS componentsite, item, uom "
               "WHERE ((womatl_wo_id=wo_id)"
               " AND (womatl_uom_id=uom_id)"
               " AND (wo_status <> 'C')"
               " AND (wo_itemsite_id=parentsite.itemsite_id)"
               " AND (womatl_itemsite_id=componentsite.itemsite_id)"
               " AND (parentsite.itemsite_item_id=item_id)"
               " AND (componentsite.itemsite_item_id=:item_id)" );

  if (_warehouse->isSelected())
    sql += " AND (componentsite.itemsite_warehous_id=:warehous_id)";

  sql += ") "
         "ORDER BY wo_startdate, item_number;";

  q.prepare(sql);
  _warehouse->bindValue(q);
  q.bindValue(":push", tr("Push"));
  q.bindValue(":pull", tr("Pull"));
  q.bindValue(":mixed", tr("Mixed"));
  q.bindValue(":error", tr("Error"));
  q.bindValue(":item_id", _item->id());
  q.exec();
  if (q.first())
  {
    double totalRequired  = 0;

    XTreeWidgetItem *last = 0;
    do
    {
      totalRequired += q.value("balance").toDouble();

      last = new XTreeWidgetItem( _womatl, last, q.value("womatl_id").toInt(),
				 q.value("wonumber"), q.value("item_number"),
				 q.value("wooperseq"), q.value("issuemethod"),
				 q.value("uom_name"), q.value("qtyper"), q.value("scrap"),
				 q.value("qtyreq"), q.value("qtyiss"),
				 q.value("scrapped"), q.value("f_balance"));
      last->setText(11, q.value("duedate").toString());
      if (q.value("latedue").toBool())
        last->setTextColor(11, "red");
    }
    while (q.next());

    new XTreeWidgetItem( _womatl, last, -1,
                       "", tr("Total Required:"), "", "", "", "", "", "", "", "",
                       formatQty(totalRequired) );
  }
}

bool dspWoMaterialsByItem::checkParameters()
{
  return TRUE;
}

