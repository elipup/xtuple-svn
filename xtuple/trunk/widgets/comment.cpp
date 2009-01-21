/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include <QHBoxLayout>

#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlError>
#include <QTextEdit>
#include <QToolTip>
#include <QVBoxLayout>
#include <QVariant>
#include <QWhatsThis>
#include <QVector>

#include <parameter.h>

#include "xcombobox.h"
#include "comment.h"

#define cNew  1
#define cEdit 2
#define cView 3

comment::comment( QWidget* parent, const char* name, bool modal, Qt::WFlags fl ) :
  QDialog( parent, name, modal, fl )
{
  setWindowTitle(tr("Comment"));

  _commentid = -1;
  _targetId = -1;
  _mode = cNew;

  if (!name)
    setObjectName("comment");

  QHBoxLayout *commentLayout = new QHBoxLayout( this, 5, 7, "commentLayout"); 
  QVBoxLayout *layout11  = new QVBoxLayout( 0, 0, 5, "layout11"); 
  QHBoxLayout *layout9   = new QHBoxLayout( 0, 0, 0, "layout9"); 
  QBoxLayout *layout8    = new QHBoxLayout( 0, 0, 5, "layout8"); 
  QVBoxLayout *Layout181 = new QVBoxLayout( 0, 0, 0, "Layout181"); 
  QVBoxLayout *Layout180 = new QVBoxLayout( 0, 0, 5, "Layout180"); 

  QLabel *_cmnttypeLit = new QLabel(tr("Comment Type:"), this, "_cmnttypeLit");
  layout8->addWidget( _cmnttypeLit );

  _cmnttype = new XComboBox( FALSE, this, "_cmnttype" );
  layout8->addWidget( _cmnttype );
  layout9->addLayout( layout8 );

  QSpacerItem* spacer = new QSpacerItem( 66, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
  layout9->addItem( spacer );
  layout11->addLayout( layout9 );

  _comment = new QTextEdit( this, "_comment" );
  layout11->addWidget( _comment );
  commentLayout->addLayout( layout11 );

  _close = new QPushButton(tr("&Cancel"), this, "_close");
  Layout180->addWidget( _close );

  _save = new QPushButton(tr("&Save"), this, "_save");
  Layout180->addWidget( _save );

  _prev = new QPushButton(tr("&Previous"), this, "_prev");
  Layout180->addWidget( _prev );

  _next = new QPushButton(tr("&Next"), this, "_next");
  Layout180->addWidget( _next );

  Layout181->addLayout( Layout180 );
  QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  Layout181->addItem( spacer_2 );
  commentLayout->addLayout( Layout181 );

  resize( QSize(524, 270).expandedTo(minimumSizeHint()) );
  //clearWState( WState_Polished );

// signals and slots connections
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_next, SIGNAL(clicked()), this, SLOT(sNextComment()));
  connect(_prev, SIGNAL(clicked()), this, SLOT(sPrevComment()));

// tab order
  setTabOrder( _cmnttype, _comment );
  setTabOrder( _comment, _save );
  setTabOrder( _save, _close );

  _source = Comments::Uninitialized;
  _cmnttype->setAllowNull(TRUE);
}

