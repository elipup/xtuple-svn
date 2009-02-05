/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "deliverEmail.h"

#include <qvariant.h>
#include <qmessagebox.h>

#include <openreports.h>
#include <parameter.h>

#include <QUrl>
#include <QDesktopServices>

/*
 *  Constructs a deliverEmail as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
deliverEmail::deliverEmail(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);

    _captive = FALSE;

    // signals and slots connections
    connect(_submit, SIGNAL(clicked()), this, SLOT(sSubmit()));
    connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
    
    

}

/*
 *  Destroys the object and frees any allocated resources
 */
deliverEmail::~deliverEmail()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void deliverEmail::languageChange()
{
    retranslateUi(this);
}

enum SetResponse deliverEmail::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;
  
  _captive = true;

  param = pParams.value("from", &valid);
  if (valid)
    _from->setText(param.toString());

  param = pParams.value("to", &valid);
  if (valid)
  {
    _to->setText(param.toString());
    if (!_to->text().isEmpty())
      _submit->setEnabled(TRUE);
  }
  
  param = pParams.value("cc", &valid);
  if (valid)
    _cc->setText(param.toString());
  
  param = pParams.value("subject", &valid);
  if (valid)
    _subject->setText(param.toString());
    
  param = pParams.value("body", &valid);
  if (valid)
    _body->setText(param.toString());
    
  param = pParams.value("fileName", &valid);
  if (valid)
    _filename->setText(param.toString());

  return NoError;
}

void deliverEmail::sSubmit()
{
  if (_to->text().isEmpty())
  {
    QMessageBox::critical( this, tr("Cannot Email for Delivery"),
                           tr("You must enter a email address to which this message is to be delivered.") );
    _to->setFocus();
    return;
  }
  
  if (_reportName.isEmpty())
    submitEmail(this,
         _from->text(),
         _to->text(),
         _cc->text(),
         _subject->text(),
         _body->toPlainText(),
         _emailHTML->isChecked());
  else
    submitReport(this,
         _reportName,
         _filename->text(),
         _from->text(),
         _to->text(),
         _cc->text(),
         _subject->text(),
         _body->toPlainText(),
         _emailHTML->isChecked(),
         _reportParams);
  
  if (_captive)
    accept();
  else
  {
    _submit->setEnabled(FALSE);
    _close->setText(tr("&Close"));
    _to->clear();
    _cc->clear();
    _body->clear();
    _subject->clear();
    _filename->clear();
  }
}


