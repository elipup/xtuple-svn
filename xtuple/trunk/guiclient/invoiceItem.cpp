/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "invoiceItem.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>

#include "priceList.h"
#include "taxDetail.h"

invoiceItem::invoiceItem(QWidget* parent, const char * name, Qt::WindowFlags fl)
    : XDialog(parent, name, fl)
{
  setupUi(this);

  connect(_billed,  SIGNAL(lostFocus()),    this, SLOT(sCalculateExtendedPrice()));
  connect(_item,    SIGNAL(newId(int)),     this, SLOT(sPopulateItemInfo(int)));
  connect(_extended,SIGNAL(valueChanged()), this, SLOT(sLookupTax()));
  connect(_listPrices,   SIGNAL(clicked()), this, SLOT(sListPrices()));
  connect(_price,   SIGNAL(idChanged(int)), this, SLOT(sPriceGroup()));
  connect(_price,   SIGNAL(valueChanged()), this, SLOT(sCalculateExtendedPrice()));
  connect(_save,    SIGNAL(clicked()),      this, SLOT(sSave()));
  connect(_taxLit,  SIGNAL(leftClickedURL(QString)), this, SLOT(sTaxDetail()));
  connect(_taxcode, SIGNAL(newID(int)),     this, SLOT(sLookupTax()));
  connect(_taxtype, SIGNAL(newID(int)),     this, SLOT(sLookupTaxCode()));
  connect(_qtyUOM, SIGNAL(newID(int)), this, SLOT(sQtyUOMChanged()));
  connect(_pricingUOM, SIGNAL(newID(int)), this, SLOT(sPriceUOMChanged()));
  connect(_miscSelected, SIGNAL(toggled(bool)), this, SLOT(sMiscSelected(bool)));

  _item->setType(ItemLineEdit::cSold);

  _ordered->setValidator(omfgThis->qtyVal());
  _billed->setValidator(omfgThis->qtyVal());

  _taxtype->setEnabled(_privileges->check("OverrideTax"));
  _taxcode->setEnabled(_privileges->check("OverrideTax"));

  _mode = cNew;
  _invcheadid	= -1;
  _custid	= -1;
  _invcitemid	= -1;
  _priceRatioCache = 1.0;
  _taxauthid	= -1;
  _cachedPctA	= 0;
  _cachedPctB	= 0;
  _cachedPctC	= 0;
  _cachedRateA	= 0;
  _cachedRateB	= 0;
  _cachedRateC	= 0;
  _qtyinvuomratio = 1.0;
  _priceinvuomratio = 1.0;
  _invuomid = -1;
  
  //If not multi-warehouse hide whs control
  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
  
  adjustSize();
}

invoiceItem::~invoiceItem()
{
    // no need to delete child widgets, Qt does it all for us
}

void invoiceItem::languageChange()
{
    retranslateUi(this);
}

