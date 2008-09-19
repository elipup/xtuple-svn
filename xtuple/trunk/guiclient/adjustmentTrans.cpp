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

#include "adjustmentTrans.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

#include "distributeInventory.h"
#include "inputManager.h"
#include "storedProcErrorLookup.h"

adjustmentTrans::adjustmentTrans(QWidget* parent, Qt::WindowFlags fl)
    : XWidget(parent, fl)
{
  setupUi(this);

//  (void)statusBar();

  _adjustmentTypeGroupInt = new QButtonGroup(this);

  connect(_absolute,                   SIGNAL(toggled(bool)), this, SLOT(sPopulateQty()));
  connect(_adjustmentTypeGroupInt,SIGNAL(buttonClicked(int)), this, SLOT(sPopulateQty()));
  connect(_post,                           SIGNAL(clicked()), this, SLOT(sPost()));
  connect(_qty,          SIGNAL(textChanged(const QString&)), this, SLOT(sPopulateQty()));
  connect(_qty,          SIGNAL(textChanged(const QString&)), this, SLOT(sCostUpdated()));
  connect(_warehouse,                     SIGNAL(newID(int)), this, SLOT(sPopulateQOH(int)));
  connect(_cost, SIGNAL(textChanged(const QString&)), this, SLOT(sCostUpdated()));

  _captive = FALSE;

  _item->setType((ItemLineEdit::cGeneralInventory ^ ItemLineEdit::cBreeder) | ItemLineEdit::cActive);
  _warehouse->setType(WComboBox::AllActiveInventory);
  _afterQty->setPrecision(omfgThis->qtyVal());
  _beforeQty->setPrecision(omfgThis->qtyVal());
  _cost->setValidator(omfgThis->costVal());
  _qty->setValidator(omfgThis->transQtyVal());
  _unitCost->setPrecision(omfgThis->costVal());

  omfgThis->inputManager()->notify(cBCItem, this, _item, SLOT(setItemid(int)));
  omfgThis->inputManager()->notify(cBCItemSite, this, _item, SLOT(setItemsiteid(int)));

  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }

  if (!_metrics->boolean("AllowAvgCostMethod"))
    _tab->removeTab(0);
  _item->setFocus();
}

adjustmentTrans::~adjustmentTrans()
{
  // no need to delete child widgets, Qt does it all for us
}

void adjustmentTrans::languageChange()
{
  retranslateUi(this);
}

enum SetResponse adjustmentTrans::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;
  int      invhistid = -1;
  bool     noQty     = TRUE;

  param = pParams.value("itemsite_id", &valid);
  if (valid)
  {
    _captive = TRUE;

    _item->setItemsiteid(param.toInt());
    _item->setEnabled(FALSE);
    _warehouse->setEnabled(FALSE);
  }

  param = pParams.value("qty", &valid);
  if (valid)
  {
    _captive = TRUE;

    _qty->setDouble(param.toDouble());
    _qty->setEnabled(FALSE);
    _afterQty->setDouble(param.toDouble());
    _absolute->setChecked(TRUE);
    _adjustmentTypeGroup->setEnabled(FALSE);

    noQty = FALSE;
  }

  param = pParams.value("invhist_id", &valid);
  if (valid)
    invhistid = param.toInt();

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      setCaption(tr("Enter Miscellaneous Adjustment"));
      _usernameLit->clear();
      _transDate->setEnabled(_privileges->check("AlterTransactionDates"));
      _transDate->setDate(omfgThis->dbDate());

      if (!_item->isValid())
        _item->setFocus();
      else if (noQty)
	_qty->setFocus();
      else
        _documentNum->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      setCaption(tr("Miscellaneous Adjustment"));
      _transDate->setEnabled(FALSE);
      _item->setReadOnly(TRUE);
      _warehouse->setEnabled(FALSE);
      _adjustmentTypeGroup->setEnabled(FALSE);
      _qty->setEnabled(FALSE);
      _documentNum->setEnabled(FALSE);
      _notes->setEnabled(FALSE);
      _post->hide();
      _close->setText(tr("&Close"));
      _close->setFocus();

      q.prepare( "SELECT * "
                 "FROM invhist "
                 "WHERE (invhist_id=:invhist_id);" );
      q.bindValue(":invhist_id", invhistid);
      q.exec();
      if (q.first())
      {
        _transDate->setDate(q.value("invhist_transdate").toDate());
        _username->setText(q.value("invhist_user").toString());
        _qty->setDouble(q.value("invhist_invqty").toDouble());
        _beforeQty->setDouble(q.value("invhist_qoh_before").toDouble());
        _afterQty->setDouble(q.value("invhist_qoh_after").toDouble());
        _documentNum->setText(q.value("invhist_ordnumber"));
        _notes->setText(q.value("invhist_comments").toString());
        _item->setItemsiteid(q.value("invhist_itemsite_id").toInt());
      }
      else if (q.lastError().type() != QSqlError::None)
      {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return UndefinedError;
      }

      _close->setFocus();
    }
  }

  return NoError;
}

