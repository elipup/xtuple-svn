/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "booItem.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include "booitemImage.h"

static const char *costReportTypes[] = { "D", "O", "N" };

booItem::booItem(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_runTime, SIGNAL(textChanged(const QString&)), this, SLOT(sCalculateInvRunTime()));
  connect(_runTimePer, SIGNAL(textChanged(const QString&)), this, SLOT(sCalculateInvRunTime()));
  connect(_stdopn, SIGNAL(newID(int)), this, SLOT(sHandleStdopn(int)));
  connect(_fixedFont, SIGNAL(toggled(bool)), this, SLOT(sHandleFont(bool)));
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_invProdUOMRatio, SIGNAL(textChanged(const QString&)), this, SLOT(sCalculateInvRunTime()));
  connect(_wrkcnt, SIGNAL(newID(int)), this, SLOT(sPopulateLocations()));
  connect(_newImage, SIGNAL(clicked()), this, SLOT(sNewImage()));
  connect(_editImage, SIGNAL(clicked()), this, SLOT(sEditImage()));
  connect(_deleteImage, SIGNAL(clicked()), this, SLOT(sDeleteImage()));

  _booitemid = -1;
  _item->setReadOnly(TRUE);

  _dates->setStartNull(tr("Always"), omfgThis->startOfTime(), TRUE);
  _dates->setStartCaption(tr("Effective"));
  _dates->setEndNull(tr("Never"), omfgThis->endOfTime(), TRUE);
  _dates->setEndCaption(tr("Expires"));

  _prodUOM->setType(XComboBox::UOMs);

  _wrkcnt->populate( "SELECT wrkcnt_id, wrkcnt_code "
                     "FROM wrkcnt JOIN site() ON (warehous_id=wrkcnt_warehous_id) "
                     "ORDER BY wrkcnt_code" );

  _stdopn->populate( "SELECT -1, TEXT('None') AS stdopn_number "
                     "UNION "
                     "SELECT stdopn_id, stdopn_number "
                     "FROM stdopn LEFT OUTER JOIN wrkcnt ON (wrkcnt_id=stdopn_wrkcnt_id)"
                     "            LEFT OUTER JOIN site() ON (warehous_id=wrkcnt_warehous_id) "
                     "WHERE ( (stdopn_wrkcnt_id=-1)"
                     "   OR   (warehous_id IS NOT NULL) ) "
                     "ORDER BY stdopn_number" );

  _setupReport->insertItem(tr("Direct Labor"));
  _setupReport->insertItem(tr("Overhead"));
  _setupReport->insertItem(tr("None"));
  _setupReport->setCurrentIndex(-1);

  _runReport->insertItem(tr("Direct Labor"));
  _runReport->insertItem(tr("Overhead"));
  _runReport->insertItem(tr("None"));
  _runReport->setCurrentIndex(-1);

  _booimage->addColumn(tr("Image Name"),  _itemColumn, Qt::AlignLeft, true, "image_name");
  _booimage->addColumn(tr("Description"), -1,          Qt::AlignLeft, true, "descrip");
  _booimage->addColumn(tr("Purpose"),     _itemColumn, Qt::AlignLeft, true, "purpose");

  _runTimePer->setValidator(omfgThis->qtyVal());
  _invProdUOMRatio->setValidator(omfgThis->ratioVal());
  _invRunTime->setPrecision(omfgThis->runTimeVal());
  _invPerMinute->setPrecision(omfgThis->runTimeVal());
  _setupTime->setValidator(omfgThis->runTimeVal());
  _runTime->setValidator(omfgThis->runTimeVal());
  
  sHandleFont(_fixedFont->isChecked());

  // hide the Allow Pull Through option as it doesn't perform
  // any function at this time.
  _pullThrough->hide();
  adjustSize();
}

booItem::~booItem()
{
  // no need to delete child widgets, Qt does it all for us
}

