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

//  main.cpp
//  Created 12/07/1999 JSL
//  Copyright (c) 1999-2007, OpenMFG, LLC

#include <stdlib.h>

#include <qfont.h>
#include <qtranslator.h>
#include <qsqldatabase.h>
#include <qwindowsstyle.h>
#include <qfile.h>

#ifdef Q_WS_MACX
#include <qmacstyle_mac.h>
#endif

#include <QMessageBox>
#include <QSplashScreen>
#include <qimage.h>
#include <qapplication.h>
//Added by qt3to4:
#include <QPixmap>

#ifdef Q_WS_WIN
#include <windows.h>
#include <qstylefactory.h>
#endif

#include <dbtools.h>
#include <parameter.h>
#include <OpenMFGWidgets.h>

#include "login2.h"

#include "OpenMFGGUIClient.h"
#include "version.h"
#include "metrics.h"
#include "metricsenc.h"

#include "woTimeClock.h"

#include "splashconst.h"

int main(int argc, char *argv[])
{
  Q_INIT_RESOURCE(guiclient);

  QString username;
  QString databaseURL;
  bool    haveUsername    = FALSE;
  bool    haveDatabaseURL = FALSE;
  bool    loggedIn        = FALSE;

#ifdef XQ_WS_WIN
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(1, 1), &wsaData))
  {
    qDebug("Error starting up Windows Socket system... Database services are not avalable.");
    exit(-1);
  }
#endif

  QApplication app(argc, argv);
  app.addLibraryPath(".");

#ifdef Q_WS_WIN
  if (app.winVersion() == QSysInfo::WV_XP)
    app.setStyle(QStyleFactory::create("windowsxpstyle"));
#if QT_VERSION >= 0x040300 
  else if (app.winVersion() == QSysInfo::WV_VISTA)
    app.setStyle(QStyleFactory::create("windowsvistastyle"));
#endif
  else
    app.setStyle(new QWindowsStyle);
#elif defined Q_WS_MACX
  app.setStyle(new QMacStyle);
#elif defined Q_WS_X11
  app.setStyle(new QWindowsStyle);