bool deliverEmail::profileEmail(QWidget *parent, int profileid, ParameterList &pParams, ParameterList &rptParams)
{
  if (!profileid)
    return false;

  QVariant param;
  bool     valid;
  
  bool    preview = false;
  QString reportName;
  QString fileName;
  
  //Token variables
  int     docid = 0;
  QString comments;
  QString descrip;
  QString docnumber;
  QString doctype;
  QString docbody;
  QString email1;
  QString email2; 
  QString email3; 
  
  //Email variables
  QString from;
  QString to;
  QString cc;
  QString subject;
  QString body;
    
  //Process parameters
  param = pParams.value("preview", &valid);
  if (valid)
    preview = param.toBool(); 
    
  param = pParams.value("reportName", &valid);
  if (valid)
    reportName = param.toString();   
    
  param = pParams.value("fileName", &valid);
  if (valid)
    fileName = param.toString();   
    
  param = pParams.value("description", &valid);
  if (valid)
    descrip = param.toString();
    
  param = pParams.value("docid", &valid);
  if (valid)
    docid = param.toInt();

  param = pParams.value("docbody", &valid);
  if (valid)
    docbody = param.toString();
    
  param = pParams.value("docnumber", &valid);
  if (valid)
    docnumber = param.toString();

  param = pParams.value("doctype", &valid);
  if (valid)
    doctype = param.toString();
    
  param = pParams.value("email1", &valid);
  if (valid)
    email1 = param.toString();
    
  param = pParams.value("email2", &valid);
  if (valid)
    email2 = param.toString();

  param = pParams.value("email3", &valid);
  if (valid)
    email3 = param.toString();

  //Get user email
  q.exec( "SELECT usr_email "
          "FROM usr "
          "WHERE (usr_username=CURRENT_USER);" );
  if (q.first())
    from=q.value("usr_email").toString();
    
  //Process profile
  q.prepare( "SELECT ediprofile_option1 AS emailto, "
             "  ediprofile_option4 AS emailcc, "
             "  ediprofile_option2 AS subject, "
             "  ediprofile_option3 AS body, "
             "  ediprofile_emailhtml "
             "FROM ediprofile "
             "WHERE ( (ediprofile_id=:ediprofile_id) "
             " AND (ediprofile_type='email') );" );
  q.bindValue(":ediprofile_id", profileid);
  q.exec();
  if (q.first())
  {
    to=q.value("emailto").toString();
    cc=q.value("emailcc").toString();
    body=QString(q.value("body").toString()
      .replace("</description>" , descrip)
      .replace("</docnumber>"   , docnumber)
      .replace("</doctype>"     , doctype)
      .replace("</docbody>"     , docbody));
    subject=QString(q.value("subject").toString()
      .replace("</description>" , descrip)
      .replace("</docnumber>"   , docnumber)
      .replace("</doctype>"     , doctype)
      .replace("</docbody>"     , docbody));
            
    //Handle E-mail
    //Don't send messages to myself
    if (email1 == to)
      email1 = QString::null;
    if (email2 == from)
      email2 = QString::null;
    if (email3 == from)
      email3 = QString::null;
      
    //Add token addresses to "To" list if applicable
    if ((email1.isEmpty()) || (to.count(email1,Qt::CaseInsensitive)))
      to=to.remove("</email1>");
    else
    {
      if (to.stripWhiteSpace()
            .remove("</email1>")
            .remove("</email2>")
            .remove("</email3>")
            .length())
        to=to.replace("</email1>", ", " + email1);
      else
        to=to.replace("</email1>", email1);
    }
  
    if ((email2.isEmpty()) || (to.count(email2,Qt::CaseInsensitive)))
      to=to.remove("</email2>");
    else
    {
      if (to.stripWhiteSpace()
            .remove("</email1>")
            .remove("</email2>")
            .remove("</email3>")
            .length())
        to=to.replace("</email2>", ", " + email2);
      else
        to=to.replace("</email2>", email2);
    }
        
    if ((email3.isEmpty()) || (to.count(email3,Qt::CaseInsensitive)))
      to=to.remove("</email3>");
    else
    {
      if (to.stripWhiteSpace()
            .remove("</email1>")
            .remove("</email2>")
            .remove("</email3>")
            .length())
        to=to.replace("</email3>", ", " + email3);
      else
        to=to.replace("</email3>", email3);
    }
    
    //Add token addresses to "CC" list if applicable
    if ((email1.isEmpty()) || (cc.count(email1,Qt::CaseInsensitive)))
      cc=cc.remove("</email1>");
    else
    {
      if (cc.stripWhiteSpace()
            .remove("</email1>")
            .remove("</email2>")
                        .remove("</email3>")
                        .length())
        cc=cc.replace("</email1>", ", " + email1);
      else
        cc=cc.replace("</email1>", email1);
    }
  
    if ((email2.isEmpty()) || (cc.count(email2,Qt::CaseInsensitive)))
      cc=cc.remove("</email2>");
    else
    {
      if (cc.stripWhiteSpace()
            .remove("</email1>")
            .remove("</email2>")
            .remove("</email3>")
            .length())
        cc=cc.replace("</email2>", ", " + email2);
      else
        cc=cc.replace("</email2>", email2);
    }
        
    if ((email3.isEmpty()) || (cc.count(email3,Qt::CaseInsensitive)))
      cc=cc.remove("</email3>");
    else
    {
      if (cc.stripWhiteSpace()
            .remove("</email1>")
            .remove("</email2>")
            .remove("</email3>")
            .length())
        cc=cc.replace("</email3>", ", " + email3);
      else
        cc=cc.replace("</email3>", email3);
    }
    
    //Build comment detail if applicable
    if (body.count("</comments>") &&
         !doctype.isEmpty() && docid)
    {                         
      q.prepare("SELECT comment_user, comment_date, comment_text "
                "FROM comment "
                "WHERE ( (comment_source=:doctype) "
                " AND (comment_source_id=:docid) ) "
                "ORDER BY comment_date;");
      q.bindValue(":doctype", doctype);
      q.bindValue(":docid", docid);
      q.exec();
      while (q.next())
      {
        comments += "-----------------------------------------------------\n";
        comments += q.value("comment_user").toString();
        comments += " - ";
        comments += q.value("comment_date").toString();
        comments += "\n-----------------------------------------------------\n";
        comments += q.value("comment_text").toString();
        comments += "\n\n";
      }
      
      body=body.replace("</comments>", comments);
    }
    
    if (preview)
    {
      ParameterList params;
      params.append("from", from);
      params.append("to", to);
      params.append("cc", cc);
      params.append("subject", subject);
      params.append("body", body);
      params.append("emailhtml", q.value("ediprofile_emailhtml").toBool());
      params.append("fileName", fileName);
       
      deliverEmail newdlg(parent, "", TRUE);
      newdlg.set(params);
      newdlg.setReportName(reportName);
      newdlg.setReportParameters(rptParams);
      if (newdlg.exec() == XDialog::Rejected)
        return false;
      else
        return true;
    }
    else if (reportName.isEmpty())
      submitEmail(parent,from,to,cc,subject,body,q.value("ediprofile_emailhtml").toBool());
    else
      submitReport(parent,reportName,fileName,from,to,cc,subject,body,q.value("ediprofile_emailhtml").toBool(),rptParams);
  }
  else
    return false;

  return true;
}

