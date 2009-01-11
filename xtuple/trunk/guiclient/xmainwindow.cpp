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

#include "xmainwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QWorkspace>
#include <QStatusBar>
#include <QSettings>
#include <QCloseEvent>
#include <QShowEvent>
#include <QtScript>
#include <QDebug>

#include "guiclient.h"
#include "scripttoolbox.h"

//
// XMainWindowPrivate
//
class XMainWindowPrivate
{
  friend class XMainWindow;

  public:
    XMainWindowPrivate();
    ~XMainWindowPrivate();

    bool _shown;
    QScriptEngine * _engine;
    QAction *_action;
};

XMainWindowPrivate::XMainWindowPrivate()
{
  _shown = false;
  _engine = 0;
  _action = 0;
}

XMainWindowPrivate::~XMainWindowPrivate()
{
  if(_engine)
    delete _engine;
  if(_action)
    delete _action;
}

XMainWindow::XMainWindow(QWidget * parent, Qt::WindowFlags flags)
  : QMainWindow(parent, flags)
{
  _private = new XMainWindowPrivate();

  _private->_action = new QAction(this);
  _private->_action->setShortcutContext(Qt::ApplicationShortcut);
  _private->_action->setText(windowTitle());
  _private->_action->setCheckable(true);
  connect(_private->_action, SIGNAL(triggered(bool)), this, SLOT(showMe(bool)));

  ScriptToolbox::setLastWindow(this);
}

XMainWindow::XMainWindow(QWidget * parent, const char * name, Qt::WindowFlags flags)
  : QMainWindow(parent, flags)
{
  if(name)
    setObjectName(name);

  _private = new XMainWindowPrivate();

  _private->_action = new QAction(this);
  _private->_action->setShortcutContext(Qt::ApplicationShortcut);
  _private->_action->setText(windowTitle());
  _private->_action->setCheckable(true);
  connect(_private->_action, SIGNAL(triggered(bool)), this, SLOT(showMe(bool)));
}

XMainWindow::~XMainWindow()
{
  if(_private)
    delete _private;
}

enum SetResponse XMainWindow::set(const ParameterList &params)
{
  enum SetResponse returnValue = NoError;
  _params = params;

  if (engine(this))
  {
    if (engine(this)->globalObject().property("set").isFunction())
    {
      QScriptValueList args;
      args << ParameterListtoScriptValue(engine(this), _params);
      QScriptValue tmp = engine(this)->globalObject().property("set").call(QScriptValue(), args);
      SetResponsefromScriptValue(tmp, returnValue);
    }
    else
      qDebug("engine's script doesn't have a set function");
  }

  return returnValue;
}

ParameterList XMainWindow::get() const
{
  return _params;
}

void XMainWindow::closeEvent(QCloseEvent *event)
{
  QString objName = objectName();
  QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenMFG");
  settings.setValue(objName + "/geometry/size", size());
  if(omfgThis->showTopLevel() || isModal())
    settings.setValue(objName + "/geometry/pos", pos());
  else
    settings.setValue(objName + "/geometry/pos", parentWidget()->pos());

  QMainWindow::closeEvent(event);
}

void XMainWindow::showEvent(QShowEvent *event)
{
  if(!_private->_shown)
  {
    _private->_shown = true;
//qDebug("isModal() %s", isModal()?"true":"false");

    QRect availableGeometry = QApplication::desktop()->availableGeometry();
    if(!omfgThis->showTopLevel() && !isModal())
      availableGeometry = omfgThis->workspace()->geometry();

    QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenMFG");
    QString objName = objectName();
    QPoint pos = settings.value(objName + "/geometry/pos").toPoint();
    QSize lsize = settings.value(objName + "/geometry/size").toSize();

    if(lsize.isValid() && settings.value(objName + "/geometry/rememberSize", true).toBool())
      resize(lsize);

    setAttribute(Qt::WA_DeleteOnClose);
    if(omfgThis->showTopLevel() || isModal())
    {
      omfgThis->_windowList.append(this);
      statusBar()->show();
      QRect r(pos, size());
      if(!pos.isNull() && availableGeometry.contains(r) && settings.value(objName + "/geometry/rememberPos", true).toBool())
        move(pos);
    }
    else
    {
      QWidget * fw = focusWidget();
      omfgThis->workspace()->addWindow(this);
      QRect r(pos, size());
      if(!pos.isNull() && availableGeometry.contains(r) && settings.value(objName + "/geometry/rememberPos", true).toBool())
        move(pos);
      // This originally had to be after the show? Will it work here?
      if(fw)
        fw->setFocus();
    }

    QStringList parts = objectName().split(" ");
    QStringList search_parts;
    QString oName;
    while(!parts.isEmpty())
    {
      search_parts.append(parts.takeFirst());
      oName = search_parts.join(" ");
      // load and run an QtScript that applies to this window
      qDebug() << "Looking for a script on window " << oName;
      q.prepare("SELECT script_source, script_order"
                "  FROM script"
                " WHERE((script_name=:script_name)"
                "   AND (script_enabled))"
                " ORDER BY script_order;");
      q.bindValue(":script_name", oName);
      q.exec();
      while(q.next())
      {
        QString script = q.value("script_source").toString();
        if(!_private->_engine)
        {
          _private->_engine = new QScriptEngine(this);
          omfgThis->loadScriptGlobals(_private->_engine);
          omfgThis->widgetScriptGlobals(centralWidget(), objectName(), _private->_engine);
          
          //This is legacy support now
          QScriptValue mywindow = _private->_engine->newQObject(this);
          _private->_engine->globalObject().setProperty("mywindow", mywindow);
        }
  
        QScriptValue result = _private->_engine->evaluate(script);
        if (_private->_engine->hasUncaughtException())
        {
          int line = _private->_engine->uncaughtExceptionLineNumber();
          qDebug() << "uncaught exception at line" << line << ":" << result.toString();
        }
      }
    }
  }

  bool blocked = _private->_action->blockSignals(true);
  _private->_action->setChecked(true);
  _private->_action->blockSignals(blocked);

  QMainWindow::showEvent(event);
}

void XMainWindow::hideEvent(QHideEvent * event)
{
  bool blocked = _private->_action->blockSignals(true);
  _private->_action->setChecked(false);
  _private->_action->blockSignals(blocked);

  QMainWindow::hideEvent(event);
}

QAction *XMainWindow::action() const
{
  return _private->_action;
}

void XMainWindow::changeEvent(QEvent *e)
{
  switch (e->type())
  {
    case QEvent::WindowTitleChange:
        _private->_action->setText(windowTitle());
        break;
    case QEvent::WindowIconChange:
        _private->_action->setIcon(windowIcon());
        break;
    default:
        break;
  }
  QMainWindow::changeEvent(e);
}

void XMainWindow::showMe(bool v)
{
  QWidget *target = parentWidget() == 0 ? this : parentWidget();

  if (v)
    target->setWindowState(target->windowState() & ~Qt::WindowMinimized);

  target->setVisible(v);
}

QScriptEngine *engine(XMainWindow *win)
{
  // copied from showEvent - why is it hidden there and not in the constructor?
  if(!win->_private->_engine)
  {
    win->_private->_engine = new QScriptEngine(win);
    omfgThis->loadScriptGlobals(win->_private->_engine);
    omfgThis->widgetScriptGlobals(win->centralWidget(), win->objectName(), win->_private->_engine);
    
    // This is legacy support now
    QScriptValue mywindow = win->_private->_engine->newQObject(win);
    win->_private->_engine->globalObject().setProperty("mywindow", mywindow);
  }

  return win->_private->_engine;
}