enum SetResponse invoiceItem::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("invchead_id", &valid);
  if (valid)
  {
    _invcheadid = param.toInt();

    q.prepare("SELECT taxauth_id, "
              "       COALESCE(taxauth_curr_id, invchead_curr_id) AS curr_id "
              "FROM invchead, taxauth "
              "WHERE ((invchead_taxauth_id=taxauth_id)"
              "  AND  (invchead_id=:invchead_id));");
    q.bindValue(":invchead_id", _invcheadid);
    q.exec();
    if (q.first())
    {
      _taxauthid = q.value("taxauth_id").toInt();
      _tax->setId(q.value("curr_id").toInt());
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return UndefinedError;
    }
  }

  param = pParams.value("invoiceNumber", &valid);
  if (valid)
    _invoiceNumber->setText(param.toString());

  param = pParams.value("cust_id", &valid);
  if (valid)
    _custid = param.toInt();

  param = pParams.value("cust_curr_id", &valid);
  if (valid)
  {
    _price->setId(param.toInt());
    sPriceGroup();
  }

  param = pParams.value("curr_date", &valid);
  if (valid)
    _price->setEffective(param.toDate());

  param = pParams.value("invcitem_id", &valid);
  if (valid)
  {
    _invcitemid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      q.prepare( "SELECT (COALESCE(MAX(invcitem_linenumber), 0) + 1) AS linenumber "
                 "FROM invcitem "
                 "WHERE (invcitem_invchead_id=:invchead_id);" );
      q.bindValue(":invchead_id", _invcheadid);
      q.exec();
      if (q.first())
        _lineNumber->setText(q.value("linenumber").toString());
      else if (q.lastError().type() != QSqlError::NoError)
      {
	systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
	return UndefinedError;
      }

      connect(_billed, SIGNAL(lostFocus()), this, SLOT(sDeterminePrice()));
      connect(_billed, SIGNAL(lostFocus()), this, SLOT(sCalculateExtendedPrice()));
      connect(_price, SIGNAL(lostFocus()), this, SLOT(sCalculateExtendedPrice()));
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      connect(_billed, SIGNAL(lostFocus()), this, SLOT(sCalculateExtendedPrice()));
      connect(_price, SIGNAL(lostFocus()), this, SLOT(sCalculateExtendedPrice()));

      _save->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _itemTypeGroup->setEnabled(FALSE);
      _custPn->setEnabled(FALSE);
      _ordered->setEnabled(FALSE);
      _billed->setEnabled(FALSE);
      _price->setEnabled(FALSE);
      _notes->setReadOnly(TRUE);
      _taxtype->setEnabled(false);
      _taxcode->setEnabled(false);
      _save->hide();
      _close->setText(tr("&Cancel"));

      _close->setFocus();
    }
  }

  return NoError;
}

