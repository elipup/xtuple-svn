/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspMaterialUsageVarianceByComponentItem.h"

#include <QVariant>
//#include <QStatusBar>
#include <QMenu>
#include <parameter.h>
#include <openreports.h>

/*
 *  Constructs a dspMaterialUsageVarianceByComponentItem as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
dspMaterialUsageVarianceByComponentItem::dspMaterialUsageVarianceByComponentItem(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  // signals and slots connections
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_womatlvar, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_item, SIGNAL(warehouseIdChanged(int)), _warehouse, SLOT(setId(int)));
  connect(_item, SIGNAL(newId(int)), _warehouse, SLOT(findItemSites(int)));

  _dates->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dates->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _womatlvar->addColumn(tr("Post Date"),      _dateColumn,  Qt::AlignCenter, true,  "posted" );
  _womatlvar->addColumn(tr("Parent Item"),    _itemColumn,  Qt::AlignLeft,   true,  "item_number"   );
  _womatlvar->addColumn(tr("Ordered"),        _qtyColumn,   Qt::AlignRight,  true,  "ordered"  );
  _womatlvar->addColumn(tr("Produced"),       _qtyColumn,   Qt::AlignRight,  true,  "received"  );
  _womatlvar->addColumn(tr("Proj. Req."),     _qtyColumn,   Qt::AlignRight,  true,  "projreq"  );
  _womatlvar->addColumn(tr("Proj. Qty. per"), _qtyColumn,   Qt::AlignRight,  true,  "projqtyper"  );
  _womatlvar->addColumn(tr("Act. Iss."),      _qtyColumn,   Qt::AlignRight,  true,  "actiss"  );
  _womatlvar->addColumn(tr("Act. Qty. per"),  _qtyColumn,   Qt::AlignRight,  true,  "actqtyper"  );
  _womatlvar->addColumn(tr("Qty. per Var."),  _qtyColumn,   Qt::AlignRight,  true,  "qtypervar"  );
  _womatlvar->addColumn(tr("%"),              _prcntColumn, Qt::AlignRight,  true,  "qtypervarpercent"  );
  _womatlvar->addColumn(tr("Notes"),              -1,       Qt::AlignLeft,   false, "womatlvar_notes");
  _womatlvar->addColumn(tr("Reference"), -1,       Qt::AlignLeft,   false, "womatlvar_ref");
}


/*
 *  Destroys the object and frees any allocated resources
 */
dspMaterialUsageVarianceByComponentItem::~dspMaterialUsageVarianceByComponentItem()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void dspMaterialUsageVarianceByComponentItem::languageChange()
{
  retranslateUi(this);
}

void dspMaterialUsageVarianceByComponentItem::sPrint()
{
  ParameterList params;
  _warehouse->appendValue(params);
  _dates->appendValue(params);
  params.append("item_id", _item->id());

  orReport report("MaterialUsageVarianceByComponentItem", params);
  if(report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspMaterialUsageVarianceByComponentItem::sPopulateMenu(QMenu *)
{
}

void dspMaterialUsageVarianceByComponentItem::sFillList()
{
  if ((_item->isValid()) && (_dates->allValid()))
  {
    QString sql( "SELECT womatlvar_id, posted, item_number,"
                 "       ordered, received,"
                 "       projreq, projqtyper,"
                 "       actiss, actqtyper,"
                 "       (actqtyper - projqtyper) AS qtypervar,"
                 "       CASE WHEN (actqtyper=projqtyper) THEN 0"
                 "            WHEN (projqtyper=0) THEN actqtyper"
                 "            ELSE ((1 - (actqtyper / projqtyper)) * -1)"
                 "       END AS qtypervarpercent,"
                 "       womatlvar_notes, womatlvar_ref,"
                 "       'qty' AS ordered_xtnumericrole,"
                 "       'qty' AS received_xtnumericrole,"
                 "       'qty' AS projreq_xtnumericrole,"
                 "       'qtyper' AS projqtyper_xtnumericrole,"
                 "       'qty' AS actiss_xtnumericrole,"
                 "       'qtyper' AS actqtyper_xtnumericrole,"
                 "       'qtyper' AS qtypervar_xtnumericrole,"
                 "       'percent' AS qtypervarpercent_xtnumericrole "
                 "FROM ( SELECT womatlvar_id, womatlvar_posted AS posted, item_number,"
                 "              womatlvar_notes, womatlvar_ref,"
                 "              womatlvar_qtyord AS ordered, womatlvar_qtyrcv AS received,"
                 "              (womatlvar_qtyrcv * (womatlvar_qtyper * (1 + womatlvar_scrap))) AS projreq,"
                 "              womatlvar_qtyper AS projqtyper,"
                 "              (womatlvar_qtyiss) AS actiss, (womatlvar_qtyiss / (womatlvar_qtyrcv * (1 + womatlvar_scrap))) AS actqtyper "
                 "       FROM womatlvar, itemsite AS component, itemsite AS parent, item "
                 "       WHERE ((womatlvar_parent_itemsite_id=parent.itemsite_id)"
                 "        AND (womatlvar_component_itemsite_id=component.itemsite_id)"
                 "        AND (parent.itemsite_item_id=item_id)"
                 "        AND (component.itemsite_item_id=:item_id)"
                 "        AND (womatlvar_posted BETWEEN :startDate AND :endDate)" );

    if (_warehouse->isSelected())
      sql += " AND (component.itemsite_warehous_id=:warehous_id)";

    sql += ") ) AS data "
           "ORDER BY posted";

    q.prepare(sql);
    _warehouse->bindValue(q);
    _dates->bindValue(q);
    q.bindValue(":item_id", _item->id());
    q.exec();
    _womatlvar->populate(q);
  }
}