void booItem::languageChange()
{
  retranslateUi(this);
}

enum SetResponse booItem::set(ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("booitem_id", &valid);
  if (valid)
  {
    _booitemid = param.toInt();
    populate();
  }

  param = pParams.value("item_id", &valid);
  if (valid)
  {
    _item->setId(param.toInt());
	if (_item->itemType() == "J")
	{
      _receiveStock->setEnabled(FALSE);
	  _receiveStock->setChecked(FALSE);
	}
  }

  param = pParams.value("revision_id", &valid);
  if (valid)
    _revisionid = param.toInt();

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      _stdopn->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      _save->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      disconnect(_booimage, SIGNAL(valid(bool)), _editImage, SLOT(setEnabled(bool)));
      disconnect(_booimage, SIGNAL(valid(bool)), _deleteImage, SLOT(setEnabled(bool)));

      _dates->setEnabled(FALSE);
      _executionDay->setEnabled(FALSE);
      _description1->setEnabled(FALSE);
      _description2->setEnabled(FALSE);
      _setupTime->setEnabled(FALSE);
      _prodUOM->setEnabled(FALSE);
      _invProdUOMRatio->setEnabled(FALSE);
      _runTime->setEnabled(FALSE);
      _runTimePer->setEnabled(FALSE);
      _reportSetup->setEnabled(FALSE);
      _reportRun->setEnabled(FALSE);
      _issueComp->setEnabled(FALSE);
      _receiveStock->setEnabled(FALSE);
      _wrkcnt->setEnabled(FALSE);
      _wipLocation->setEnabled(FALSE);
      _toolingReference->setEnabled(FALSE);
      _setupReport->setEnabled(FALSE);
      _runReport->setEnabled(FALSE);
      _stdopn->setEnabled(FALSE);
      _overlap->setEnabled(FALSE);
      _pullThrough->setEnabled(FALSE);
      _instructions->setEnabled(FALSE);
      _newImage->setEnabled(FALSE);
      _save->hide();
      _close->setText(tr("&Close"));

      _close->setFocus();
    }
  }

  return NoError;
}

