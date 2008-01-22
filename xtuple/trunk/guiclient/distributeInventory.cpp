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

#include "distributeInventory.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>

#include "assignLotSerial.h"
#include "distributeToLocation.h"
#include "inputManager.h"

#define cIncludeLotSerial   0x01
#define cNoIncludeLotSerial 0x02

distributeInventory::distributeInventory(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_bcDistribute,    SIGNAL(clicked()), this, SLOT(sBcDistribute()));
  connect(_default,         SIGNAL(clicked()), this, SLOT(sDefault()));
  connect(_defaultAndPost,  SIGNAL(clicked()), this, SLOT(sDefaultAndPost()));
  connect(_distribute,      SIGNAL(clicked()), this, SLOT(sSelectLocation()));
  connect(_itemloc, SIGNAL(itemSelected(int)), this, SLOT(sSelectLocation()));
  connect(_post,            SIGNAL(clicked()), this, SLOT(sPost()));
  connect(_taggedOnly,  SIGNAL(toggled(bool)), this, SLOT(sFillList()));
  connect(_bc,   SIGNAL(textChanged(QString)), this, SLOT(sBcChanged(QString)));
  connect(_cancel,          SIGNAL(clicked()), this, SLOT(sCancel()));

  omfgThis->inputManager()->notify(cBCLotSerialNumber, this, this, SLOT(sCatchLotSerialNumber(QString)));

  _trapClose = TRUE;

  _item->setReadOnly(TRUE);
  
  _itemloc->addColumn(tr("Location"),     _itemColumn, Qt::AlignLeft   );
  _itemloc->addColumn(tr("Default"),      _ynColumn,   Qt::AlignLeft   );
  _itemloc->addColumn(tr("Netable"),      _ynColumn,   Qt::AlignCenter );
  _itemloc->addColumn(tr("Lot/Serial #"), -1,          Qt::AlignLeft   );
  _itemloc->addColumn(tr("Expiration"),   _dateColumn, Qt::AlignCenter );
  _itemloc->addColumn(tr("Qty. Before"),  _qtyColumn,  Qt::AlignRight  );
  _itemloc->addColumn(tr("Tagged Qty."),  _qtyColumn,  Qt::AlignRight  );
  _itemloc->addColumn(tr("Qty. After"),   _qtyColumn,  Qt::AlignRight  );
  
  //If not multi-warehouse hide whs control
  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
  
  //If not lot serial control, hide info
  if (!_metrics->boolean("LotSerialControl"))
  {
    _lotSerial->hide();
    _lotSerialLit->hide();
    _bcLit->hide();
    _bc->hide();
    _bcQtyLit->hide();
    _bcQty->hide();
    _bcDistribute->hide();
  }

  _itemlocdistid = -1;
}

distributeInventory::~distributeInventory()
{
  // no need to delete child widgets, Qt does it all for us
}

void distributeInventory::languageChange()
{
  retranslateUi(this);
}

