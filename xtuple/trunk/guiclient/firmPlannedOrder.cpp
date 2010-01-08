/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "firmPlannedOrder.h"

#include <QSqlError>
#include <QValidator>
#include <QVariant>

firmPlannedOrder::firmPlannedOrder(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_firm, SIGNAL(clicked()), this, SLOT(sFirm()));
  _quantity->setValidator(omfgThis->qtyVal());

  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
}

firmPlannedOrder::~firmPlannedOrder()
{
  // no need to delete child widgets, Qt does it all for us
}

void firmPlannedOrder::languageChange()
{
  retranslateUi(this);
}

enum SetResponse firmPlannedOrder::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("planord_id", &valid);
  if (valid)
  {
    _planordid = param.toInt();
    _item->setReadOnly(TRUE);

    q.prepare( "SELECT planord.*,"
               "       itemsite_leadtime "
               "FROM planord JOIN itemsite ON (itemsite_id=itemsite_id) "
               "WHERE (planord_id=:planord_id);" );
    q.bindValue(":planord_id", _planordid);
    q.exec();
    if (q.first())
    {
      _item->setItemsiteid(q.value("planord_itemsite_id").toInt());
      _quantity->setDouble(q.value("planord_qty").toDouble());
      _dueDate->setDate(q.value("planord_duedate").toDate());
      _comments->setText(q.value("planord_comments").toString());
      _number = q.value("planord_number").toInt();
      _itemsiteid = q.value("planord_itemsite_id").toInt();
      _leadTime = q.value("itemsite_leadtime").toInt();

      _type = q.value("planord_type").toString();  
      if (q.value("planord_type").toString() == "P")
        _orderType->setText(tr("Purchase Order"));
      else if (q.value("planord_type").toString() == "W")
        _orderType->setText(tr("Work Order"));
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      reject();
    }
  }

  return NoError;
}

void firmPlannedOrder::sFirm()
{
  q.prepare( "SELECT deletePlannedOrder( :planord_id, true) AS result;" );
  q.bindValue(":planord_id", _planordid);
  q.exec();
  if (q.first())
  {
    bool result = q.value("result").toBool();
    if (! result)
    {
      systemError(this, tr("DeletePlannedOrder returned FALSE, indicating an "
                           "error occurred."),
                  __FILE__, __LINE__);
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  q.prepare( "SELECT createPlannedOrder( -1, :orderNumber, :itemsite_id, :qty,"
             "                           (DATE(:dueDate) - :leadTime), :dueDate,"
             "                           TRUE, FALSE, NULL, :itemType) AS result;" );
  q.bindValue(":orderNumber", _number);
  q.bindValue(":itemsite_id", _itemsiteid);
  q.bindValue(":qty", _quantity->toDouble());
  q.bindValue(":dueDate", _dueDate->date());
  q.bindValue(":leadTime", _leadTime);
  if (_type == "P")
    q.bindValue(":itemType", "P");
  else
    q.bindValue(":itemType", "M");
  q.exec();
  if (q.first())
  {
    double result = q.value("result").toDouble();
    if (result < 0.0)
    {
      systemError(this, tr("CreatePlannedOrder returned %, indicating an "
                           "error occurred.").arg(result),
                  __FILE__, __LINE__);
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  q.prepare( "UPDATE planord "
             "SET planord_comments=:planord_comments, planord_firm=TRUE "
             "WHERE (planord_number=:orderNumber);" );
  q.bindValue(":planord_comments", _comments->toPlainText());
  q.bindValue(":orderNumber", _number);
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(_planordid);
}
