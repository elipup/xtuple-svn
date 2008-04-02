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

#include "taxSelection.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QSqlError>


taxSelection::taxSelection(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : QDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_save,	SIGNAL(clicked()), this, SLOT(sSave()));

  _mode		= cNew;
  _taxselId	= -1;
}

taxSelection::~taxSelection()
{
  // no need to delete child widgets, Qt does it all for us
}

void taxSelection::languageChange()
{
  retranslateUi(this);
}

enum SetResponse taxSelection::set(const ParameterList& pParams)
{
  QVariant	param;
  bool		valid;

  param = pParams.value("taxsel_id", &valid);
  if (valid)
  {
    _taxselId = param.toInt();
    sPopulate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _taxauth->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
      _tax->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;
      _taxauth->setEnabled(false);
      _taxtype->setEnabled(false);
      _tax->setEnabled(false);
      _save->hide();
      _close->setText(tr("Close"));
      _close->setFocus();
    }
  }

  return NoError;
}

void taxSelection::sSave()
{
  if (cNew == _mode)
  {
    q.prepare("INSERT INTO taxsel ("
	      "    taxsel_taxauth_id,  taxsel_taxtype_id,  taxsel_tax_id "
	      "  ) VALUES ( "
	      "    :taxsel_taxauth_id, :taxsel_taxtype_id, :taxsel_tax_id "
	      ");");
  }
  else
  {
    q.prepare("UPDATE taxsel SET "
	      "    taxsel_taxauth_id=:taxsel_taxauth_id,"
	      "    taxsel_taxtype_id=:taxsel_taxtype_id,"
	      "    taxsel_tax_id=:taxsel_tax_id "
	      "WHERE (taxsel_id=:taxsel_id);");
    q.bindValue(":taxsel_id", _taxselId);
  }
  if (_taxauth->isValid())
    q.bindValue(":taxsel_taxauth_id", _taxauth->id());
  if (_taxtype->isValid())
    q.bindValue(":taxsel_taxtype_id", _taxtype->id());
  if (_tax->isValid())
    q.bindValue(":taxsel_tax_id", _tax->id());

  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  accept();
}

void taxSelection::sPopulate()
{
  q.prepare("SELECT taxsel_taxauth_id, taxsel_taxtype_id, taxsel_tax_id "
	    "FROM taxsel "
	    "WHERE (taxsel_id=:taxsel_id);");
  q.bindValue(":taxsel_id", _taxselId);
  q.exec();
  if (q.first())
  {
    if (! q.value("taxsel_taxauth_id").isNull())
      _taxauth->setId(q.value("taxsel_taxauth_id").toInt());
    if (! q.value("taxsel_taxtype_id").isNull())
      _taxtype->setId(q.value("taxsel_taxtype_id").toInt());
    if (! q.value("taxsel_tax_id").isNull())
      _tax->setId(q.value("taxsel_tax_id").toInt());
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