#endif

  // Try and load a default translation file and install it
  QTranslator defaultTranslator(0);
  if (defaultTranslator.load("default.qm", app.applicationDirPath()))
    app.installTranslator(&defaultTranslator);

  app.processEvents();

  if (argc > 1)
  {
    bool    havePasswd          = FALSE;
    bool    interactive         = TRUE;
    bool    createCountTags     = FALSE;
    QString passwd;

    for (int intCounter = 1; intCounter < argc; intCounter++)
    {
      QString argument(argv[intCounter]);

      if (argument.contains("-databaseURL="))
      {
        haveDatabaseURL = TRUE;
        databaseURL = argument.right(argument.length() - 13);
      }
      else if (argument.contains("-username="))
      {
        haveUsername = TRUE;
        username = argument.right(argument.length() - 10);
      }
      else if (argument.contains("-passwd="))
      {
        havePasswd = TRUE;
        passwd     = argument.right(argument.length() - 8);
      }
      else if (argument.contains("-noAuth"))
      {
        haveUsername = TRUE;
        havePasswd   = TRUE;
      } 

      else if (argument.contains("-createCountTags"))
      {
        interactive = FALSE;
        createCountTags = TRUE;
      }
    }

    if ( (haveDatabaseURL) && (haveUsername) && (havePasswd) )
    {
      QSqlDatabase db;
      QString      hostName;
      QString      dbName;
      QString      port;

      db = QSqlDatabase::addDatabase("QPSQL7");
      if (!db.isValid())
      {
        qDebug("Cannot load Database Driver.  Contact your Systems Administrator.");
        exit(-1);
      }

      QString protocol;
      parseDatabaseURL(databaseURL, protocol, hostName, dbName, port);
      db.setDatabaseName(dbName);
      db.setUserName(username);
      db.setPassword(passwd);
      db.setHostName(hostName);
      bool valport = false;
      int iport = port.toInt(&valport);
      if(!valport) iport = 5432;
      db.setPort(iport);

      if (!db.open())
      {
        qDebug( QObject::tr( "Cannot log onto the database with the supplied username/password!\n"
                             "Host = %1\n"
                             "Database = %2\n"
                             "Username = %3\n" )
                .arg(hostName)
                .arg(dbName)
                .arg(username) );
        exit(-1);
      }
      else
        loggedIn = TRUE;
    }

    if (!interactive)
      return 0;
  }

  _splash = new QSplashScreen();
  _splash->setPixmap(QPixmap(":/images/splashEmpty.png"));

  _evaluation = FALSE;

  if (!loggedIn)
  {
    ParameterList params;
//    params.append("name", _Name);  -- We can't tell now until were logged in what the app is.
    params.append("copyright", _Copyright);
    params.append("version", _Version);
    params.append("build", QString("%1 %2").arg(__DATE__).arg(__TIME__));

    if (haveUsername)
      params.append("username", username);

    if (haveDatabaseURL)
      params.append("databaseURL", databaseURL);

    if (_evaluation)
      params.append("evaluation");

    login2 newdlg(0, "", TRUE);
    newdlg.set(params, _splash);

    if (newdlg.exec() == QDialog::Rejected)
      return -1;
    else
    {
      databaseURL = newdlg._databaseURL;
      username = newdlg.username();
    }
  }

  XSqlQuery metric;
  metric.exec("SELECT metric_value"
           "  FROM metric"
           " WHERE (metric_name = 'Application')" );
  if(!metric.first() || (metric.value("metric_value").toString() != "OpenMFG"))
  {
    _splash->setPixmap(QPixmap(":/images/splashPostBooks.png"));
    _Name.prepend("PostBooks");
  }
  else
  {
    _splash->setPixmap(QPixmap(":/images/splashOpenMFG.png"));
    _Name.prepend("OpenMFG");
  }

  metric.exec("SELECT metric_value"
           "  FROM metric"
           " WHERE (metric_name = 'OpenMFGServerVersion')" );
  if(!metric.first() || (metric.value("metric_value").toString() != _dbVersion))
  {
    _splash->hide();
    int result;
    result = QMessageBox::warning( 0, QObject::tr("Version Mismatch"),
      QObject::tr("The version of the database you are connecting to is not the version\n"
                  "this client was designed to work against.\n\n"
                  "This client was designed to work against the database version %1.\n"
                  "If you continue some or all functionality may not work properly\n"
                  "or at all. You may also cause other problems on the database.\n\n"
                  "Do you want to continue anyway?").arg(_dbVersion),
               QMessageBox::Yes,
               QMessageBox::No | QMessageBox::Escape | QMessageBox::Default );
    if(result != QMessageBox::Yes)
      return 0;
    _splash->show();
  }

  _splash->showMessage(QObject::tr("Loading Database Metrics"), SplashTextAlignment, SplashTextColor);
  qApp->processEvents();
  _metrics = new Metrics();

  _splash->showMessage(QObject::tr("Loading User Preferences"), SplashTextAlignment, SplashTextColor);
  qApp->processEvents();
  _preferences = new Preferences(username);

  _splash->showMessage(QObject::tr("Loading User Privileges"), SplashTextAlignment, SplashTextColor);
  qApp->processEvents();
  _privleges = new Privleges();

//  Load the translator defined by the User's locale
  _splash->showMessage(QObject::tr("Loading Translation Dictionary"), SplashTextAlignment, SplashTextColor);
  qApp->processEvents();
  QTranslator translator(0);
  XSqlQuery language( "SELECT locale_lang "
                      "FROM usr, locale "
                      "WHERE ( (usr_username=CURRENT_USER)"
                      " AND (usr_locale_id=locale_id) );" );
  if (language.first())
  {
    QString langName = language.value("locale_lang").toString();
    if (langName.length())
    {
      QStringList paths;
      paths << "dict";
      paths << "";
      paths << "../dict";
      paths << app.applicationDirPath() + "/dict";
      paths << app.applicationDirPath();
      paths << app.applicationDirPath() + "/../dict";
#if defined Q_WS_MACX
      paths << app.applicationDirPath() + "/../../../dict";
      paths << app.applicationDirPath() + "/../../..";
#endif
      bool langFound = false;
      for(QStringList::Iterator it = paths.begin(); it != paths.end(); ++it)
      {
        if (translator.load(langName, *it))
        {
          app.installTranslator(&translator);
          langFound = true;
          break;
        }
      }

      if(!langFound)
        QMessageBox::warning( 0, QObject::tr("Cannot Load Dictionary"),
                              QObject::tr( "The specified Translation Dictionary, \"dict/%1.qm\" cannot be loaded.\n"
                                           "Reverting to the default dictionary instead." )
                                       .arg(langName) );
    }
  }

  qApp->processEvents();
  QString key;

  // TODO: Add code to check a few locations - Hopefully done

  QString keypath;
  QString keyname;
  QString keytogether;
  
  keytogether = app.applicationDirPath() + "/OpenMFG.key";
