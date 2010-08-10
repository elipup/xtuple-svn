/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspSummarizedSalesByCustomerType.h"

#include <QVariant>
#include <QMessageBox>

#include "xtreewidget.h"

dspSummarizedSalesByCustomerType::dspSummarizedSalesByCustomerType(QWidget* parent, const char*, Qt::WFlags fl)
  : display(parent, "dspSummarizedSalesByCustomerType", fl)
{
  setupUi(optionsWidget());
  setWindowTitle(tr("Summarized Sales by Customer Type"));
  setListLabel(tr("Sales History"));
  setReportName("SummarizedSalesByCustomerType");
  setMetaSQLOptions("summarizedSalesHistory", "detail");

  _customerType->setType(ParameterGroup::CustomerType);

  list()->addColumn(tr("Customer Type"),    -1,              Qt::AlignLeft,   true,  "custtype_code"   );
  list()->addColumn(tr("Site"),             _whsColumn,      Qt::AlignCenter, true,  "warehous_code" );
  list()->addColumn(tr("Min. Price"),       _priceColumn,    Qt::AlignRight,  true,  "minprice"  );
  list()->addColumn(tr("Max. Price"),       _priceColumn,    Qt::AlignRight,  true,  "maxprice"  );
  list()->addColumn(tr("Avg. Price"),       _priceColumn,    Qt::AlignRight,  true,  "avgprice"  );
  list()->addColumn(tr("Wt. Avg. Price"),   _priceColumn,    Qt::AlignRight,  true,  "wtavgprice"  );
  list()->addColumn(tr("Total Units"),      _qtyColumn,      Qt::AlignRight,  true,  "totalunits"  );
  list()->addColumn(tr("Total Sales"),      _bigMoneyColumn, Qt::AlignRight,  true,  "totalsales"  );
  list()->setDragString("custtypeid=");
}

void dspSummarizedSalesByCustomerType::languageChange()
{
  display::languageChange();
  retranslateUi(this);
}

bool dspSummarizedSalesByCustomerType::setParams(ParameterList & params)
{
  if (!_dates->startDate().isValid())
  {
    if(isVisible()) {
      QMessageBox::warning( this, tr("Enter Start Date"),
                            tr("Please enter a valid Start Date.") );
      _dates->setFocus();
    }
    return FALSE;
  }

  if (!_dates->endDate().isValid())
  {
    if(isVisible()) {
      QMessageBox::warning( this, tr("Enter End Date"),
                            tr("Please enter a valid End Date.") );
      _dates->setFocus();
    }
    return FALSE;
  }

  if (_customerType->isPattern())
  {
    QString pattern = _customerType->pattern();
    if (pattern.length() == 0)
      return FALSE;
  }

  _dates->appendValue(params);
  _warehouse->appendValue(params);
  _customerType->appendValue(params);

  params.append("byCustomerType"); // metasql only?
  params.append("commonParamsSet1"); // metasql only?

  return true;
}
