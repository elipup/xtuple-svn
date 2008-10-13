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
  if(_url->text().stripWhiteSpace().isEmpty())
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
  q.bindValue(":title", _title->text().stripWhiteSpace());
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
