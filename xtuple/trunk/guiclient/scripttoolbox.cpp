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

#include "scripttoolbox.h"

#include <QWidget>
#include <QLayout>
#include <QGridLayout>
#include <QBoxLayout>
#include <QStackedLayout>
#include <QMessageBox>
#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QDateTime>
#include <QUiLoader>
#include <QList>

#include <parameter.h>
#include <metasql.h>

#include "scriptquery.h"

ScriptToolbox::ScriptToolbox(QScriptEngine * engine)
  : QObject(engine)
{
  _engine = engine;
}

ScriptToolbox::~ScriptToolbox()
{
}

QObject * ScriptToolbox::executeQuery(const QString & query, QVariantMap parameters)
{
  ParameterList params;
  QMapIterator<QString, QVariant> i(parameters);
  while (i.hasNext())
  {
    i.next();
    params.append(i.key(), i.value());
  }

  ScriptQuery * sq = new ScriptQuery(_engine);
  MetaSQLQuery mql(query);
  sq->setQuery(mql.toQuery(params));
  return sq;
}

QObject * ScriptToolbox::widgetGetLayout(QWidget * w)
{
  QObject * p = w->parentWidget();
  while(p)
  {
    QList<QLayout*> list = p->findChildren<QLayout*>();
    for (int i = 0; i < list.size(); ++i)
    {
      if (list.at(i)->indexOf(w) != -1)
        return list.at(i);
    }
  }
  return NULL;
}

void ScriptToolbox::layoutBoxInsertWidget(QObject * obj, int index, QWidget * widget, int stretch, int alignment)
{
  QBoxLayout * layout = qobject_cast<QBoxLayout*>(obj);
  if(layout && widget)
    layout->insertWidget(index, widget, stretch, (Qt::Alignment)alignment);
}

void ScriptToolbox::layoutGridAddWidget(QObject * obj, QWidget * widget, int row, int column, int alignment)
{
  QGridLayout * layout = qobject_cast<QGridLayout*>(obj);
  if(layout && widget)
    layout->addWidget(widget, row, column, (Qt::Alignment)alignment);
}

void ScriptToolbox::layoutGridAddWidget(QObject * obj, QWidget * widget, int fromRow, int fromColumn, int rowSpan, int columnSpan, int alignment)
{
  QGridLayout * layout = qobject_cast<QGridLayout*>(obj);
  if(layout && widget)
    layout->addWidget(widget, fromRow, fromColumn, rowSpan, columnSpan, (Qt::Alignment)alignment);
}

void ScriptToolbox::layoutStackedInsertWidget(QObject * obj, int index, QWidget * widget)
{
  QStackedLayout * layout = qobject_cast<QStackedLayout*>(obj);
  if(layout && widget)
    layout->insertWidget(index, widget);
}

QWidget * ScriptToolbox::createWidget(const QString & className, QWidget * parent, const QString & name)
{
  QUiLoader ui;
  return ui.createWidget(className, parent, name);
}

int ScriptToolbox::messageBox(const QString & type, QWidget * parent, const QString & title, const QString & text, int buttons, int defaultButton)
{
  int btn;
  if(type == "critical")
    btn = QMessageBox::critical(parent, title, text, buttons, defaultButton);
  else if(type == "information")
    btn = QMessageBox::information(parent, title, text, buttons, defaultButton);
  else if(type == "question")
    btn = QMessageBox::question(parent, title, text, buttons, defaultButton);
  else //if(type == "warning")
    btn = QMessageBox::warning(parent, title, text, buttons, defaultButton);
  return btn;
}

QScriptValue ScriptToolbox::variantToScriptValue(QScriptEngine * engine, QVariant var)
{
  if(var.isNull())
    return engine->nullValue();

  switch(var.type())
  {
    case QVariant::Invalid:
      return engine->nullValue();
    case QVariant::Bool:
      return QScriptValue(engine, var.toBool());
    case QVariant::Int:
      return QScriptValue(engine, var.toInt());
    case QVariant::UInt:
      return QScriptValue(engine, var.toUInt());
    case QVariant::Double:
      return QScriptValue(engine, var.toDouble());
    case QVariant::Char:
      return QScriptValue(engine, var.toChar().unicode());
    case QVariant::String:
      return QScriptValue(engine, var.toString());
    case QVariant::LongLong:
      return QScriptValue(engine, qsreal(var.toLongLong()));
    case QVariant::ULongLong:
      return QScriptValue(engine, qsreal(var.toULongLong()));
    case QVariant::Date:
    case QVariant::Time:
    case QVariant::DateTime:
      return engine->newDate(var.toDateTime());
    case QVariant::RegExp:
      return engine->newRegExp(var.toRegExp());
/*
 * Would be ideal to have these as well but I don't know if they are really necessary
    case QVariant::StringList:
    case QVariant::List:

    case QVariant::Map:
*/
  }

  // If we are not doing an explicity conversion just pass the variant back
  // and see what happens
  return engine->newVariant(var);
}


// ParameterList Conversion functions
QScriptValue ParameterListtoScriptValue(QScriptEngine *engine, const ParameterList &params)
{
  QScriptValue obj = engine->newObject();
  for(int i = 0; i < params.count(); i++)
  {
    obj.setProperty(params.name(i), ScriptToolbox::variantToScriptValue(engine, params.value(i)));
  }

  return obj;
}

void ParameterListfromScriptValue(const QScriptValue &obj, ParameterList &params)
{
  QScriptValueIterator it(obj);
  while (it.hasNext())
  {
    it.next();
    if(it.flags() & QScriptValue::SkipInEnumeration)
      continue;
    params.append(it.name(), it.value().toVariant());
  }
}

QScriptValue SetResponsetoScriptValue(QScriptEngine *engine, const enum SetResponse &sr)
{
  return QScriptValue(engine, (int)sr);
}

void SetResponsefromScriptValue(const QScriptValue &obj, enum SetResponse &sr)
{
  sr = (enum SetResponse)obj.toInt32();
}
