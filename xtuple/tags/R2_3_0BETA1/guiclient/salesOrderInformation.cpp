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

#include "salesOrderInformation.h"

#include <qvariant.h>

/*
 *  Constructs a salesOrderInformation as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
salesOrderInformation::salesOrderInformation(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
salesOrderInformation::~salesOrderInformation()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void salesOrderInformation::languageChange()
{
    retranslateUi(this);
}

void salesOrderInformation::init()
{
    _soitemid = 0;
}

enum SetResponse salesOrderInformation::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("soitem_id", &valid);
  if (valid)
  {
    _soitemid = param.toInt();
    q.prepare( "SELECT coitem_cohead_id "
               "FROM coitem "
               "WHERE (coitem_id=:soitem_id);" );
    q.bindValue(":soitem_id", _soitemid);
    q.exec();
    if (q.first())
    {
      _soheadid = q.value("coitem_cohead_id").toInt();
      populate();
    }
  }

  param = pParams.value("sohead_id", &valid);
  if (valid)
  {
    _soheadid = param.toInt();
    populate();
  }

  return NoError;
}

void salesOrderInformation::populate()
{
  q.prepare( "SELECT cohead_number, warehous_code,"
             "       formatDate(cohead_orderdate) AS f_orderdate,"
             "       formatDate(MIN(coitem_scheddate)) AS f_shipdate,"
             "       formatDate(cohead_packdate) AS f_packdate,"
             "       CASE WHEN (cohead_holdtype='N') THEN :none"
             "            WHEN (cohead_holdtype='C') THEN :credit"
             "            WHEN (cohead_holdtype='S') THEN :ship"
             "            WHEN (cohead_holdtype='P') THEN :pack"
             "            ELSE :other"
             "       END AS f_holdtype,"
             "       cohead_shipvia, cohead_billtoname,"
             "       cohead_billtoaddress1, cohead_billtoaddress2, cohead_billtoaddress3,"
             "       cohead_billtocity, cohead_billtostate, cohead_billtozipcode,"
             "       cohead_shiptoname,"
             "       cohead_shiptoaddress1, cohead_shiptoaddress2, cohead_shiptoaddress3,"
             "       cohead_shiptocity, cohead_shiptostate, cohead_shiptozipcode "
             "FROM cohead, coitem, itemsite LEFT OUTER JOIN "
	     "     warehous ON (itemsite_warehous_id = warehous_id) "
             "WHERE ( (coitem_cohead_id=cohead_id)"
	     " AND (coitem_itemsite_id=itemsite_id) "
             " AND (coitem_status <> 'X')"
             " AND (coitem_id=:soitem_id) "
             " AND (cohead_id=:sohead_id) ) "
             "GROUP BY cohead_number, warehous_code, cohead_orderdate, cohead_packdate,"
             "         cohead_holdtype, cohead_shipvia, cohead_billtoname,"
             "         cohead_billtoaddress1, cohead_billtoaddress2, cohead_billtoaddress3,"
             "         cohead_billtocity, cohead_billtostate, cohead_billtozipcode,"
             "         cohead_shiptoname,"
             "         cohead_shiptoaddress1, cohead_shiptoaddress2, cohead_shiptoaddress3,"
             "         cohead_shiptocity, cohead_shiptostate, cohead_shiptozipcode;" );
  q.bindValue(":none", tr("None"));
  q.bindValue(":credit", tr("Credit"));
  q.bindValue(":ship", tr("Ship"));
  q.bindValue(":pack", tr("Pack"));
  q.bindValue(":other", tr("Other"));
  q.bindValue(":sohead_id", _soheadid);
  q.bindValue(":soitem_id", _soitemid);
  q.exec();
  if (q.first())
  {
    _orderNumber->setText(q.value("cohead_number").toString());
    _warehouse->setText(q.value("warehous_code").toString());
    _orderDate->setText(q.value("f_orderdate").toString());
    _shipDate->setText(q.value("f_shipdate").toString());
    _packDate->setText(q.value("f_packdate").toString());
    _shipVia->setText(q.value("cohead_shipvia").toString());
    _holdType->setText(q.value("f_holdtype").toString());

    _billtoName->setText(q.value("cohead_billtoname").toString());
    _billtoAddress1->setText(q.value("cohead_billtoaddress1").toString());
    _billtoAddress2->setText(q.value("cohead_billtoaddress2").toString());
    _billtoAddress3->setText(q.value("cohead_billtoaddress3").toString());
    _billtoCity->setText(q.value("cohead_billtocity").toString());
    _billtoState->setText(q.value("cohead_billtostate").toString());
    _billtoZipCode->setText(q.value("cohead_billtozipcode").toString());

    _shiptoName->setText(q.value("cohead_shiptoname").toString());
    _shiptoAddress1->setText(q.value("cohead_shiptoaddress1").toString());
    _shiptoAddress2->setText(q.value("cohead_shiptoaddress2").toString());
    _shiptoAddress3->setText(q.value("cohead_shiptoaddress3").toString());
    _shiptoCity->setText(q.value("cohead_shiptocity").toString());
    _shiptoState->setText(q.value("cohead_shiptostate").toString());
    _shiptoZipCode->setText(q.value("cohead_shiptozipcode").toString());
  }
}