void booItem::sSave()
{

  if (_wrkcnt->id() == -1)
  {
    QMessageBox::critical( this, tr("Cannot Save BOO Item"),
                           tr("You must select a Work Center for this BOO Item before you may save it.") );
    _wrkcnt->setFocus();
    return;
  }
  
  if (_setupReport->currentIndex() == -1)
  {
    QMessageBox::critical( this, tr("Cannot Save BOO Item"),
                           tr("You must select a Setup Cost reporting method for this BOO item before you may save it.") );
    _setupReport->setFocus();
    return;
  }

  if (_runReport->currentIndex() == -1)
  {
    QMessageBox::critical( this, tr("Cannot Save BOO Item"),
                           tr("You must select a Run Cost reporting method for this BOO item before you may save it.") );
    _runReport->setFocus();
    return;
  }  
  
  if (_receiveStock->isChecked())
  {
    q.prepare( "UPDATE booitem "
               "SET booitem_rcvinv=FALSE "
               "WHERE (booitem_item_id=:item_id);" );
    q.bindValue(":item_id", _item->id());
    q.exec();
    if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }

  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('booitem_booitem_id_seq') AS _booitem_id;");
    if (q.first())
      _booitemid = q.value("_booitem_id").toInt();
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

      q.prepare( "INSERT INTO booitem "
                 "( booitem_effective, booitem_expires, booitem_execday,"
                 "  booitem_id, booitem_item_id,"
                 "  booitem_seqnumber,"
                 "  booitem_wrkcnt_id, booitem_stdopn_id,"
                 "  booitem_descrip1, booitem_descrip2,"
                 "  booitem_toolref,"
                 "  booitem_sutime, booitem_sucosttype, booitem_surpt,"
                 "  booitem_rntime, booitem_rncosttype, booitem_rnrpt,"
                 "  booitem_produom, booitem_invproduomratio,"
                 "  booitem_rnqtyper,"
                 "  booitem_issuecomp, booitem_rcvinv,"
                 "  booitem_pullthrough, booitem_overlap,"
                 "  booitem_configtype, booitem_configid, booitem_configflag,"
                 "  booitem_instruc, booitem_wip_location_id, booitem_rev_id ) "
                 "VALUES "
                 "( :effective, :expires, :booitem_execday,"
                 "  :booitem_id, :booitem_item_id,"
                 "  ((SELECT COALESCE(MAX(booitem_seqnumber), 0) FROM booitem WHERE (booitem_item_id=:booitem_item_id)) + 10),"
                 "  :booitem_wrkcnt_id, :booitem_stdopn_id,"
                 "  :booitem_descrip1, :booitem_descrip2,"
                 "  :booitem_toolref,"
                 "  :booitem_sutime, :booitem_sucosttype, :booitem_surpt,"
                 "  :booitem_rntime, :booitem_rncosttype, :booitem_rnrpt,"
                 "  :booitem_produom, :booitem_invproduomratio,"
                 "  :booitem_rnqtyper,"
                 "  :booitem_issuecomp, :booitem_rcvinv,"
                 "  :booitem_pullthrough, :booitem_overlap,"
                 "  :booitem_configtype, :booitem_configid, :booitem_configflag,"
		 "  :booitem_instruc, :booitem_wip_location_id, :booitem_rev_id );" );
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE booitem "
               "SET booitem_effective=:effective, booitem_expires=:expires, booitem_execday=:booitem_execday,"
               "    booitem_wrkcnt_id=:booitem_wrkcnt_id, booitem_stdopn_id=:booitem_stdopn_id,"
               "    booitem_descrip1=:booitem_descrip1, booitem_descrip2=:booitem_descrip2,"
               "    booitem_toolref=:booitem_toolref,"
               "    booitem_sutime=:booitem_sutime, booitem_sucosttype=:booitem_sucosttype, booitem_surpt=:booitem_surpt,"
               "    booitem_rntime=:booitem_rntime, booitem_rncosttype=:booitem_rncosttype, booitem_rnrpt=:booitem_rnrpt,"
               "    booitem_produom=:booitem_produom, booitem_invproduomratio=:booitem_invproduomratio, booitem_rnqtyper=:booitem_rnqtyper,"
               "    booitem_issuecomp=:booitem_issuecomp, booitem_rcvinv=:booitem_rcvinv,"
               "    booitem_pullthrough=:booitem_pullthrough, booitem_overlap=:booitem_overlap,"
               "    booitem_configtype=:booitem_configtype, booitem_configid=:booitem_configid, booitem_configflag=:booitem_configflag,"
               "    booitem_instruc=:booitem_instruc, booitem_wip_location_id=:booitem_wip_location_id "
               "WHERE (booitem_id=:booitem_id);" );

  q.bindValue(":booitem_id", _booitemid);
  q.bindValue(":booitem_item_id", _item->id());
  q.bindValue(":effective", _dates->startDate());
  q.bindValue(":expires", _dates->endDate());
  q.bindValue(":booitem_execday", _executionDay->value());
  q.bindValue(":booitem_descrip1", _description1->text());
  q.bindValue(":booitem_descrip2", _description2->text());
  q.bindValue(":booitem_produom", _prodUOM->currentText());
  q.bindValue(":booitem_toolref", _toolingReference->text());
  q.bindValue(":booitem_instruc",         _instructions->toPlainText());
  q.bindValue(":booitem_invproduomratio", _invProdUOMRatio->toDouble());
  q.bindValue(":booitem_sutime", _setupTime->toDouble());
  q.bindValue(":booitem_rntime", _runTime->toDouble());
  q.bindValue(":booitem_sucosttype", costReportTypes[_setupReport->currentIndex()]);
  q.bindValue(":booitem_rncosttype", costReportTypes[_runReport->currentIndex()]);
  q.bindValue(":booitem_rnqtyper", _runTimePer->toDouble());
  q.bindValue(":booitem_rnrpt",       QVariant(_reportRun->isChecked()));
  q.bindValue(":booitem_surpt",       QVariant(_reportSetup->isChecked()));
  q.bindValue(":booitem_issuecomp",   QVariant(_issueComp->isChecked()));
  q.bindValue(":booitem_rcvinv",      QVariant(_receiveStock->isChecked()));
  q.bindValue(":booitem_pullthrough", QVariant(_pullThrough->isChecked()));
  q.bindValue(":booitem_overlap",     QVariant(_overlap->isChecked()));
  q.bindValue(":booitem_wrkcnt_id", _wrkcnt->id());
  q.bindValue(":booitem_wip_location_id", _wipLocation->id());
  q.bindValue(":booitem_stdopn_id", _stdopn->id());
  q.bindValue(":booitem_configtype", "N");
  q.bindValue(":booitem_configid", -1);
  q.bindValue(":booitem_configflag",  QVariant(FALSE));
  q.bindValue(":booitem_rev_id", _revisionid);
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  omfgThis->sBOOsUpdated(_booitemid, TRUE);
  done(_booitemid);
}