#ifdef Q_WS_WIN
  keypath = _metrics->value("CCWinEncKey");
#elif defined Q_WS_MACX
  keypath = _metrics->value("CCMacEncKey");
#elif defined Q_WS_X11
  keypath = _metrics->value("CCLinEncKey");
#endif
  
  if (keypath.isEmpty())
  {
  	keypath = app.applicationDirPath() + "/";
  }

  keyname = _metrics->value("CCEncKeyName");
  if (keyname.isEmpty())
  {
  	keyname = "OpenMFG.key";
  }
  
  keytogether = keypath + keyname;
  
  QFile keyFile(keytogether);

  if(keyFile.exists())
  {
    if(keyFile.open(QIODevice::ReadOnly))
    {
      key = keyFile.readLine(1024);
      // strip off any newline characters
      key = key.stripWhiteSpace();
    }
  }

  omfgThis = new OpenMFGGUIClient(databaseURL, username);
  omfgThis->_key = key;

/*
  QString mymess;
  mymess = key.latin1();
  QMessageBox::warning( 0, "Encyrption Key", mymess );
*/
  
  if (key.length() > 0) {
	_splash->showMessage(QObject::tr("Loading Database Encryption Metrics"), SplashTextAlignment, SplashTextColor);
	qApp->processEvents();
	_metricsenc = new Metricsenc(key);
  }
  
  initializePlugin(_preferences, _metrics, _privleges, omfgThis->workspace());

  if (omfgThis->_singleWindow.isEmpty())
  {
    omfgThis->show();
    app.setMainWidget(omfgThis);
  }
  // keep this synchronized with OpenMFGGUIClient and user.ui.h
  else if (omfgThis->_singleWindow == "woTimeClock")
  {
    woTimeClock* newdlg = new woTimeClock();
    ParameterList params;
    params.append("captive");
    newdlg->set(params);
    newdlg->show();
    app.setMainWidget(newdlg);
  }

  if(!omfgThis->singleCurrency())
  {
    // Check for the gain/loss and discrep accounts
    q.prepare("SELECT COALESCE((SELECT TRUE"
              "                   FROM accnt, metric"
              "                  WHERE ((accnt_id=metric_value)"
              "                    AND  (metric_name='CurrencyGainLossAccount'))), FALSE)"
              "   AND COALESCE((SELECT TRUE"
              "                   FROM accnt, metric"
              "                  WHERE ((accnt_id=metric_value)"
              "                    AND  (metric_name='GLSeriesDiscrepancyAccount'))), FALSE) AS result; ");
    q.exec();
    if(q.first() && q.value("result").toBool() != true)
      QMessageBox::warning( omfgThis, QObject::tr("Additional Configuration Required"),
        QObject::tr("Your system is configured to use multiple Currencies, but the\n"
                    "Currency Gain/Loss Account and/or the G/L Series Discrepancy Account\n"
                    "does not appear to be configured correctly. You should define these\n"
                    "Accounts in 'System | Configure Modules | Configure G/L...' before\n"
                    "posting any transactions in the system.") );
  }

  app.exec();

//  Clean up
  delete _metrics;
  delete _preferences;
  delete _privleges;
  if (0 != _metricsenc)
    delete _metricsenc;

#ifdef XQ_WS_WIN
  WSACleanup();
#endif

  return 0;
}