void comment::set(ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("comment_id", &valid);
  if (valid)
  {
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _commentid = param.toInt();
    populate();
  }

  param = pParams.value("commentIDList", &valid);
  if (valid)
  {
    _commentIDList = param.toList();
    _commentLocation = _commentIDList.indexOf(_commentid);

    if((_commentLocation-1) >= 0)
      _prev->setEnabled(true); 
    else
      _prev->setEnabled(false); 

    if((_commentLocation+1) < _commentIDList.size())
      _next->setEnabled(true); 
    else
      _next->setEnabled(false); 
  }

  param = pParams.value("cust_id", &valid);
  if (valid)
  {
    _source = Comments::Customer;
    _cmnttype->setType(XComboBox::CustomerCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("vend_id", &valid);
  if (valid)
  {
    _source = Comments::Vendor;
    _cmnttype->setType(XComboBox::VendorCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("item_id", &valid);
  if (valid)
  {
    _source = Comments::Item;
    _cmnttype->setType(XComboBox::ItemCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("itemsite_id", &valid);
  if (valid)
  {
    _source = Comments::ItemSite;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }


//  Quotes
  param = pParams.value("quhead_id", &valid);
  if (valid)
  {
    _source = Comments::Quote;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("quitem_id", &valid);
  if (valid)
  {
    _source = Comments::QuoteItem;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }


//  Sales Orders
  param = pParams.value("sohead_id", &valid);
  if (valid)
  {
    _source = Comments::SalesOrder;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("soitem_id", &valid);
  if (valid)
  {
    _source = Comments::SalesOrderItem;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

//  Sales Orders
  param = pParams.value("tohead_id", &valid);
  if (valid)
  {
    _source = Comments::TransferOrder;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

//  Return Authorizations
  param = pParams.value("rahead_id", &valid);
  if (valid)
  {
    _source = Comments::ReturnAuth;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("raitem_id", &valid);
  if (valid)
  {
    _source = Comments::ReturnAuthItem;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

//  Purchase Orders
  param = pParams.value("pohead_id", &valid);
  if (valid)
  {
    _source = Comments::PurchaseOrder;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("poitem_id", &valid);
  if (valid)
  {
    _source = Comments::PurchaseOrderItem;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }


  param = pParams.value("lsdetail_id", &valid);
  if (valid)
  {
    _source = Comments::LotSerial;
    _cmnttype->setType(XComboBox::LotSerialCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("prj_id", &valid);
  if (valid)
  {
    _source = Comments::Project;
    _cmnttype->setType(XComboBox::ProjectCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("warehous_id", &valid);
  if (valid)
  {
    _source = Comments::Warehouse;
    _cmnttype->setType(XComboBox::ProjectCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("addr_id", &valid);
  if (valid)
  {
    _source = Comments::Address;
    _cmnttype->setType(XComboBox::AllCommentTypes);
    _targetId = param.toInt();
  }

  param = pParams.value("sourceType", &valid);
  if (valid)
  {
    _source = (enum Comments::CommentSources)param.toInt();
    switch (_source)
    {
      default:
        _cmnttype->setType(XComboBox::AllCommentTypes);
        break;
    }
  }

  param = pParams.value("source_id", &valid);
  if (valid)
    _targetId = param.toInt();

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      
      _comment->setFocus();
      _next->setVisible(false);
      _prev->setVisible(false);
    }
    else if (param.toString() == "view")
    {
      _mode = cView;
      _next->setVisible(true);
      _prev->setVisible(true);
      _cmnttype->setEnabled(FALSE);
      _comment->setReadOnly(TRUE);
      _save->hide();
      _close->setText(tr("&Close"));

      _close->setFocus();
    }
  }
}

void comment::sSave()
{
  if (_cmnttype->id() == -1)
  {
    QMessageBox::critical( this, tr("Cannot Post Comment"),
                           tr("<p>You must select a Comment Type for this "
                              "Comment before you may post it.") );
    _cmnttype->setFocus();
    return;
  }

  _query.prepare("SELECT postComment(:cmnttype_id, :source, :source_id, :text) AS result;");
  _query.bindValue(":cmnttype_id", _cmnttype->id());
  _query.bindValue(":source", Comments::_commentMap[_source].ident);
  _query.bindValue(":source_id", _targetId);
  _query.bindValue(":text", _comment->toPlainText().trimmed());
  _query.exec();
  if (_query.first())
  {
    int result = _query.value("result").toInt();
    if (result < 0)
    {
      QMessageBox::critical(this, tr("Cannot Post Comment"),
                            tr("<p>A Stored Procedure failed to run "
                               "properly.<br>(%1, %2)<br>")
                              .arg("postComment").arg(result));
      reject();
    }
    done (_query.value("result").toInt());
  }
  else if (_query.lastError().type() != QSqlError::NoError)
  {
    QMessageBox::critical(this, tr("Cannot Post Comment"),
                          _query.lastError().databaseText());
    reject();
  }
}

void comment::populate()
{
  _query.prepare( "SELECT comment_cmnttype_id, comment_text "
                  "FROM comment "
                  "WHERE (comment_id=:comment_id);" );
  _query.bindValue(":comment_id", _commentid);
  _query.exec();
  if (_query.first())
  {
    _cmnttype->setId(_query.value("comment_cmnttype_id").toInt());
    _comment->setText(_query.value("comment_text").toString());
  }
  else if (_query.lastError().type() != QSqlError::NoError)
  {
    QMessageBox::critical(this, tr("Error Selecting Comment"),
                          _query.lastError().databaseText());
    return;
  }
}

void comment::sNextComment()
{
  if((_commentLocation+1) < _commentIDList.size())
  {
    _commentLocation++;
    _commentid = _commentIDList[_commentLocation].toInt();
    populate();
    if((_commentLocation+1) < _commentIDList.size())
      _next->setEnabled(true); 
    else
      _next->setEnabled(false); 

    if((_commentLocation-1) >= 0)
      _prev->setEnabled(true); 
    else
      _prev->setEnabled(false); 
  }
}

void comment::sPrevComment()
{
  if((_commentLocation-1) >= 0)
  {
    _commentLocation--;
    _commentid = _commentIDList[_commentLocation].toInt();
    populate();
    if((_commentLocation-1) >= 0)
      _prev->setEnabled(true); 
    else
      _prev->setEnabled(false); 

    if((_commentLocation+1) < _commentIDList.size())
      _next->setEnabled(true); 
    else
      _next->setEnabled(false); 
  }
}