QList<QVariant> distributeInventory::SeriesAdjust(int pItemsiteid, double pQty, QWidget *pParent, const QString & pPresetLotnum, const QDate & pPresetLotexp)
{
  QList<QVariant> itemlocSeriesList;
  int c = 0;

  q.exec("SELECT NEXTVAL('itemloc_series_seq') AS itemlocseries;");
  q.first();
  int itemlocSeries = q.value("itemlocseries").toInt();

  q.prepare("INSERT INTO itemlocdist "	
            "( itemlocdist_itemsite_id, itemlocdist_source_type, "
            "  itemlocdist_reqlotserial, "
            "  itemlocdist_distlotserial, "
            "  itemlocdist_lotserial, itemlocdist_expiration, "
            "  itemlocdist_qty, "
            "  itemlocdist_series, itemlocdist_invhist_id ) "
			" SELECT itemsite_id, 'O', "
			" ((:qty > 0)  AND itemsite_controlmethod IN ('L','S')), "
			" (:qty < 0), "
            " '', endOfTime(), "
			" :qty, "
			" :itemlocseries, -1 "
			" FROM itemsite "
			" WHERE (itemsite_id=:itemsiteid);");
  q.bindValue(":itemlocseries", itemlocSeries);
  q.bindValue(":itemsiteid", pItemsiteid);
  q.bindValue(":qty", pQty);
  q.exec();
  if (q.lastError().type() == QSqlError::None)
  {
    XSqlQuery itemloc;
    itemloc.prepare( "SELECT itemlocdist_id, itemlocdist_reqlotserial," 
                     "       itemlocdist_distlotserial, itemlocdist_qty,"
                     "       itemsite_loccntrl, itemsite_controlmethod,"
                     "       itemsite_perishable "
                     "FROM itemlocdist, itemsite "
                     "WHERE ( (itemlocdist_itemsite_id=itemsite_id)"
                     " AND (itemlocdist_series=:itemlocdist_series) ) "
                     "ORDER BY itemlocdist_id;" );
    itemloc.bindValue(":itemlocdist_series", itemlocSeries);
    itemloc.exec();
    while (itemloc.next())
    {
      if (itemloc.value("itemlocdist_reqlotserial").toBool())
      {
        itemlocSeries = -1;
        XSqlQuery query;
        // Check to see if this is a lot controlled item and if we have
        // a predefined lot#/expdate to use. If so assign that information
        // with itemlocdist_qty and move on. otherwise do the normal dialog
        // to ask the user for that information.
        if(itemloc.value("itemsite_controlmethod").toString() == "L" && !pPresetLotnum.isEmpty())
        {
          query.exec("SELECT nextval('itemloc_series_seq') AS _itemloc_series;");
          if(query.first())
          {
            itemlocSeries = query.value("_itemloc_series").toInt();
            query.exec("SELECT NEXTVAL('itemlocdist_itemlocdist_id_seq') AS itemlocdist_id;");
            query.first();
            int itemlocdistid = query.value("itemlocdist_id").toInt();
            query.prepare( "INSERT INTO itemlocdist "
                           "( itemlocdist_id, itemlocdist_source_type, itemlocdist_source_id,"
                           "  itemlocdist_itemsite_id, itemlocdist_lotserial, itemlocdist_expiration,"
                           "  itemlocdist_qty, itemlocdist_series, itemlocdist_invhist_id ) "
                           "SELECT :newItemlocdist_id, 'D', itemlocdist_id,"
                           "       itemlocdist_itemsite_id, :lotSerialNumber, :itemsite_expiration,"
                           "       :qtyToAssign, :itemlocdist_series, itemlocdist_invhist_id "
                           "FROM itemlocdist "
                           "WHERE (itemlocdist_id=:itemlocdist_id);" );
            if(itemloc.value("itemsite_perishable").toBool())
              query.bindValue(":itemsite_expiration", pPresetLotexp);
            else
              query.bindValue(":itemsite_expiration", omfgThis->startOfTime());

            query.bindValue(":newItemlocdist_id", itemlocdistid);
            query.bindValue(":lotSerialNumber", pPresetLotnum);
            query.bindValue(":qtyToAssign", itemloc.value("itemlocdist_qty"));
            query.bindValue(":itemlocdist_series", itemlocSeries);
            query.bindValue(":itemlocdist_id", itemloc.value("itemlocdist_id"));
            query.exec();

            query.prepare( "INSERT INTO lsdetail "
                           "( lsdetail_itemsite_id, lsdetail_lotserial, lsdetail_created,"
                           "  lsdetail_source_type, lsdetail_source_id, lsdetail_source_number ) "
                           "SELECT itemlocdist_itemsite_id, :lotSerialNumber, CURRENT_TIMESTAMP,"
                           "       'I', itemlocdist_id, '' "
                           "FROM itemlocdist "
                           "WHERE (itemlocdist_id=:itemlocdist_id);" );
            query.bindValue(":lotSerialNumber", pPresetLotnum);
            query.bindValue(":itemlocdist_id", itemlocdistid);
            query.exec();

            query.prepare( "UPDATE itemlocdist "
                           "SET itemlocdist_source_type='O' "
                           "WHERE (itemlocdist_series=:itemlocdist_series);"
              
                           "DELETE FROM itemlocdist "
                           "WHERE (itemlocdist_id=:itemlocdist_id);" );
            query.bindValue(":itemlocdist_series", itemlocSeries);
            query.bindValue(":itemlocdist_id", itemloc.value("itemlocdist_id"));
            query.exec();
          }
        }

        if(itemlocSeries == -1)
        {
          ParameterList params;
          params.append("itemlocdist_id", itemloc.value("itemlocdist_id").toInt());
		  params.append("cancelVisible");

          assignLotSerial newdlg(pParent, "", TRUE);
          newdlg.set(params);
		  itemlocSeries=newdlg.exec();
          if (itemlocSeries == -1)	//Transaction was canceled
		  {
			itemlocSeriesList.prepend(-1);
            return itemlocSeriesList;
		  }
        }

        if (itemloc.value("itemsite_loccntrl").toBool())
        {
          query.prepare( "SELECT itemlocdist_id " 
                         "FROM itemlocdist "
                         "WHERE (itemlocdist_series=:itemlocdist_series) "
                         "ORDER BY itemlocdist_id;" );
          query.bindValue(":itemlocdist_series", itemlocSeries);
          query.exec();
          while (query.next())
          {
            ParameterList params;
            params.append("itemlocdist_id", query.value("itemlocdist_id").toInt());
		    params.append("cancelVisible");

            distributeInventory newdlg(pParent, "", TRUE);
            newdlg.set(params);
		    itemlocSeriesList.prepend(newdlg.exec());
            if (itemlocSeriesList.at(0) == -1) //Transaction was canceled
			{
			  for(c = 1; c < itemlocSeriesList.count(); c++)
			  {
				q.prepare( "DELETE FROM itemlocdist "
						   "WHERE (itemlocdist_id=:itemlocdistid); "
						   "DELETE FROM itemlocdist "
						   "WHERE (itemlocdist_itemlocdist_id=:itemlocdistid); ");
				q.bindValue(":itemlocdistid", itemlocSeriesList.at(c));
				q.exec();
			  }
			  return itemlocSeriesList;
			}
		  }
        }
        else
		{
          query.prepare( "UPDATE itemlocdist "
                         "SET itemlocdist_source_type='L', itemlocdist_source_id=-1 "
                         "WHERE (itemlocdist_series=:itemlocdist_series); ");
          query.bindValue(":itemlocdist_series", itemlocSeries);
          query.exec();

		  itemlocSeriesList.prepend(itemlocSeries);
        }
      }
      else
      {
        ParameterList params;
        params.append("itemlocdist_id", itemloc.value("itemlocdist_id").toInt());

        if (itemloc.value("itemlocdist_distlotserial").toBool())
          params.append("includeLotSerialDetail");
		  params.append("cancelVisible");

        distributeInventory newdlg(pParent, "", TRUE);
        newdlg.set(params);
		itemlocSeriesList.prepend(newdlg.exec());
        if (itemlocSeriesList.at(0) == -1)	//Transaction canceled
		{
          for(c = 1; c < itemlocSeriesList.count(); c++)
		  {
            q.prepare( "DELETE FROM itemlocdist "
	                   "WHERE (itemlocdist_id=:itemlocdistid); "
	                   "DELETE FROM itemlocdist "
	                   "WHERE (itemlocdist_itemlocdist_id=:itemlocdistid); ");
            q.bindValue(":itemlocdistid", itemlocSeriesList.at(c));
            q.exec();
		  }
          return itemlocSeriesList;
		}
      }
    }
  }
  return itemlocSeriesList;
}