void adjustmentTrans::sPost()
{
  double qty = 0.0;
  double cost = _cost->toDouble();

  if (_absolute->isChecked())
    qty = (_qty->toDouble() - _cachedQOH);
  else if (_relative->isChecked())
    qty = _qty->toDouble();

  struct {
    bool        condition;
    QString     msg;
    QWidget     *widget;
  } error[] = {
    { ! _item->isValid(),
      tr("You must select an Item before posting this transaction."), _item },
    { _qty->text().length() == 0 || qty == 0,
      tr("<p>You must enter a Quantity before posting this Transaction."),
      _qty },
    { _costAdjust->isEnabled() && _costAdjust->isChecked() && _costManual->isChecked() && (_cost->text().length() == 0 || cost == 0),
      tr("<p>You must enter a total cost value for the inventory to be transacted."),
      _cost },
    { true, "", NULL }
  };
 
  int errIndex;
  for (errIndex = 0; ! error[errIndex].condition; errIndex++)
    ;
  if (! error[errIndex].msg.isEmpty())
  {
    QMessageBox::critical(this, tr("Cannot Post Transaction"),
                          error[errIndex].msg);
    error[errIndex].widget->setFocus();
    return;
  }

  XSqlQuery rollback;
  rollback.prepare("ROLLBACK;");

  q.exec("BEGIN;");	// because of possible distribution cancelations
  q.prepare( "SELECT invAdjustment(itemsite_id, :qty, :docNumber,"
             "                     :comments, :date, :cost) AS result "
             "FROM itemsite "
             "WHERE ( (itemsite_item_id=:item_id)"
             " AND (itemsite_warehous_id=:warehous_id) );" );
  q.bindValue(":qty", qty);
  q.bindValue(":docNumber", _documentNum->text());
  q.bindValue(":comments", _notes->text());
  q.bindValue(":item_id", _item->id());
  q.bindValue(":warehous_id", _warehouse->id());
  q.bindValue(":date",        _transDate->date());
  if(!_costAdjust->isChecked())
    q.bindValue(":cost", 0.0);
  else if(_costManual->isChecked())
    q.bindValue(":cost", cost);
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      rollback.exec();
      systemError(this, storedProcErrorLookup("invAdjustment", result),
                  __FILE__, __LINE__);
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    if (distributeInventory::SeriesAdjust(q.value("result").toInt(), this) == XDialog::Rejected)
    {
      rollback.exec();
      QMessageBox::information(this, tr("Inventory Adjustment"),
                               tr("Transaction Canceled") );
      return;
    }

    q.exec("COMMIT;");
    if (_captive)
      close();
    else
    {
      _close->setText(tr("&Close"));
      _item->setId(-1);
      _qty->clear();
      _documentNum->clear();
      _notes->clear();
      _beforeQty->clear();
      _afterQty->clear();
      _transDate->setDate(omfgThis->dbDate());

      _item->setFocus();
    }
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    rollback.exec();
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  else
  {
    rollback.exec();
    systemError( this,
                tr("<p>No transaction was done because Item %1 "
                   "was not found at Site %2.")
                .arg(_item->itemNumber()).arg(_warehouse->currentText()));
  }
}

void adjustmentTrans::sPopulateQOH(int pWarehousid)
{
  if (_mode != cView)
  {
    q.prepare( "SELECT itemsite_freeze, itemsite_qtyonhand, itemsite_costmethod "
               "FROM itemsite "
               "WHERE ( (itemsite_item_id=:item_id)"
               " AND (itemsite_warehous_id=:warehous_id));" );
    q.bindValue(":item_id", _item->id());
    q.bindValue(":warehous_id", pWarehousid);
    q.exec();
    if (q.first())
    {
      _cachedQOH = q.value("itemsite_qtyonhand").toDouble();
      if(_cachedQOH == 0.0)
        _costManual->setChecked(true);
      _beforeQty->setDouble(q.value("itemsite_qtyonhand").toDouble());
      _costAdjust->setChecked(true);
      _costAdjust->setEnabled(q.value("itemsite_costmethod").toString() == "A");

      if (q.value("itemsite_freeze").toBool())
        _absolute->setPaletteForegroundColor(QColor("red"));
      else
        _absolute->setPaletteForegroundColor(QColor("black"));
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      _absolute->setPaletteForegroundColor(QColor("black"));
      return;
    }
    else
      _absolute->setPaletteForegroundColor(QColor("black"));

    sPopulateQty();
  }
}

void adjustmentTrans::sPopulateQty()
{
  if (_mode == cNew)
  {
    if (_qty->text().stripWhiteSpace().length())
    {
      if (_absolute->isChecked())
        _afterQty->setDouble(_qty->toDouble());

      else if (_relative->isChecked())
        _afterQty->setDouble(_cachedQOH + _qty->toDouble());
    }
    else
      _afterQty->clear();
  }
}

void adjustmentTrans::sCostUpdated()
{
  if(_cost->toDouble() == 0.0 || _qty->toDouble() == 0.0)
    _unitCost->setText(tr("N/A"));
  else
    _unitCost->setDouble(_cost->toDouble() / _qty->toDouble());
}

