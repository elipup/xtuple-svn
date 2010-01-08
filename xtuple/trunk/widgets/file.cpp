/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include <QFileDialog>
#include <QUrl>

#include "file.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include <qvariant.h>

/*
 *  Constructs a file as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
file::file(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

    // signals and slots connections
    connect(_cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
    connect(_fileList, SIGNAL(clicked()), this, SLOT(sFileList()));
    connect(_fileButton, SIGNAL(toggled(bool)), this, SLOT(sHandleButtons()));
    connect(_internetButton, SIGNAL(toggled(bool)), this, SLOT(sHandleButtons()));

    _urlid = -1;
    _mode = cNew;
    _source = Documents::Uninitialized;
    _sourceid = -1;

#ifndef Q_WS_MAC
    _fileList->setMaximumWidth(25);
#else
    _fileList->setMinimumWidth(60);
    _fileList->setMinimumHeight(32);
#endif
    
    sHandleButtons();
}

/*
 *  Destroys the object and frees any allocated resources
 */
file::~file()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void file::languageChange()
{
    retranslateUi(this);
}

void file::set( const ParameterList & pParams )
{
  QVariant param;
  bool        valid;
  
  param = pParams.value("sourceType", &valid);
  if (valid)
    _source = (enum Documents::DocumentSources)param.toInt();
    
  param = pParams.value("source_id", &valid);
  if(valid)
    _sourceid = param.toInt();

  param = pParams.value("url_id", &valid);
  if(valid)
  {
    XSqlQuery q;
    _urlid = param.toInt();
    q.prepare("SELECT url_source, url_source_id, url_title, url_url"
              "  FROM url"
              " WHERE (url_id=:url_id);" );
    q.bindValue(":url_id", _urlid);
    q.exec();
    if(q.first())
    { 
      _title->setText(q.value("url_title").toString());
      
      QUrl url(q.value("url_url").toString());
      if (url.scheme().isEmpty())
      {
        url.setScheme("file");
        _url->setText(url.toString());
      }
      else 
      {
        _url->setText(url.toString());
        if (url.scheme() != "file")
          _internetButton->setChecked(true);
      }
    }
  }

  param = pParams.value("mode", &valid);
  if(valid)
  {
    if(param.toString() == "new")
      _mode = cNew;
    else if(param.toString() == "edit")
      _mode = cEdit;
    else if(param.toString() == "view")
    {
      _mode = cView;
      _save->hide();
      _title->setEnabled(false);
      _url->setEnabled(false);
    }
  }
}

void file::sSave()
{
  if(_url->text().trimmed().isEmpty())
  {
    QMessageBox::warning( this, tr("Must Specify file"),
      tr("You must specify a file before you may save.") );
    return;
  }
  
  QUrl url(_url->text());
  if (url.scheme().isEmpty())
  {
    if (_fileButton->isChecked())
      url.setScheme("file");
    else
      url.setScheme("http");
  }
  
  XSqlQuery q;

  if(cNew == _mode)
    q.prepare("INSERT INTO url"
              "       (url_source, url_source_id, url_title, url_url) "
              "VALUES (:source, :source_id, :title, :url); ");
  else //if(cEdit == _mode)
    q.prepare("UPDATE url"
              "   SET url_title=:title,"
              "       url_url=:url"
              " WHERE (url_id=:url_id); ");

  q.bindValue(":source", Documents::_documentMap[_source].ident);
  q.bindValue(":source_id", _sourceid);
  q.bindValue(":url_id", _urlid);
  q.bindValue(":title", _title->text().trimmed());
  q.bindValue(":url", url.toString());
  q.exec();

  accept();
}

void file::sHandleButtons()
{
  QUrl url(_url->text());
  
  if (_fileButton->isChecked())
  {
    _fileList->show();
    url.setScheme("file");
  }
  else
  {
    _fileList->hide();
    url.setScheme("http");
  }
  _url->setText(url.toString());
}

void file::sFileList()
{
  _url->setText(QString("file:%1").arg(QFileDialog::getOpenFileName( this, tr("Select File"), QString::null)));
}
