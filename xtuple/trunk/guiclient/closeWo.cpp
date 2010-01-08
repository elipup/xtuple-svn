/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "closeWo.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include "inputManager.h"

closeWo::closeWo(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_closeWo, SIGNAL(clicked()), this, SLOT(sCloseWo()));

  _captive = FALSE;

  omfgThis->inputManager()->notify(cBCWorkOrder, this, _wo, SLOT(setId(int)));

  _wo->setType(cWoOpen | cWoExploded | cWoReleased | cWoIssued);
  
  _postMaterialVariance->setChecked(_metrics->boolean("PostMaterialVariances"));
}

closeWo::~closeWo()
{
    // no need to delete child widgets, Qt does it all for us
}

void closeWo::languageChange()
{
    retranslateUi(this);
}

enum SetResponse closeWo::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("wo_id", &valid);
  if (valid)
  {
    _wo->setId(param.toInt());
    _wo->setReadOnly(TRUE);
    _closeWo->setFocus();
  }

  return NoError;
}

bool closeWo::okToSave()
{
  XSqlQuery tool;
  tool.prepare( "SELECT (count(womatl_id) > 0) AS disttool"
                " FROM womatl "
                "  JOIN itemsite ON (womatl_itemsite_id = itemsite_id) "
                "  JOIN item ON (itemsite_item_id = item_id) "
                " WHERE ((womatl_wo_id=:wo_id)"
                " AND (item_type = 'T' ) "
                " AND (womatl_qtyiss > 0 ) "
                " AND ((itemsite_controlmethod IN ('L','S') "
                "      OR (itemsite_loccntrl)))); ");
  tool.bindValue(":wo_id", _wo->id());
  tool.exec();
  if (tool.first())
  {
    if (tool.value("disttool").toBool())
    {
      QMessageBox::critical(this, tr("Controlled Tool"),
                            tr("<p>A controlled tooling item still resides on this work order. "
                               "You must return the tooling item before closing it.") );
      _wo->setFocus();
      return false;
    }
  }

  XSqlQuery type;
  type.prepare( "SELECT item_type "
                "FROM item,itemsite,wo "
                "WHERE ((wo_id=:wo_id) "
                "AND (wo_itemsite_id=itemsite_id) "
                "AND (itemsite_item_id=item_id)); ");
  type.bindValue(":wo_id", _wo->id());
  type.exec();
  if (type.first())
  {
    if (type.value("item_type").toString() == "J")
    {
      QMessageBox::critical(this, tr("Invalid Work Order"),
                            tr("<p>Work Orders of Item Type Job are posted "
                               "when shipping the Sales Order they are "
                               "associated with.") );
      _wo->setFocus();
      return false;
    }
    else
    {
      q.prepare("SELECT wo_qtyrcv, womatl_issuemethod, womatl_qtyiss "
                "FROM wo, womatl "
                "WHERE ((womatl_wo_id=wo_id)"
                "   AND (wo_id=:wo_id));" );
      q.bindValue(":wo_id", _wo->id());
      q.exec();
      if (q.first())
      {
        if (q.value("wo_qtyrcv").toDouble() == 0.0)
          QMessageBox::warning(this, tr("No Production Posted"),
                               tr("<p>There has not been any Production "
                                  "received from this Work Order. This "
                                  "probably means Production Postings for "
                                  "this Work Order have been overlooked." ));

        bool unissuedMaterial = FALSE;
        bool unpushedMaterial = FALSE;
        do
        {
          if (! unissuedMaterial &&
              (q.value("womatl_issuemethod") == "S") &&
              (q.value("womatl_qtyiss").toDouble() == 0.0) )
          {
            QMessageBox::warning(this, tr("Unissued Push Items"),
                                 tr("<p>The selected Work Order has Material "
                                    "Requirements that are Push-Issued but "
                                    "have not had any material issued to them. "
                                    "This probably means that required manual "
                                    "material issues have been overlooked."));
            unissuedMaterial = TRUE;
          }
          else if (! unpushedMaterial &&
                   ( (q.value("womatl_issuemethod") == "L") ||
                     (q.value("womatl_issuemethod") == "M") ) &&
                   (q.value("womatl_qtyiss").toDouble() == 0.0) )
          {
            QMessageBox::warning(this, tr("Unissued Pull Items"),
                                 tr("<p>The selected Work Order has Material "
                                    "Requirements that are Pull-Issued but "
                                    "have not had any material issued to them. "
                                    "This probably means that Production was "
                                    "posted for this Work Order through "
                                    "posting Operations. The BOM for this Item "
                                    "should be modified to list Used At "
                                    "selections for each BOM Item." ) );
            unpushedMaterial = TRUE;
          }
        }
        while (q.next());
      }
      else if (q.lastError().type() != QSqlError::NoError)
      {
        systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
        return false;
      }

      return true;      // this is the only successful case
    }
  }
  else if (type.lastError().type() != QSqlError::None)
  {
    systemError(this, type.lastError().databaseText(), __FILE__, __LINE__);
    return false;
  }
  else // type not found
  {
    systemError(this, tr("Cannot close Work Order %1 because the Item does not "
                         "appear to have an Item Type! Please check the Item "
                         "definition. You may need to reset the Type and save "
                         "the Item record.").arg(_wo->woNumber()));
    return false;
  }

  return false;
}

void closeWo::sCloseWo()
{
  if (okToSave() &&
      QMessageBox::question(this, tr("Close Work Order"),
                            tr("<p>Are you sure you want to close this "
                               "Work Order?"),
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
  {
    q.prepare("SELECT closeWo(:wo_id, :postMatVar);");
    q.bindValue(":wo_id", _wo->id());
    q.bindValue(":postMatVar",   QVariant(_postMaterialVariance->isChecked()));
    q.exec();
    if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    omfgThis->sWorkOrdersUpdated(_wo->id(), TRUE);

    if (_captive)
      close();
    else
    {
      clear();
    }
  }
}

void closeWo::clear()
{
  _wo->setId(-1);
  _close->setText(tr("&Close"));
  _wo->setFocus();
}