void booItem::sHandleStdopn(int pStdopnid)
{
  if (_stdopn->id() != -1)
  {
    q.prepare( "SELECT * "
               "FROM stdopn "
               "WHERE (stdopn_id=:stdopn_id);" );
    q.bindValue(":stdopn_id", pStdopnid);
    q.exec();
    if (q.first())
    {
      _description1->setEnabled(FALSE);
      _description2->setEnabled(FALSE);
      _instructions->setEnabled(FALSE);

      _description1->setText(q.value("stdopn_descrip1"));
      _description2->setText(q.value("stdopn_descrip2"));
      _instructions->setText(q.value("stdopn_instructions").toString());
      _toolingReference->setText(q.value("stdopn_toolref"));
      _wrkcnt->setId(q.value("stdopn_wrkcnt_id").toInt());

      if (q.value("stdopn_stdtimes").toBool())
      {
        _setupTime->setDouble(q.value("stdopn_sutime").toDouble());
        _runTime->setDouble(q.value("stdopn_rntime").toDouble());
        _runTimePer->setDouble(q.value("stdopn_rnqtyper").toDouble());
        _prodUOM->setText(q.value("stdopn_produom"));
        _invProdUOMRatio->setDouble(q.value("stdopn_invproduomratio").toDouble());

        if (q.value("stdopn_sucosttype").toString() == "D")
          _setupReport->setCurrentIndex(0);
        else if (q.value("stdopn_sucosttype").toString() == "O")
          _setupReport->setCurrentIndex(1);
        else if (q.value("stdopn_sucosttype").toString() == "N")
          _setupReport->setCurrentIndex(2);

        if (q.value("stdopn_rncosttype").toString() == "D")
          _runReport->setCurrentIndex(0);
        else if (q.value("stdopn_rncosttype").toString() == "O")
          _runReport->setCurrentIndex(1);
        else if (q.value("stdopn_rncosttype").toString() == "N")
          _runReport->setCurrentIndex(2);
      }
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else
  {
    _description1->setEnabled(TRUE);
    _description1->clear();
    _description2->setEnabled(TRUE);
    _description2->clear();
    _instructions->setEnabled(TRUE);
    _instructions->clear();
  }
}

void booItem::sCalculateInvRunTime()
{
  if ((_runTimePer->toDouble() != 0.0) && (_invProdUOMRatio->toDouble() != 0.0))
  {
    _invRunTime->setDouble(_runTime->toDouble() / _runTimePer->toDouble() / _invProdUOMRatio->toDouble());

    _invPerMinute->setDouble(_runTimePer->toDouble() / _runTime->toDouble() * _invProdUOMRatio->toDouble());

  }
  else
  {
    _invRunTime->setDouble(0.0);
    _invPerMinute->setDouble(0.0);
  }
}

void booItem::sHandleFont(bool pFixed)
{
  if (pFixed)
    _instructions->setFont(omfgThis->fixedFont());
  else
    _instructions->setFont(omfgThis->systemFont());
}

void booItem::populate()
{
  XSqlQuery booitem;
  booitem.prepare( "SELECT item_config,"
                   "       booitem_effective, booitem_expires,"
                   "       booitem_execday, booitem_item_id, booitem_seqnumber,"
                   "       booitem_wrkcnt_id, booitem_stdopn_id,"
                   "       booitem_descrip1, booitem_descrip2, booitem_toolref,"
                   "       booitem_sutime, booitem_sucosttype, booitem_surpt,"
                   "       booitem_rntime, booitem_rncosttype, booitem_rnrpt,"
                   "       booitem_produom, uom_name,"
                   "       booitem_invproduomratio,"
                   "       booitem_rnqtyper,"
                   "       booitem_issuecomp, booitem_rcvinv,"
                   "       booitem_pullthrough, booitem_overlap,"
                   "       booitem_configtype, booitem_configid, booitem_configflag,"
                   "       booitem_instruc, booitem_wip_location_id "
                   "FROM booitem, item, uom "
                   "WHERE ( (booitem_item_id=item_id)"
                   " AND (item_inv_uom_id=uom_id)"
                   " AND (booitem_id=:booitem_id) );" );
  booitem.bindValue(":booitem_id", _booitemid);
  booitem.exec();
  if (booitem.first())
  {
    _stdopn->setId(booitem.value("booitem_stdopn_id").toInt());
    if (booitem.value("booitem_stdopn_id").toInt() != -1)
    {
      _description1->setEnabled(FALSE);
      _description2->setEnabled(FALSE);
      _instructions->setEnabled(FALSE);
    }

    _dates->setStartDate(booitem.value("booitem_effective").toDate());
    _dates->setEndDate(booitem.value("booitem_expires").toDate());
    _executionDay->setValue(booitem.value("booitem_execday").toInt());
    _operSeqNum->setText(booitem.value("booitem_seqnumber").toString());
    _description1->setText(booitem.value("booitem_descrip1"));
    _description2->setText(booitem.value("booitem_descrip2"));
    _toolingReference->setText(booitem.value("booitem_toolref"));
    _setupTime->setDouble(booitem.value("booitem_sutime").toDouble());
    _prodUOM->setText(booitem.value("booitem_produom"));
    _invUOM1->setText(booitem.value("uom_name").toString());
    _invUOM2->setText(booitem.value("uom_name").toString());
    _invProdUOMRatio->setDouble(booitem.value("booitem_invproduomratio").toDouble());
    _runTime->setDouble(booitem.value("booitem_rntime").toDouble());
    _runTimePer->setDouble(booitem.value("booitem_rnqtyper").toDouble());

    _reportSetup->setChecked(booitem.value("booitem_surpt").toBool());
    _reportRun->setChecked(booitem.value("booitem_rnrpt").toBool());
    _issueComp->setChecked(booitem.value("booitem_issuecomp").toBool());
    _receiveStock->setChecked(booitem.value("booitem_rcvinv").toBool());
    _overlap->setChecked(booitem.value("booitem_overlap").toBool());
    _pullThrough->setChecked(booitem.value("booitem_pullthrough").toBool());
    _instructions->setText(booitem.value("booitem_instruc").toString());
    _wrkcnt->setId(booitem.value("booitem_wrkcnt_id").toInt());
    _wipLocation->setId(booitem.value("booitem_wip_location_id").toInt());

    if (booitem.value("booitem_sucosttype").toString() == "D")
      _setupReport->setCurrentIndex(0);
    else if (booitem.value("booitem_sucosttype").toString() == "O")
      _setupReport->setCurrentIndex(1);
    else if (booitem.value("booitem_sucosttype").toString() == "N")
      _setupReport->setCurrentIndex(2);

    if (booitem.value("booitem_rncosttype").toString() == "D")
      _runReport->setCurrentIndex(0);
    else if (booitem.value("booitem_rncosttype").toString() == "O")
      _runReport->setCurrentIndex(1);
    else if (booitem.value("booitem_rncosttype").toString() == "N")
      _runReport->setCurrentIndex(2);

    _item->setId(booitem.value("booitem_item_id").toInt());
	if (_item->itemType() == "J")
	{
      _receiveStock->setEnabled(FALSE);
	  _receiveStock->setChecked(FALSE);
	}

    sCalculateInvRunTime();
    sFillImageList();
  }
  else if (booitem.lastError().type() != QSqlError::NoError)
  {
    systemError(this, booitem.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void booItem::sPopulateLocations()
{
  int locid = _wipLocation->id();

  XSqlQuery loc;
  loc.prepare("SELECT location_id, formatLocationName(location_id) AS locationname"
              "  FROM location, wrkcnt"
              " WHERE ( (location_warehous_id=wrkcnt_warehous_id)"
              "   AND   (NOT location_restrict)"
              "   AND   (wrkcnt_id=:wrkcnt_id) ) "
              "UNION "
              "SELECT location_id, formatLocationName(location_id) AS locationname"
              "  FROM locitem, location, wrkcnt"
              " WHERE ( (location_warehous_id=wrkcnt_warehous_id)"
              "   AND   (location_restrict)"
              "   AND   (locitem_location_id=location_id)"
              "   AND   (locitem_item_id=:item_id)"
              "   AND   (wrkcnt_id=:wrkcnt_id) )"
              " ORDER BY locationname; ");
  loc.bindValue(":wrkcnt_id", _wrkcnt->id());
  loc.bindValue(":item_id", _item->id());
  loc.exec();

  _wipLocation->populate(loc, locid);
  if (loc.lastError().type() != QSqlError::NoError)
  {
    systemError(this, loc.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void booItem::sNewImage()
{
  if(cNew == _mode)
  {
    QMessageBox::information( this, tr("Must Save BOO Item"),
      tr("You must save the BOO Item before you can add images to it.") );
    return;
  }

  ParameterList params;
  params.append("mode", "new");
  params.append("booitem_id", _booitemid);

  booitemImage newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillImageList();
}

void booItem::sEditImage()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("booimage_id", _booimage->id());

  booitemImage newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillImageList();
}

void booItem::sDeleteImage()
{
  q.prepare( "DELETE FROM booimage "
             "WHERE (booimage_id=:booimage_id);" );
  q.bindValue(":booimage_id", _booimage->id());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillImageList();
}

void booItem::sFillImageList()
{
  q.prepare( "SELECT booimage_id, image_name, firstLine(image_descrip) AS descrip,"
             "       CASE WHEN (booimage_purpose='I') THEN :inventory"
             "            WHEN (booimage_purpose='P') THEN :product"
             "            WHEN (booimage_purpose='E') THEN :engineering"
             "            WHEN (booimage_purpose='M') THEN :misc"
             "            ELSE :other"
             "       END AS purpose "
             "FROM booimage, image "
             "WHERE ( (booimage_image_id=image_id)"
             " AND (booimage_booitem_id=:booitem_id) ) "
             "ORDER BY image_name;" );
  q.bindValue(":inventory", tr("Inventory Description"));
  q.bindValue(":product", tr("Product Description"));
  q.bindValue(":engineering", tr("Engineering Reference"));
  q.bindValue(":misc", tr("Miscellaneous"));
  q.bindValue(":other", tr("Other"));
  q.bindValue(":booitem_id", _booitemid);
  q.exec();
  _booimage->populate(q);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