int distributeInventory::SeriesAdjust(int pItemlocSeries, QWidget *pParent, const QString & pPresetLotnum, const QDate & pPresetLotexp)
{
  //This can be removed when cancel logic has been included in all transactions and db functions
  if (pItemlocSeries != 0)
  {
    XSqlQuery itemloc;
    itemloc.prepare( "SELECT itemlocdist_id, itemlocdist_reqlotserial," 
                     "       itemlocdist_distlotserial, itemlocdist_qty,"
                     "       itemsite_loccntrl, itemsite_controlmethod,"
                     "       itemsite_perishable "
                     "FROM itemlocdist, itemsite "
                     "WHERE ( (itemlocdist_itemsite_id=itemsite_id)"
                     " AND (itemlocdist_series=:itemlocdist_series) ) "
                     "ORDER BY itemlocdist_id;" );
    itemloc.bindValue(":itemlocdist_series", pItemlocSeries);
    itemloc.exec();
    while (itemloc.next())
    {
      if (itemloc.value("itemlocdist_reqlotserial").toBool())
      {
        int itemlocSeries = -1;
        XSqlQuery query;
        // Check to see if this is a lot controlled item and if we have
        // a predefined lot#/expdate to use. If so assign that information
        // with itemlocdist_qty and move on. otherwise do the normal dialog
        // to ask the user for that information.
        if(itemloc.value("itemsite_controlmethod").toString() == "L" && !pPresetLotnum.isEmpty())
        {
          query.exec("SELECT nextval('itemloc_series_seq') AS _itemloc_series;");
          if(query.first())
          {
            itemlocSeries = query.value("_itemloc_series").toInt();
            query.exec("SELECT NEXTVAL('itemlocdist_itemlocdist_id_seq') AS itemlocdist_id;");
            query.first();
            int itemlocdistid = query.value("itemlocdist_id").toInt();
            query.prepare( "INSERT INTO itemlocdist "
                           "( itemlocdist_id, itemlocdist_source_type, itemlocdist_source_id,"
                           "  itemlocdist_itemsite_id, itemlocdist_lotserial, itemlocdist_expiration,"
                           "  itemlocdist_qty, itemlocdist_series, itemlocdist_invhist_id ) "
                           "SELECT :newItemlocdist_id, 'D', itemlocdist_id,"
                           "       itemlocdist_itemsite_id, :lotSerialNumber, :itemsite_expiration,"
                           "       :qtyToAssign, :itemlocdist_series, itemlocdist_invhist_id "
                           "FROM itemlocdist "
                           "WHERE (itemlocdist_id=:itemlocdist_id);" );
            if(itemloc.value("itemsite_perishable").toBool())
              query.bindValue(":itemsite_expiration", pPresetLotexp);
            else
              query.bindValue(":itemsite_expiration", omfgThis->startOfTime());

            query.bindValue(":newItemlocdist_id", itemlocdistid);
            query.bindValue(":lotSerialNumber", pPresetLotnum);
            query.bindValue(":qtyToAssign", itemloc.value("itemlocdist_qty"));
            query.bindValue(":itemlocdist_series", itemlocSeries);
            query.bindValue(":itemlocdist_id", itemloc.value("itemlocdist_id"));
            query.exec();

            query.prepare( "INSERT INTO lsdetail "
                           "( lsdetail_itemsite_id, lsdetail_lotserial, lsdetail_created,"
                           "  lsdetail_source_type, lsdetail_source_id, lsdetail_source_number ) "
                           "SELECT itemlocdist_itemsite_id, :lotSerialNumber, CURRENT_TIMESTAMP,"
                           "       'I', itemlocdist_id, '' "
                           "FROM itemlocdist "
                           "WHERE (itemlocdist_id=:itemlocdist_id);" );
            query.bindValue(":lotSerialNumber", pPresetLotnum);
            query.bindValue(":itemlocdist_id", itemlocdistid);
            query.exec();

            query.prepare( "UPDATE itemlocdist "
                           "SET itemlocdist_source_type='O' "
                           "WHERE (itemlocdist_series=:itemlocdist_series);"
              
                           "DELETE FROM itemlocdist "
                           "WHERE (itemlocdist_id=:itemlocdist_id);" );
            query.bindValue(":itemlocdist_series", itemlocSeries);
            query.bindValue(":itemlocdist_id", itemloc.value("itemlocdist_id"));
            query.exec();
          }
        }

        if(itemlocSeries == -1)
        {
          ParameterList params;
          params.append("itemlocdist_id", itemloc.value("itemlocdist_id").toInt());

          assignLotSerial newdlg(pParent, "", TRUE);
          newdlg.set(params);
          itemlocSeries = newdlg.exec();
        }

        if (itemloc.value("itemsite_loccntrl").toBool())
        {
          query.prepare( "SELECT itemlocdist_id " 
                         "FROM itemlocdist "
                         "WHERE (itemlocdist_series=:itemlocdist_series) "
                         "ORDER BY itemlocdist_id;" );
          query.bindValue(":itemlocdist_series", itemlocSeries);
          query.exec();
          while (query.next())
          {
            ParameterList params;
            params.append("itemlocdist_id", query.value("itemlocdist_id").toInt());

            distributeInventory newdlg(pParent, "", TRUE);
            newdlg.set(params);
            newdlg.exec();
          }
        }
        else
        {
          query.prepare( "UPDATE itemlocdist "
                         "SET itemlocdist_source_type='L', itemlocdist_source_id=-1 "
                         "WHERE (itemlocdist_series=:itemlocdist_series); ");
          query.bindValue(":itemlocdist_series", itemlocSeries);
          query.exec();

          query.prepare( "SELECT distributeItemlocSeries(:itemlocdist_series) AS result;");
          query.bindValue(":itemlocdist_series", itemlocSeries);
          query.exec();
        }
      }
      else
      {
        ParameterList params;
        params.append("itemlocdist_id", itemloc.value("itemlocdist_id").toInt());

        if (itemloc.value("itemlocdist_distlotserial").toBool())
          params.append("includeLotSerialDetail");

        distributeInventory newdlg(pParent, "", TRUE);
        newdlg.set(params);
        newdlg.exec();
      }
    }
  }

  return QDialog::Accepted;
}