void invoiceItem::sSave()
{
  if (_itemSelected->isChecked())
  {
    if (!_item->isValid())
    {
      QMessageBox::critical( this, tr("Cannot Save Invoice Item"),
                             tr("<p>You must select an Item for this Invoice Item before you may save it.") );
      _item->setFocus();
      return;
    }
  }
  else
  {
    if (!_itemNumber->text().length())
    {
      QMessageBox::critical( this, tr("Cannot Save Invoice Item"),
                             tr("<p>You must enter an Item Number for this Miscellaneous Invoice Item before you may save it.") );
      _itemNumber->setFocus();
      return;
    }

    if (!_itemDescrip->toPlainText().length())
    {
      QMessageBox::critical( this, tr("Cannot Save Invoice Item"),
                             tr("<p>You must enter a Item Description for this Miscellaneous Invoice Item before you may save it.") );
      _itemDescrip->setFocus();
      return;
    }

    if (!_salescat->isValid())
    {
      QMessageBox::critical( this, tr("Cannot Save Invoice Item"),
                             tr("<p>You must select a Sales Category for this Miscellaneous Invoice Item before you may save it.") );
      _salescat->setFocus();
      return;
    }

    if (!_ordered->text().length())
    {
      QMessageBox::critical( this, tr("Cannot Save Invoice Item"),
                             tr("<p>You must enter a Quantity Ordered for this Invoice Item before you may save it.") );
      _salescat->setFocus();
      return;
    }

    if (!_billed->text().length())
    {
      QMessageBox::critical( this, tr("Cannot Save Invoice Item"),
                             tr("<p>You must enter a Quantity Billed for this Invoice Item before you may save it.") );
      _salescat->setFocus();
      return;
    }

    if (_price->isZero())
    {
      QMessageBox::critical( this, tr("Cannot Save Invoice Item"),
                             tr("<p>You must enter a Price for this Invoice Item before you may save it.") );
      _salescat->setFocus();
      return;
    }
  }

  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('invcitem_invcitem_id_seq') AS invcitem_id;");
    if (q.first())
      _invcitemid = q.value("invcitem_id").toInt();
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    q.prepare( "INSERT INTO invcitem "
               "( invcitem_id, invcitem_invchead_id, invcitem_linenumber,"
               "  invcitem_item_id, invcitem_warehous_id,"
               "  invcitem_number, invcitem_descrip, invcitem_salescat_id,"
               "  invcitem_custpn,"
               "  invcitem_ordered, invcitem_billed,"
               "  invcitem_qty_uom_id, invcitem_qty_invuomratio,"
               "  invcitem_custprice, invcitem_price,"
               "  invcitem_price_uom_id, invcitem_price_invuomratio,"
               "  invcitem_notes, "
	       "  invcitem_tax_id, invcitem_taxtype_id, "
	       "  invcitem_tax_pcta, invcitem_tax_pctb, invcitem_tax_pctc, "
	       "  invcitem_tax_ratea, invcitem_tax_rateb, invcitem_tax_ratec) "
               "VALUES "
               "( :invcitem_id, :invchead_id, :invcitem_linenumber,"
               "  :item_id, :warehous_id,"
               "  :invcitem_number, :invcitem_descrip, :invcitem_salescat_id,"
               "  :invcitem_custpn,"
               "  :invcitem_ordered, :invcitem_billed,"
               "  :qty_uom_id, :qty_invuomratio,"
               "  :invcitem_custprice, :invcitem_price,"
               "  :price_uom_id, :price_invuomratio,"
               "  :invcitem_notes, "
	       "  :invcitem_tax_id, :invcitem_taxtype_id, "
	       "  :invcitem_tax_pcta, :invcitem_tax_pctb, :invcitem_tax_pctc, "
	       "  :invcitem_tax_ratea, :invcitem_tax_rateb, :invcitem_tax_ratec);");

    q.bindValue(":invchead_id", _invcheadid);
    q.bindValue(":invcitem_linenumber", _lineNumber->text());
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE invcitem "
               "SET invcitem_item_id=:item_id, invcitem_warehous_id=:warehous_id,"
               "    invcitem_number=:invcitem_number, invcitem_descrip=:invcitem_descrip,"
               "    invcitem_salescat_id=:invcitem_salescat_id,"
               "    invcitem_custpn=:invcitem_custpn,"
               "    invcitem_ordered=:invcitem_ordered, invcitem_billed=:invcitem_billed,"
               "    invcitem_qty_uom_id=:qty_uom_id, invcitem_qty_invuomratio=:qty_invuomratio,"
               "    invcitem_custprice=:invcitem_custprice, invcitem_price=:invcitem_price,"
               "    invcitem_price_uom_id=:price_uom_id, invcitem_price_invuomratio=:price_invuomratio,"
               "    invcitem_notes=:invcitem_notes,"
	       "    invcitem_tax_id=:invcitem_tax_id,"
	       "    invcitem_taxtype_id=:invcitem_taxtype_id,"
	       "    invcitem_tax_pcta=:invcitem_tax_pcta,"
	       "    invcitem_tax_pctb=:invcitem_tax_pctb,"
	       "    invcitem_tax_pctc=:invcitem_tax_pctc,"
	       "    invcitem_tax_ratea=:invcitem_tax_ratea,"
	       "    invcitem_tax_rateb=:invcitem_tax_rateb,"
	       "    invcitem_tax_ratec=:invcitem_tax_ratec "
               "WHERE (invcitem_id=:invcitem_id);" );

  if (_itemSelected->isChecked())
  {
    q.bindValue(":item_id", _item->id());
    q.bindValue(":warehous_id", _warehouse->id());
  }
  else
  {
    q.bindValue(":item_id", -1);
    q.bindValue(":warehous_id", -1);
  }

  q.bindValue(":invcitem_id", _invcitemid);
  q.bindValue(":invcitem_number", _itemNumber->text());
  q.bindValue(":invcitem_descrip", _itemDescrip->toPlainText());
  q.bindValue(":invcitem_salescat_id", _salescat->id());
  q.bindValue(":invcitem_custpn", _custPn->text());
  q.bindValue(":invcitem_ordered", _ordered->toDouble());
  q.bindValue(":invcitem_billed", _billed->toDouble());
  if(!_miscSelected->isChecked())
    q.bindValue(":qty_uom_id", _qtyUOM->id());
  q.bindValue(":qty_invuomratio", _qtyinvuomratio);
  q.bindValue(":invcitem_custprice", _custPrice->localValue());
  q.bindValue(":invcitem_price", _price->localValue());
  if(!_miscSelected->isChecked())
    q.bindValue(":price_uom_id", _pricingUOM->id());
  q.bindValue(":price_invuomratio", _priceinvuomratio);
  q.bindValue(":invcitem_notes", _notes->toPlainText());
  if(_taxcode->isValid())
    q.bindValue(":invcitem_tax_id",	_taxcode->id());
  if(_taxtype->isValid())
    q.bindValue(":invcitem_taxtype_id",	_taxtype->id());
  q.bindValue(":invcitem_tax_pcta",	_cachedPctA);
  q.bindValue(":invcitem_tax_pctb",	_cachedPctB);
  q.bindValue(":invcitem_tax_pctc",	_cachedPctC);
  q.bindValue(":invcitem_tax_ratea",	_cachedRateA);
  q.bindValue(":invcitem_tax_rateb",	_cachedRateB);
  q.bindValue(":invcitem_tax_ratec",	_cachedRateC);

  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(_invcitemid);
}

