/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspFreightPricesByCustomer.h"

#include <openreports.h>
#include <parameter.h>

dspFreightPricesByCustomer::dspFreightPricesByCustomer(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));

  _price->addColumn(tr("Schedule"),      _itemColumn,     Qt::AlignLeft,   true,  "ipshead_name"  );
  _price->addColumn(tr("Source"),        _itemColumn,     Qt::AlignLeft,   true,  "source"  );
  _price->addColumn(tr("Qty. Break"),    _qtyColumn,      Qt::AlignRight,  true,  "ipsfreight_qtybreak" );
  _price->addColumn(tr("Price"),         _priceColumn,    Qt::AlignRight,  true,  "ipsfreight_price" );
  _price->addColumn(tr("Method"),        _itemColumn,     Qt::AlignLeft,   true,  "method"  );
  _price->addColumn(tr("Currency"),      _currencyColumn, Qt::AlignLeft,   true,  "currConcat");
  _price->addColumn(tr("From"),          _itemColumn,     Qt::AlignLeft,   true,  "warehous_code"  );
  _price->addColumn(tr("To"),            _itemColumn,     Qt::AlignLeft,   true,  "shipzone_name"  );
  _price->addColumn(tr("Freight Class"), _itemColumn,     Qt::AlignLeft,   true,  "freightclass_code"  );
  _price->addColumn(tr("Ship Via"),      _itemColumn,     Qt::AlignLeft,   true,  "ipsfreight_shipvia"  );

}

dspFreightPricesByCustomer::~dspFreightPricesByCustomer()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspFreightPricesByCustomer::languageChange()
{
  retranslateUi(this);
}

