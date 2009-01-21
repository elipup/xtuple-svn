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

#include "recallOrders.h"

#include <QMessageBox>
#include <QSqlError>
//#include <QStatusBar>
#include <QVariant>

#include <metasql.h>
#include <parameter.h>
#include "storedProcErrorLookup.h"

recallOrders::recallOrders(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_recall,	   SIGNAL(clicked()),	  this, SLOT(sRecall()));
  connect(_showInvoiced, SIGNAL(toggled(bool)), this, SLOT(sFillList()));

  _showInvoiced->setEnabled(_privileges->check("RecallInvoicedShipment"));
  
  _ship->addColumn(tr("Ship Date"),	_dateColumn,  Qt::AlignCenter, true, "shiphead_shipdate" );
  _ship->addColumn(tr("Order #"),	_orderColumn, Qt::AlignLeft  , true, "number");
  _ship->addColumn(tr("Shipment #"),    _orderColumn, Qt::AlignLeft  , true, "shiphead_number");
  _ship->addColumn(tr("Customer"),      -1,           Qt::AlignLeft  , true, "cohead_billtoname" );
  _ship->addColumn(tr("Invoiced"),	_ynColumn,    Qt::AlignCenter, true, "shipitem_invoiced" );

  sFillList();
}

recallOrders::~recallOrders()
{
  // no need to delete child widgets, Qt does it all for us
}

void recallOrders::languageChange()
{
  retranslateUi(this);
}

void recallOrders::sRecall()
{
  if (!checkSitePrivs(_ship->id()))
    return;
    
  q.prepare("SELECT recallShipment(:shiphead_id) AS result;");
  q.bindValue(":shiphead_id", _ship->id());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("recallShipment", result),
		  __FILE__, __LINE__);
      return;
    }
    sFillList();
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void recallOrders::sFillList()
{
  ParameterList params;

  if (_showInvoiced->isChecked())
    params.append("showInvoiced", true);
  if (_metrics->boolean("MultiWhs"))
    params.append("MultiWhs");

  QString sql = "SELECT DISTINCT shiphead_id, shiphead_shipdate, "
                "       cohead_number AS number, shiphead_number, cohead_billtoname, "
                "       shipitem_invoiced "
                "FROM shiphead JOIN shipitem ON (shipitem_shiphead_id=shiphead_id)"
                "              JOIN coitem ON (coitem_id=shipitem_orderitem_id)"
                "              JOIN cohead ON (cohead_id=coitem_cohead_id)"
                "              JOIN itemsite ON (itemsite_id=coitem_itemsite_id)"
                "              JOIN site() ON (warehous_id=itemsite_warehous_id)"
                "              LEFT OUTER JOIN invcitem ON (invcitem_id=shipitem_invcitem_id)"
                "              LEFT OUTER JOIN invchead ON (invchead_id=invcitem_invchead_id) "
                "WHERE ( (shiphead_shipped)"
                "  AND   (shiphead_order_type='SO')"
                "<? if exists(\"showInvoiced\") ?>"
                "  AND   (NOT shipitem_invoiced OR invchead_posted=FALSE)"
                "<? else ?>"
                "  AND   (NOT shipitem_invoiced) "
                "<? endif ?>"
                " ) "
                "<? if exists(\"MultiWhs\") ?>"
                "UNION "
                "SELECT DISTINCT shiphead_id, shiphead_shipdate, "
                "       tohead_number AS number, shiphead_number, '' AS cohead_billtoname, "
                "       false AS shipitem_invoiced "
                "FROM shiphead, tohead, toitem "
                "WHERE ((toitem_tohead_id=tohead_id)"
                "  AND  (shiphead_order_id=tohead_id)"
                "  AND  (shiphead_shipped)"
                "  AND  (shiphead_order_type='TO')) "
                "<? endif ?>"
                "ORDER BY shiphead_shipdate DESC, number;" ;
  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  _ship->clear();
  _ship->populate(q);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

bool recallOrders::checkSitePrivs(int orderid)
{
  if (_preferences->boolean("selectedSites"))
  {
    XSqlQuery check;
    check.prepare("SELECT checkShipmentSitePrivs(:shipheadid) AS result;");
    check.bindValue(":shipheadid", orderid);
    check.exec();
    if (check.first())
    {
      if (!check.value("result").toBool())
      {
        QMessageBox::critical(this, tr("Access Denied"),
                                       tr("You may not recall this Shipment as it references "
                                       "a Site for which you have not been granted privileges.")) ;
        return false;
      }
    }
  }
  return true;
}