void invoiceItem::populate()
{
  XSqlQuery invcitem;
  invcitem.prepare( "SELECT invcitem.*,"
                    "       CASE WHEN (item_id IS NULL) THEN :na"
                    "            ELSE item_listprice"
                    "       END AS f_listprice,"
                    "       taxauth_id,"
                    "       COALESCE(taxauth_curr_id, invchead_curr_id) AS taxcurrid "
                    "FROM invcitem JOIN "
                    "     invchead ON (invcitem_invchead_id=invchead_id) LEFT OUTER JOIN"
                    "     item ON (invcitem_item_id=item_id) LEFT OUTER JOIN " 
                    "     taxauth ON (invchead_taxauth_id=taxauth_id) "
                    "WHERE (invcitem_id=:invcitem_id);" );
  invcitem.bindValue(":invcitem_id", _invcitemid);
  invcitem.exec();
  if (invcitem.first())
  {
    _invcheadid = invcitem.value("invcitem_invchead_id").toInt();
    _lineNumber->setText(invcitem.value("invcitem_linenumber").toString());

    if (invcitem.value("invcitem_item_id").toInt() != -1)
    {
      _itemSelected->setChecked(TRUE);
      _item->setId(invcitem.value("invcitem_item_id").toInt());
      _warehouse->setId(invcitem.value("invcitem_warehous_id").toInt());
    }
    else
    {
      _miscSelected->setChecked(TRUE);
      _itemNumber->setText(invcitem.value("invcitem_number"));
      _itemDescrip->setText(invcitem.value("invcitem_descrip").toString());
      _salescat->setId(invcitem.value("invcitem_salescat_id").toInt());
    }

    _qtyUOM->setId(invcitem.value("invcitem_qty_uom_id").toInt());
    _qtyinvuomratio = invcitem.value("invcitem_qty_invuomratio").toDouble();
    _pricingUOM->setId(invcitem.value("invcitem_price_uom_id").toInt());
    _priceinvuomratio = invcitem.value("invcitem_price_invuomratio").toDouble();

    // do tax stuff before invcitem_price and _tax_* to avoid signal cascade problems
    if (! invcitem.value("taxauth_id").isNull())
      _taxauthid = invcitem.value("taxauth_id").toInt();
    _tax->setId(invcitem.value("taxcurr_id").toInt());
    _taxcode->setId(invcitem.value("invcitem_tax_id").toInt());
    _taxtype->setId(invcitem.value("invcitem_taxtype_id").toInt());

    _ordered->setDouble(invcitem.value("invcitem_ordered").toDouble());
    _billed->setDouble(invcitem.value("invcitem_billed").toDouble());
    _price->setLocalValue(invcitem.value("invcitem_price").toDouble());
    _custPrice->setLocalValue(invcitem.value("invcitem_custprice").toDouble());
    _listPrice->setBaseValue(invcitem.value("f_listprice").toDouble() * (_priceinvuomratio / _priceRatioCache));

    _custPn->setText(invcitem.value("invcitem_custpn").toString());
    _notes->setText(invcitem.value("invcitem_notes").toString());

    _cachedPctA  = invcitem.value("invcitem_tax_pcta").toDouble();
    _cachedPctB  = invcitem.value("invcitem_tax_pctb").toDouble();
    _cachedPctC  = invcitem.value("invcitem_tax_pctc").toDouble();
    _cachedRateA = invcitem.value("invcitem_tax_ratea").toDouble();
    _cachedRateB = invcitem.value("invcitem_tax_rateb").toDouble();
    _cachedRateC = invcitem.value("invcitem_tax_ratec").toDouble();

    _tax->setLocalValue(_cachedRateA + _cachedRateB + _cachedRateC);
  }
  else if (invcitem.lastError().type() != QSqlError::NoError)
  {
    systemError(this, invcitem.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sCalculateExtendedPrice();
}

void invoiceItem::sCalculateExtendedPrice()
{
  _extended->setLocalValue((_billed->toDouble() * _qtyinvuomratio) * (_price->localValue() / _priceinvuomratio));
}

void invoiceItem::sPopulateItemInfo(int pItemid)
{
  if ( (_itemSelected->isChecked()) && (pItemid != -1) )
  {
    XSqlQuery uom;
    uom.prepare("SELECT uom_id, uom_name"
                "  FROM item"
                "  JOIN uom ON (item_inv_uom_id=uom_id)"
                " WHERE(item_id=:item_id)"
                " UNION "
                "SELECT uom_id, uom_name"
                "  FROM item"
                "  JOIN itemuomconv ON (itemuomconv_item_id=item_id)"
                "  JOIN uom ON (itemuomconv_to_uom_id=uom_id)"
                " WHERE((itemuomconv_from_uom_id=item_inv_uom_id)"
                "   AND (item_id=:item_id))"
                " UNION "
                "SELECT uom_id, uom_name"
                "  FROM item"
                "  JOIN itemuomconv ON (itemuomconv_item_id=item_id)"
                "  JOIN uom ON (itemuomconv_from_uom_id=uom_id)"
                " WHERE((itemuomconv_to_uom_id=item_inv_uom_id)"
                "   AND (item_id=:item_id))"
                " ORDER BY uom_name;");
    uom.bindValue(":item_id", _item->id());
    uom.exec();
    _qtyUOM->populate(uom);
    _pricingUOM->populate(uom);

    q.prepare( "SELECT item_inv_uom_id, item_price_uom_id,"
               "       iteminvpricerat(item_id) AS invpricerat,"
               "       item_listprice, "
               "       stdcost(item_id) AS f_unitcost,"
	       "       getItemTaxType(item_id, :taxauth) AS taxtype_id "
               "  FROM item"
               " WHERE (item_id=:item_id);" );
    q.bindValue(":item_id", pItemid);
    q.bindValue(":taxauth", _taxauthid);
    q.exec();
    if (q.first())
    {
      if (_mode == cNew)
        sDeterminePrice();

      _priceRatioCache = q.value("invpricerat").toDouble();
      _listPrice->setBaseValue(q.value("item_listprice").toDouble());

      _invuomid = q.value("item_inv_uom_id").toInt();
      _qtyUOM->setId(q.value("item_inv_uom_id").toInt());
      _pricingUOM->setId(q.value("item_price_uom_id").toInt());
      _qtyinvuomratio = 1.0;
      _priceinvuomratio = q.value("invpricerat").toDouble();
      _unitCost->setBaseValue(q.value("f_unitcost").toDouble());
      _taxtype->setId(q.value("taxtype_id").toInt());
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else
  {
    _priceRatioCache = 1.0;
    _qtyinvuomratio = 1.0;
    _priceinvuomratio = 1.0;
    _qtyUOM->clear();
    _pricingUOM->clear();
    _listPrice->clear();
    _unitCost->clear();
  }
}

void invoiceItem::sDeterminePrice()
{
  if ( (_itemSelected->isChecked()) && (_item->isValid()) && (_billed->toDouble()) )
  {
    XSqlQuery itemprice;
    itemprice.prepare( "SELECT itemPrice(item_id, :cust_id, -1, "
		       "		 :qty, :qtyUOM, :priceUOM, :curr_id, :effective) AS price "
                       "FROM item "
                       "WHERE (item_id=:item_id);" );
    itemprice.bindValue(":cust_id", _custid);
    itemprice.bindValue(":qty", _billed->toDouble());
    itemprice.bindValue(":qtyUOM", _qtyUOM->id());
    itemprice.bindValue(":priceUOM", _pricingUOM->id());
    itemprice.bindValue(":item_id", _item->id());
    itemprice.bindValue(":curr_id", _price->id());
    itemprice.bindValue(":effective", _price->effective());
    itemprice.exec();
    if (itemprice.first())
    {
      if (itemprice.value("price").toDouble() == -9999.0)
      {
        QMessageBox::critical( this, tr("Customer Cannot Buy at Quantity"),
			       tr("<p>Although the selected Customer may "
				  "purchase the selected Item at some quantity "
				  "levels, the entered Quantity Ordered is too "
				  "low. You may click on the price list button "
				  "(...) next to the Unit Price to determine "
				  "the minimum quantity the selected "
				  "Customer may purchase." ) );
        _custPrice->clear();
        _price->clear();
        _billed->clear();

        _billed->setFocus();
	return;
      }
      double price = itemprice.value("price").toDouble();
      //price = price * (_priceinvuomratio / _priceRatioCache);
      _custPrice->setLocalValue(price);
      _price->setLocalValue(price);
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void invoiceItem::sPriceGroup()
{
  if (! omfgThis->singleCurrency())
    _priceGroup->setTitle(tr("In %1:").arg(_price->currAbbr()));
}

void invoiceItem::sLookupTaxCode()
{
  XSqlQuery taxq;
  taxq.prepare("SELECT tax_ratea, tax_rateb, tax_ratec, tax_id "
	       "FROM tax "
	       "WHERE (tax_id=getTaxSelection(:auth, :type));");
  taxq.bindValue(":auth",    _taxauthid);
  taxq.bindValue(":type",    _taxtype->id());
  taxq.exec();
  if (taxq.first())
  {
    _taxcode->setId(taxq.value("tax_id").toInt());
    _cachedPctA	= taxq.value("tax_ratea").toDouble();
    _cachedPctB	= taxq.value("tax_rateb").toDouble();
    _cachedPctC	= taxq.value("tax_ratec").toDouble();
  }
  else if (taxq.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taxq.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  else
    _taxcode->setId(-1);
}

void invoiceItem::sLookupTax()
{
  XSqlQuery calcq;
  calcq.prepare("SELECT currToCurr(:extcurr, COALESCE(taxauth_curr_id, :extcurr),"
		"         calculateTax(:tax_id, :ext, 0, 'A'), :date) AS valA,"
		"       currToCurr(:extcurr, COALESCE(taxauth_curr_id, :extcurr),"
		"         calculateTax(:tax_id, :ext, 0, 'B'), :date) AS valB,"
		"       currToCurr(:extcurr, COALESCE(taxauth_curr_id, :extcurr),"
		"         calculateTax(:tax_id, :ext, 0, 'C'), :date) AS valC "
		"FROM taxauth "
		"WHERE (taxauth_id=:auth);");

  calcq.bindValue(":extcurr", _extended->id());
  calcq.bindValue(":tax_id",  _taxcode->id());
  calcq.bindValue(":ext",     _extended->localValue());
  calcq.bindValue(":date",    _extended->effective());
  calcq.bindValue(":auth",    _taxauthid);
  calcq.exec();
  if (calcq.first())
  {
    _cachedRateA= calcq.value("valA").toDouble();
    _cachedRateB= calcq.value("valB").toDouble();
    _cachedRateC= calcq.value("valC").toDouble();
    _tax->setLocalValue(_cachedRateA + _cachedRateB + _cachedRateC);
  }
  else if (calcq.lastError().type() != QSqlError::NoError)
  {
    systemError(this, calcq.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void invoiceItem::sTaxDetail()
{
  taxDetail newdlg(this, "", true);
  ParameterList params;
  params.append("tax_id",   _taxcode->id());
  params.append("curr_id",  _tax->id());
  params.append("valueA",   _cachedRateA);
  params.append("valueB",   _cachedRateB);
  params.append("valueC",   _cachedRateC);
  params.append("pctA",	    _cachedPctA);
  params.append("pctB",	    _cachedPctB);
  params.append("pctC",	    _cachedPctC);
  params.append("subtotal", CurrDisplay::convert(_extended->id(), _tax->id(),
						 _extended->localValue(),
						 _extended->effective()));

  if(cView == _mode)
    params.append("readOnly");

  if (newdlg.set(params) == NoError && newdlg.exec())
  {
    _cachedRateA = newdlg.amountA();
    _cachedRateB = newdlg.amountB();
    _cachedRateC = newdlg.amountC();
    _cachedPctA	 = newdlg.pctA();
    _cachedPctB	 = newdlg.pctB();
    _cachedPctC	 = newdlg.pctC();

    if (_taxcode->id() != newdlg.tax())
      _taxcode->setId(newdlg.tax());

    _tax->setLocalValue(_cachedRateA + _cachedRateB + _cachedRateC);
  }
}

void invoiceItem::sQtyUOMChanged()
{
  if(_qtyUOM->id() == _invuomid)
    _qtyinvuomratio = 1.0;
  else
  {
    XSqlQuery invuom;
    invuom.prepare("SELECT itemuomtouomratio(item_id, :uom_id, item_inv_uom_id) AS ratio"
                   "  FROM item"
                   " WHERE(item_id=:item_id);");
    invuom.bindValue(":item_id", _item->id());
    invuom.bindValue(":uom_id", _qtyUOM->id());
    invuom.exec();
    if(invuom.first())
      _qtyinvuomratio = invuom.value("ratio").toDouble();
    else
      systemError(this, invuom.lastError().databaseText(), __FILE__, __LINE__);
  }

  if(_qtyUOM->id() != _invuomid)
  {
    _pricingUOM->setId(_qtyUOM->id());
    _pricingUOM->setEnabled(false);
  }
  else
    _pricingUOM->setEnabled(true);
  sDeterminePrice();
  sCalculateExtendedPrice();
}

void invoiceItem::sPriceUOMChanged()
{
  if(_pricingUOM->id() == -1 || _qtyUOM->id() == -1)
    return;

  if(_pricingUOM->id() == _invuomid)
    _priceinvuomratio = 1.0;
  else
  {
    XSqlQuery invuom;
    invuom.prepare("SELECT itemuomtouomratio(item_id, :uom_id, item_inv_uom_id) AS ratio"
                   "  FROM item"
                   " WHERE(item_id=:item_id);");
    invuom.bindValue(":item_id", _item->id());
    invuom.bindValue(":uom_id", _pricingUOM->id());
    invuom.exec();
    if(invuom.first())
      _priceinvuomratio = invuom.value("ratio").toDouble();
    else
      systemError(this, invuom.lastError().databaseText(), __FILE__, __LINE__);
  }

  XSqlQuery item;
  item.prepare("SELECT item_listprice"
               "  FROM item"
               " WHERE(item_id=:item_id);");
  item.bindValue(":item_id", _item->id());
  item.exec();
  item.first();
  _listPrice->setBaseValue(item.value("item_listprice").toDouble() * (_priceinvuomratio / _priceRatioCache));
  sDeterminePrice();
  sCalculateExtendedPrice();
}

void invoiceItem::sMiscSelected(bool isMisc)
{
  if(isMisc)
    _item->setId(-1);
}

void invoiceItem::sListPrices()
{
  ParameterList params;
  params.append("cust_id",   _custid);
  //params.append("shipto_id", _shiptoid);
  params.append("item_id",   _item->id());
  params.append("qty",       _billed->toDouble() * _qtyinvuomratio);
  params.append("curr_id",   _price->id());
  params.append("effective", _price->effective());

  priceList newdlg(this);
  newdlg.set(params);
  if ( (newdlg.exec() == XDialog::Accepted) &&
       (_privileges->check("OverridePrice")) &&
       (!_metrics->boolean("DisableSalesOrderPriceOverride")) )
  {
    _price->setLocalValue(newdlg._selectedPrice * (_priceinvuomratio / _priceRatioCache));
  }
}