void dspFreightPricesByCustomer::sPrint()
{
  ParameterList params;

  params.append("cust_id", _cust->id());

  if(_showExpired->isChecked())
    params.append("showExpired");
  if(_showFuture->isChecked())
    params.append("showFuture");

  orReport report("FreightPricesByCustomer", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspFreightPricesByCustomer::sFillList()
{
  _price->clear();

  if (_cust->isValid())
  {
    QString sql = "SELECT itemid, sourcetype, ipshead_name, source, ipsfreight_qtybreak, ipsfreight_price,"
                  "       CASE WHEN (ipsfreight_type = 'F') THEN :flatrate"
                  "            ELSE :peruom"
                  "       END AS method,"
                  "       currConcat(ipshead_curr_id) AS currConcat,"
                  "       warehous_code, shipzone_name, freightclass_code, ipsfreight_shipvia,"
                  "       'qty' AS ipsfreight_qtybreak_xtnumericrole,"
                  "       :na AS ipsfreight_qtybreak_xtnullrole,"
                  "       'salesprice' AS ipsfreight_price_xtnumericrole,"
                  "       :any AS warehous_code_xtnullrole,"
                  "       :any AS shipzone_name_xtnullrole,"
                  "       :any AS freightclass_code_xtnullrole,"
                  "       :any AS ipsfreight_shipvia_xtnullrole ";

    sql += "FROM ( SELECT ipsfreight_id AS itemid, 1 AS sourcetype,"
           "              ipshead_name, :customer AS source,"
           "              ipsfreight_qtybreak, ipsfreight_price,"
           "              ipsfreight_type, ipshead_curr_id,"
           "              warehous_code, shipzone_name, freightclass_code, ipsfreight_shipvia "
           "FROM ipsass JOIN ipshead ON (ipshead_id=ipsass_ipshead_id)"
           "            JOIN ipsfreight ON (ipsfreight_ipshead_id=ipshead_id)"
           "            LEFT OUTER JOIN whsinfo ON (warehous_id=ipsfreight_warehous_id)"
           "            LEFT OUTER JOIN shipzone ON (shipzone_id=ipsfreight_shipzone_id)"
           "            LEFT OUTER JOIN freightclass ON (freightclass_id=ipsfreight_freightclass_id) "
           "WHERE ( (ipsass_cust_id=:cust_id)"
           " AND (COALESCE(LENGTH(ipsass_shipto_pattern), 0) = 0) ";

    if (!_showExpired->isChecked())
      sql += " AND (ipshead_expires > CURRENT_DATE)";

    if (!_showFuture->isChecked())
      sql += " AND (ipshead_effective <= CURRENT_DATE)";

    sql += ") "
           "UNION SELECT ipsfreight_id AS itemid, 2 AS sourcetype,"
           "             ipshead_name, :custType AS source,"
           "             ipsfreight_qtybreak, ipsfreight_price,"
           "             ipsfreight_type, ipshead_curr_id,"
           "             warehous_code, shipzone_name, freightclass_code, ipsfreight_shipvia "
           "FROM ipsass JOIN ipshead ON (ipshead_id=ipsass_ipshead_id)"
           "            JOIN ipsfreight ON (ipsfreight_ipshead_id=ipshead_id)"
           "            JOIN cust ON (cust_custtype_id=ipsass_custtype_id)"
           "            LEFT OUTER JOIN whsinfo ON (warehous_id=ipsfreight_warehous_id)"
           "            LEFT OUTER JOIN shipzone ON (shipzone_id=ipsfreight_shipzone_id)"
           "            LEFT OUTER JOIN freightclass ON (freightclass_id=ipsfreight_freightclass_id) "
           "WHERE ( (cust_id=:cust_id) ";
                  
    if (!_showExpired->isChecked())
      sql += " AND (ipshead_expires > CURRENT_DATE)";

    if (!_showFuture->isChecked())
      sql += " AND (ipshead_effective <= CURRENT_DATE)";

    sql += ") "
           "UNION SELECT ipsfreight_id AS itemid, 3 AS sourcetype,"
           "             ipshead_name, :custTypePattern AS source,"
           "             ipsfreight_qtybreak, ipsfreight_price,"
           "             ipsfreight_type, ipshead_curr_id,"
           "             warehous_code, shipzone_name, freightclass_code, ipsfreight_shipvia "
           "FROM cust   JOIN custtype ON (custtype_id=cust_custtype_id)"
           "            JOIN ipsass ON ((coalesce(length(ipsass_custtype_pattern), 0) > 0) AND"
           "                            (custtype_code ~ ipsass_custtype_pattern))"
           "            JOIN ipshead ON (ipshead_id=ipsass_ipshead_id)"
           "            JOIN ipsfreight ON (ipsfreight_ipshead_id=ipshead_id)"
           "            LEFT OUTER JOIN whsinfo ON (warehous_id=ipsfreight_warehous_id)"
           "            LEFT OUTER JOIN shipzone ON (shipzone_id=ipsfreight_shipzone_id)"
           "            LEFT OUTER JOIN freightclass ON (freightclass_id=ipsfreight_freightclass_id) "
           "WHERE ( (cust_id=:cust_id) ";
                  
    if (!_showExpired->isChecked())
      sql += " AND (ipshead_expires > CURRENT_DATE)";

    if (!_showFuture->isChecked())
      sql += " AND (ipshead_effective <= CURRENT_DATE)";

    sql += ") "
           "UNION SELECT ipsfreight_id AS itemid, 4 AS sourcetype,"
           "             ipshead_name, (:sale || '-' || sale_name) AS source,"
           "             ipsfreight_qtybreak, ipsfreight_price,"
           "             ipsfreight_type, ipshead_curr_id,"
           "             warehous_code, shipzone_name, freightclass_code, ipsfreight_shipvia "
           "FROM sale JOIN ipshead ON (ipshead_id=sale_ipshead_id)"
           "          JOIN ipsfreight ON (ipsfreight_ipshead_id=ipshead_id)"
           "          LEFT OUTER JOIN whsinfo ON (warehous_id=ipsfreight_warehous_id)"
           "          LEFT OUTER JOIN shipzone ON (shipzone_id=ipsfreight_shipzone_id)"
           "          LEFT OUTER JOIN freightclass ON (freightclass_id=ipsfreight_freightclass_id) "
           "WHERE ((TRUE)";
                  
    if (!_showExpired->isChecked())
      sql += " AND (sale_enddate > CURRENT_DATE)";

    if (!_showFuture->isChecked())
      sql += " AND (sale_startdate <= CURRENT_DATE)";

    sql += ") ) AS data "
           "ORDER BY ipsfreight_qtybreak, ipsfreight_price;";

    q.prepare(sql);
    q.bindValue(":na", tr("N/A"));
    q.bindValue(":any", tr("Any"));
    q.bindValue(":flatrate", tr("Flat Rate"));
    q.bindValue(":peruom", tr("Per UOM"));
    q.bindValue(":customer", tr("Customer"));
    q.bindValue(":custType", tr("Cust. Type"));
    q.bindValue(":custTypePattern", tr("Cust. Type Pattern"));
    q.bindValue(":sale", tr("Sale"));
    q.bindValue(":cust_id", _cust->id());
    q.exec();
    _price->populate(q, true);
  }
}
