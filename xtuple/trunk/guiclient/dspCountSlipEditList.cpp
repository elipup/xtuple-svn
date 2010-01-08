/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspCountSlipEditList.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>
#include "countTagList.h"
#include "countSlip.h"

dspCountSlipEditList::dspCountSlipEditList(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_post, SIGNAL(clicked()), this, SLOT(sPost()));
  connect(_cntslip, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));
  connect(_countTagList, SIGNAL(clicked()), this, SLOT(sCountTagList()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_postAll, SIGNAL(clicked()), this, SLOT(sPostAll()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));

#ifndef Q_WS_MAC
  _countTagList->setMaximumWidth(25);
#endif

  _item->setReadOnly(TRUE);

  _cntslip->addColumn(tr("User"),    _dateColumn, Qt::AlignCenter,true, "user");
  _cntslip->addColumn(tr("#"),       _itemColumn, Qt::AlignLeft,  true, "cntslip_number");
  _cntslip->addColumn(tr("Location"),_itemColumn, Qt::AlignLeft,  true, "locname");
  _cntslip->addColumn(tr("Lot/Serial #"),     -1, Qt::AlignLeft,  true, "cntslip_lotserial");
  _cntslip->addColumn(tr("Posted"),    _ynColumn, Qt::AlignCenter,true, "cntslip_posted");
  _cntslip->addColumn(tr("Entered"), _itemColumn, Qt::AlignCenter,true, "cntslip_entered");
  _cntslip->addColumn(tr("Slip Qty."),_qtyColumn, Qt::AlignRight, true, "cntslip_qty");

  if (_privileges->check("EnterCountSlips"))
  {
    connect(_cntslip, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_item, SIGNAL(valid(bool)), _new, SLOT(setEnabled(bool)));
    connect(_cntslip, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }

  if (_privileges->check("DeleteCountSlips"))
    connect(_cntslip, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));

  if (_privileges->check("PostCountSlips"))
  {
    connect(_cntslip, SIGNAL(valid(bool)), _post, SLOT(setEnabled(bool)));
    _postAll->setEnabled(TRUE);
  }
    
  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
}

dspCountSlipEditList::~dspCountSlipEditList()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspCountSlipEditList::languageChange()
{
  retranslateUi(this);
}

enum SetResponse dspCountSlipEditList::set(const ParameterList &pParams)
{
  XWidget::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("cnttag_id", &valid);
  if (valid)
  {
    _cnttagid = param.toInt();
    populate();
  }

  return NoError;
}

void dspCountSlipEditList::sPrint()
{
  ParameterList params;
  params.append("cnttag_id", _cnttagid);

  orReport report("CountSlipEditList", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspCountSlipEditList::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pSelected)
{
  int menuItem;

  menuItem = pMenu->insertItem("Edit Count Slip...", this, SLOT(sEdit()), 0);
  if (!_privileges->check("EnterCountSlips"))
    pMenu->setItemEnabled(menuItem, FALSE);

  if (((XTreeWidgetItem *)pSelected)->altId() == 0)
  {
    menuItem = pMenu->insertItem("Post Count Slip...", this, SLOT(sPost()), 0);
    if (!_privileges->check("PostCountSlips"))
      pMenu->setItemEnabled(menuItem, FALSE);
  }
}

void dspCountSlipEditList::sNew()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("cnttag_id", _cnttagid);

  countSlip newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspCountSlipEditList::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("cntslip_id", _cntslip->id());

  countSlip newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspCountSlipEditList::sDelete()
{
  if (QMessageBox::question( this, tr("Delete Count Slip?"),
                            tr("Are you sure that you want to delete the selected Count Slip?"),
                            QMessageBox::Yes,
                            QMessageBox::No | QMessageBox::Default) == QMessageBox::Yes)
  {
    q.prepare( "DELETE FROM cntslip "
               " WHERE(cntslip_id=:cntslip_id);" );
    q.bindValue(":cntslip_id", _cntslip->id());
    q.exec();

    sFillList();
  }
}

void dspCountSlipEditList::sPost()
{
  q.prepare("SELECT postCountSlip(:cntslip_id);");
  q.bindValue(":cntslip_id", _cntslip->id());
  q.exec();

  sFillList();
}

void dspCountSlipEditList::sPostAll()
{
  q.prepare( "SELECT postCountSlip(cntslip_id) "
             "FROM cntslip "
             "WHERE ( (NOT cntslip_posted)"
             " AND (cntslip_cnttag_id=:cnttag_id) );" );
  q.bindValue(":cnttag_id", _cnttagid);
  q.exec();

  sFillList();
}

void dspCountSlipEditList::sCountTagList()
{
  ParameterList params;
  params.append("cnttag_id", _cnttagid);
  params.append("tagType", cUnpostedCounts);

  countTagList newdlg(this, "", TRUE);
  newdlg.set(params);
  _cnttagid = newdlg.exec();

  populate();
}

void dspCountSlipEditList::populate()
{
  q.prepare( "SELECT invcnt_tagnumber, invcnt_itemsite_id "
             "FROM invcnt "
             "WHERE (invcnt_id=:cnttag_id);" );
  q.bindValue(":cnttag_id", _cnttagid);
  q.exec();
  if (q.first())
  {
    _countTagNumber->setText(q.value("invcnt_tagnumber").toString());
    _item->setItemsiteid(q.value("invcnt_itemsite_id").toInt());
  }

  sFillList();
}

void dspCountSlipEditList::sFillList()
{
  q.prepare( "SELECT cntslip_id,"
             "       CASE WHEN (cntslip_posted) THEN 1"
             "            ELSE 0"
             "       END,"
             "       cntslip_username AS user, cntslip_number,"
             "       CASE WHEN (cntslip_location_id=-1) THEN ''"
             "            ELSE formatLocationName(cntslip_location_id)"
             "       END AS locname,"
             "       cntslip_lotserial, cntslip_posted,"
             "       cntslip_entered, cntslip_qty,"
             "       'qty' AS cntslip_qty_xtnumericrole "
             "FROM cntslip, invcnt "
             "WHERE ( (cntslip_cnttag_id=invcnt_id)"
             "    AND (NOT invcnt_posted)"
             "    AND (invcnt_id=:cnttag_id) ) "
             "ORDER BY cntslip_number;" );
  q.bindValue(":cnttag_id", _cnttagid);
  q.exec();
  _cntslip->populate(q, TRUE);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