enum SetResponse distributeInventory::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("includeLotSerialDetail", &valid);
  if (valid)
  {
    _mode = cIncludeLotSerial;
  }
  else
    _mode = cNoIncludeLotSerial;

  param = pParams.value("itemlocdist_id", &valid);
  if (valid)
  {
    _itemlocdistid = param.toInt();
    populate();
  }

  param = pParams.value("cancelVisible", &valid);
  _cancel->setVisible(valid);

  return NoError;
}

void distributeInventory::closeEvent(QCloseEvent *pEvent)
{
  if (_trapClose)
  {
    QMessageBox::critical( this, tr("Cannot Cancel Distribution"),
                           tr( "<p>You must select locations to distribute to/from "
			       "and select the 'Post' button. You may not cancel "
			       "this action." ) );
    pEvent->ignore();
  }
  else
    pEvent->accept();
}

void distributeInventory::populate()
{
  q.prepare("SELECT itemsite_controlmethod "
	    "FROM itemsite, itemlocdist "
	    "WHERE ((itemlocdist_itemsite_id=itemsite_id)"
	    "  AND  (itemlocdist_id=:itemlocdist_id));");

  q.bindValue(":itemlocdist_id", _itemlocdistid);
  q.exec();
  if (q.first())
  {
    _controlMethod = q.value("itemsite_controlmethod").toString();
    _bc->setEnabled(_controlMethod == "L" || _controlMethod == "S");
    _bcQty->setEnabled(_controlMethod == "L");
    _bcDistribute->setEnabled(_controlMethod == "L" || _controlMethod == "S");
    if (_controlMethod == "S")
      _bcQty->setText("1");
    else
      _bcQty->clear();
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void distributeInventory::sSelectLocation()
{
  ParameterList params;
  params.append("source_itemlocdist_id", _itemlocdistid);

  if (_itemloc->altId() == cLocation)
    params.append("location_id", _itemloc->id());
  else if (_itemloc->altId() == cItemloc)
    params.append("itemlocdist_id", _itemloc->id());

  distributeToLocation newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() == QDialog::Accepted)
    sFillList();
}


void distributeInventory::sPost()
{
  if (_qtyRemaining->text().toDouble() != 0.0)
  {
    QMessageBox::critical( this, tr("Cannot Perform Partial Distribution"),
                           tr( "<p>You must completely distribute the quantity "
			       "before selecting the 'Post' button." ) );
    return;
  }

  if (!_cancel->isVisible())
  {
    q.prepare("SELECT distributeToLocations(:itemlocdist_id) AS result;");
    q.bindValue(":itemlocdist_id", _itemlocdistid);
    q.exec();

    _trapClose = FALSE;
    accept();
  }
  else
    done(_itemlocdistid);
}

void distributeInventory::sDefault()
{
  if(_mode == cIncludeLotSerial)
    q.prepare("SELECT distributeToDefaultItemLoc(:itemlocdist_id) AS result;");
  else
    q.prepare("SELECT distributeToDefault(:itemlocdist_id) AS result;");
  q.bindValue(":itemlocdist_id", _itemlocdistid);
  q.exec();
  sFillList();
}

void distributeInventory::sDefaultAndPost()
{
  sDefault();
  sPost();
}

void distributeInventory::sFillList()
{
  q.prepare( "SELECT itemsite_id, "
	     "       COALESCE(itemsite_location_id,-1) AS itemsite_location_id,"
	     "       itemlocdist_lotserial,"
             "       (itemsite_controlmethod IN ('L', 'S')) AS lscontrol,"
             "       parent.itemlocdist_qty AS qtytodistribute,"
             "       ( ( SELECT COALESCE(SUM(child.itemlocdist_qty), 0)"
             "             FROM itemlocdist AS child"
             "            WHERE (child.itemlocdist_itemlocdist_id=parent.itemlocdist_id) ) ) AS qtytagged,"
             "       (parent.itemlocdist_qty - ( SELECT COALESCE(SUM(child.itemlocdist_qty), 0)"
             "                                     FROM itemlocdist AS child"
             "                                    WHERE (child.itemlocdist_itemlocdist_id=parent.itemlocdist_id) ) ) AS qtybalance "
             "FROM itemsite, itemlocdist AS parent "
             "WHERE ( (itemlocdist_itemsite_id=itemsite_id)"
             " AND (itemlocdist_id=:itemlocdist_id) );" );
  q.bindValue(":itemlocdist_id", _itemlocdistid);
  q.exec();
  if (q.first())
  {
    _item->setItemsiteid(q.value("itemsite_id").toInt());
    _lotSerial->setText(q.value("itemlocdist_lotserial").toString());
    _qtyToDistribute->setText(formatNumber(q.value("qtytodistribute").toDouble(),6));
    _qtyTagged->setText(formatNumber(q.value("qtytagged").toDouble(),6));
    _qtyRemaining->setText(formatNumber(q.value("qtybalance").toDouble(),6));

    if ( (q.value("itemsite_location_id").toInt() != -1) &&
         ( (_mode == cNoIncludeLotSerial) || ( (_mode == cIncludeLotSerial) && (!q.value("lscontrol").toBool()) ) ) )
    {
      _default->setEnabled(TRUE);
      _defaultAndPost->setEnabled(TRUE);
    }
    else
    {
      _default->setEnabled(FALSE);
      _defaultAndPost->setEnabled(FALSE);
    }

    QString sql( "SELECT id, type,"
                 "       locationname,"
		 "       CASE WHEN defaultlocation THEN <? value(\"yes\") ?>"
		 "            ELSE <? value(\"no\") ?>"
		 "       END AS defaultlocation,"
		 "       CASE WHEN (location_netable) THEN <? value(\"yes\") ?>"
		 "            ELSE <? value(\"no\") ?>"
		 "       END AS netable,"
		 "       lotserial, f_expiration, expired,"
                 "       qty,"
                 "       qtytagged,"
                 "       (qty + qtytagged) AS balance "
                 "FROM (" 
		 "<? if exists(\"cNoIncludeLotSerial\") ?>"
		 "SELECT location_id AS id, <? value(\"locationType\") ?> AS type,"
		 "       formatLocationName(location_id) AS locationname,"
		 "       (location_id=itemsite_location_id) AS defaultlocation,"
		 "       location_netable,"
		 "       TEXT('') AS lotserial,"
		 "       TEXT(<? value(\"na\") ?>) AS f_expiration, FALSE AS expired,"
		 "       ( SELECT COALESCE(SUM(itemloc_qty), 0)"
		 "         FROM itemloc "
		 "         WHERE ( (itemloc_location_id=location_id)"
		 "          AND (itemloc_itemsite_id=itemsite_id) ) ) AS qty,"
		 "       itemlocdistQty(location_id, itemlocdist_id) AS qtytagged "
		 "FROM itemlocdist, location, itemsite "
		 "WHERE ( (itemlocdist_itemsite_id=itemsite_id)"
		 " AND (itemsite_loccntrl)"
		 " AND (itemsite_warehous_id=location_warehous_id)"
		 " AND (validLocation(location_id, itemsite_id))"
		 " AND (itemlocdist_id=<? value(\"itemlocdist_id\") ?>) ) "
		 "<? elseif exists(\"cIncludeLotSerial\") ?>"
		 "SELECT itemloc_id AS id, <? value(\"itemlocType\") ?> AS type,"
		 "       COALESCE(formatLocationName(location_id),"
		 "                <? value(\"undefined\") ?>) AS locationname,"
		 "       (location_id IS NOT NULL"
		 "        AND location_id=itemsite_location_id) AS defaultlocation,"
		 "       COALESCE(location_netable, false) AS location_netable,"
		 "       itemloc_lotserial AS lotserial,"
		 "       CASE WHEN (itemsite_perishable) THEN formatDate(itemloc_expiration)"
		 "            ELSE <? value(\"na\") ?>"
		 "       END AS f_expiration,"
		 "       CASE WHEN (itemsite_perishable) THEN (itemloc_expiration < CURRENT_DATE)"
		 "            ELSE FALSE" 
		 "       END AS expired,"
		 "       itemloc_qty AS qty,"
		 "       ( SELECT COALESCE(SUM(target.itemlocdist_qty), 0)"
		 "         FROM itemlocdist AS target"
		 "         WHERE ( (target.itemlocdist_source_type='I')"
		 "          AND (target.itemlocdist_source_id=itemloc_id)"
		 "          AND (target.itemlocdist_itemlocdist_id=source.itemlocdist_id)) ) AS qtytagged "
		 "FROM itemlocdist AS source, itemsite, itemloc LEFT OUTER JOIN location ON (itemloc_location_id=location_id) "
		 "WHERE ( (source.itemlocdist_itemsite_id=itemsite_id)"
		 " AND (itemloc_itemsite_id=itemsite_id)"
		 " AND (source.itemlocdist_id=<? value(\"itemlocdist_id\") ?>) ) "
		 " UNION "
		 "SELECT location_id AS id, <? value(\"locationType\") ?> AS type,"
		 "       formatLocationName(location_id) AS locationname,"
		 "       (location_id=itemsite_location_id) AS defaultlocation,"
		 "       location_netable,"
		 "       TEXT('') AS lotserial,"
		 "       TEXT(<? value(\"na\") ?>) AS f_expiration, FALSE AS expired,"
		 "       ( SELECT COALESCE(SUM(itemloc_qty), 0)"
		 "         FROM itemloc "
		 "         WHERE ( (itemloc_location_id=location_id)"
		 "          AND (itemloc_itemsite_id=itemsite_id) ) ) AS qty,"
		 "       itemlocdistQty(location_id, itemlocdist_id) AS qtytagged "
		 "FROM itemlocdist, location, itemsite "
		 "WHERE ( (itemlocdist_itemsite_id=itemsite_id)"
		 " AND (itemsite_loccntrl)"
		 " AND (itemsite_warehous_id=location_warehous_id)"
		 " AND (validLocation(location_id, itemsite_id))"
		 " AND (location_id NOT IN (SELECT DISTINCT itemloc_location_id FROM itemloc WHERE (itemloc_itemsite_id=itemsite_id)))"
		 " AND (itemlocdist_id=<? value(\"itemlocdist_id\") ?>) ) "
		 "<? endif ?>"
		 ") AS data "
		 "<? if exists(\"showOnlyTagged\") ?>"
		 "WHERE (qtytagged != 0) "
		 "<? endif ?>"
		 "ORDER BY locationname;");

    ParameterList params;

    if (_mode == cNoIncludeLotSerial)
      params.append("cNoIncludeLotSerial");
    else if (_mode == cIncludeLotSerial)
      params.append("cIncludeLotSerial");

    if (_taggedOnly->isChecked())
      params.append("showOnlyTagged");

    params.append("locationType",   cLocation);
    params.append("itemlocType",    cItemloc);
    params.append("yes",            tr("Yes"));
    params.append("no",             tr("No"));
    params.append("na",             tr("N/A"));
    params.append("undefined",      tr("Undefined"));
    params.append("itemlocdist_id", _itemlocdistid);

    MetaSQLQuery mql(sql);
    q = mql.toQuery(params);

    _itemloc->clear();
    XTreeWidgetItem *last = 0;
    while (q.next())
    {
      last = new XTreeWidgetItem(_itemloc, last,
				 q.value("id").toInt(), q.value("type").toInt(),
				 q.value("locationname"), q.value("defaultlocation"),
				 q.value("netable"), q.value("lotserial"),
				 q.value("f_expiration"),
				 formatNumber(q.value("qty").toDouble(),6),
				 formatNumber(q.value("qtytagged").toDouble(),6),
				 formatNumber(q.value("balance").toDouble(),6) );
      if (q.value("expired").toBool())
        last->setTextColor("red");
    }
  }
}

void distributeInventory::sBcDistribute()
{
  if (_bc->text().isEmpty())
  {
    QMessageBox::warning(this, tr("No Bar Code scanned"),
			 tr("<p>Cannot search for Items by Bar Code without a "
			    "Bar Code."));
    _bc->setFocus();
    return;
  }

  q.prepare( "SELECT itemloc_id "
	     "FROM  itemlocdist, itemloc, itemsite "
	     "WHERE ((itemlocdist_itemsite_id=itemloc_itemsite_id)"
	     "  AND  (itemloc_itemsite_id=itemsite_id)"
             "  AND  (itemsite_controlmethod IN ('L', 'S'))"
	     "  AND  (itemloc_lotserial=:lotserial)"
	     "  AND  (itemlocdist_id=:itemlocdist_id));");

  q.bindValue(":itemlocdist_id", _itemlocdistid);
  q.bindValue(":lotserial",      _bc->text());
  q.exec();

  if(!q.first())
  {
    if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
    QMessageBox::warning(this, tr("No Match Found"),
			 tr("<p>No available lines match the specified Barcode.")); 
    _bc->clear();
    return;
  }

  ParameterList params;
  params.append("itemlocdist_id",        q.value("itemloc_id"));
  params.append("source_itemlocdist_id", _itemlocdistid);
  params.append("qty",                   _bcQty->text());
  params.append("distribute");

  distributeToLocation newdlg(this, "", TRUE);
  if (newdlg.set(params) != NoError)
    return;

  _bc->clear();
  if (_controlMethod == "S")
    _bcQty->setText("1");
  else
    _bcQty->clear();
  sFillList();
  _bc->setFocus();
}

void distributeInventory::sCatchLotSerialNumber(const QString plotserial)
{
  //qDebug("sCatchLotSerialNumber");
  _bc->setText(plotserial);
  if (_controlMethod == "S")
    _bcDistribute->setFocus();
  else
    _bcQty->setFocus();
}

void distributeInventory::sBcChanged(const QString p)
{
  _post->setDefault(p.isEmpty());
  _bcDistribute->setDefault(! p.isEmpty());
}

void distributeInventory::sCancel()
{
  q.prepare( "DELETE FROM itemlocdist "
	         "WHERE (itemlocdist_id=:itemlocdistid); "
	         "DELETE FROM itemlocdist "
	         "WHERE (itemlocdist_itemlocdist_id=:itemlocdistid); ");
  q.bindValue(":itemlocdistid", _itemlocdistid);
  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(-1);
}