bool deliverEmail::submitEmail(QWidget* parent, const QString to, const QString cc, const QString subject, const QString body)
{
  QString from;
  
  //Get user email
  q.exec( "SELECT usr_email "
          "FROM usr "
          "WHERE (usr_username=CURRENT_USER);" );
  if (q.first())
    from=q.value("usr_email").toString();
  else
    return false;
    
  return submitEmail(parent,from,to,cc,subject,body);
}

bool deliverEmail::submitEmail(QWidget* parent, const QString from, const QString to, const QString cc, const QString subject, const QString body)
{
  return submitEmail(parent,from,to,cc,subject,body,false);
}

bool deliverEmail::submitEmail(QWidget* parent, const QString from, const QString to, const QString cc, const QString subject, const QString body, const bool emailHTML)
{
  if (to.isEmpty())
    return false;

  q.prepare( "SELECT submitEmailToBatch( :fromEmail, :emailAddress, :ccAddress, :subject,"
             "                            :emailBody, :fileName, CURRENT_TIMESTAMP, :emailHTML) AS batch_id;" );
  q.bindValue(":fromEmail", from);
  q.bindValue(":emailAddress", to);
  q.bindValue(":ccAddress", cc);
  q.bindValue(":subject", subject);
  q.bindValue(":emailBody", body);
  q.bindValue(":emailHTML", emailHTML);
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(parent, q.lastError().databaseText(), __FILE__, __LINE__);
    return false;
  }
  
  return true;
}

bool deliverEmail::submitReport(QWidget* parent, const QString reportName, const QString fileName, const QString from, const QString to, const QString cc, const QString subject, const QString body, const bool emailHTML, ParameterList &rptParams)
{
  if (to.isEmpty())
    return false;

  q.prepare( "SELECT submitReportToBatch( :reportname, :fromEmail, :emailAddress, :ccAddress, :subject,"
             "                            :emailBody, :fileName, CURRENT_TIMESTAMP, :emailHTML) AS batch_id;" );
  q.bindValue(":reportname", reportName);
  q.bindValue(":fileName", fileName);
  q.bindValue(":fromEmail", from);
  q.bindValue(":emailAddress", to);
  q.bindValue(":ccAddress", cc);
  q.bindValue(":subject", subject);
  q.bindValue(":emailBody", body);
  q.bindValue(":emailHTML", emailHTML);
  q.exec();
  if (q.first())
  {
    int batch_id = q.value("batch_id").toInt();
    int counter;
 
    q.prepare( "INSERT INTO batchparam "
             "( batchparam_batch_id, batchparam_order,"
             "  batchparam_name, batchparam_value ) "
             "VALUES "
             "( :batchparam_batch_id, :batchparam_order,"
             "  :batchparam_name, :batchparam_value );" );
    q.bindValue(":batchparam_batch_id", batch_id);
                 
    for (counter = 0; counter < rptParams.count(); counter++)
    {
      q.bindValue(":batchparam_order", counter+1);
      q.bindValue(":batchparam_name", rptParams.name(counter));
      q.bindValue(":batchparam_value", rptParams.value(counter));
      q.exec();
      if (q.lastError().type() != QSqlError::NoError)
      {
        systemError(parent, q.lastError().databaseText(), __FILE__, __LINE__);
        return false;
      }
    }

    q.bindValue(":batchparam_batch_id", batch_id);
    q.bindValue(":batchparam_order", counter+2);
    q.bindValue(":batchparam_name", "title");
    q.bindValue(":batchparam_value", "Emailed Customer Copy");
    q.exec();
    if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(parent, q.lastError().databaseText(), __FILE__, __LINE__);
      return false;
    }
  }
  
  return true;
}


