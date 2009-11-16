/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

/*! \mainpage xTuple ERP Programmer Reference Index Page

  \section intro_sec Introduction

  xTuple ERP is an application written in C++ using the \Qt
  cross-platform application and \user interface framework. This
  document provides reference information for people who want to
  either understand the core application code or develop \scripts
  and \packages to extend the xTuple ERP core.

  To navigate this documentation you should know the following:
  <ul>
    <li>Most xTuple classes are derived from \Qt classes. Much of the
        behavior that is available to you is provided by the \Qt
        classes.
    </li>
    <li>Most classes with names starting with the letter 'Q'
        are \Qt classes while the others are xTuple classes.
        The documentation of \Qt classes comes directly from \Qt and
        so has a different visual style than the xTuple documentation.
    </li>
    <li>Most windows and widgets implemented by xTuple use
	multiple inheritance.  An application window class might
	inherit properties and behavior from QMainWindow (probably
        indirectly through XMainWindow) and its collection of
        widgets from a related class in the Ui namespace.
        Look for the following xTuple conventions:
        <ul>
          <li>Widget names typically begin with an underscore ('_').
              If a widget name does not begin with this character then
              it's probably not important for manipulation (e.g. it's a
              layout or spacer).
          </li>
	  <li>Widget names that end with 'Lit' are usually given
	      to the labels for widgets that do something interesting.
	      For example, if you see a QLabel named '_currencyLit',
	      then there is probably another widget named '_currency'
	      that you might want to manipulate. If you hide a
	      widget then you should probably hide the corresponding
	      label, too.
          </li>
        </ul>
    </li>
    <li>Because of these inheritance relationships, you should use
        the 'List of all members' link frequently. The page documenting
        an xTuple class only shows the slots, signals, methods, and
        properties unique to or overridden by that class. The inherited
        features are only displayed if you look up the inheritance hierarchy
        either directly or through the 'List of all members.'
    </li>
    <li>If you are writing \scripts, you can use any slots, signals,
        and properties displayed on these pages. You can also use
        any methods marked Q_INVOKABLE. Methods which are <i>not</i>
        Q_INVOKABLE are not available for scripting.
    </li>
    <li>The documentation for xTuple classes is sparse at the moment.
	The addition of details is a work in progress.  If you would
	like to help with this effort, please send email to
        <a href="mailto:devdocs@xtuple.com?subject=xTuple%20ERP%20Programmer%20Reference%20contributions">devdocs@xtuple.com</a>.
	You can attach \f\ile diffs with \comment changes to the email
	for us to review and hopefully merge into the core.

        See the
        <a href="http://www.stack.nl/~dimitri/doxygen/commands.html">Special Commands</a>
        chapter in the
        <a href="http://www.stack.nl/~dimitri/doxygen/manual.html">Doxygen Manual</a>
        for information on how to tag the documentation.
    </li>
    <li>The application code is continually changing as we improve
	the product. Classes and object and method names <i>will</i>
	change over time. Each page in this documentation is marked
	with the xTuple ERP version for which it applies. The fact
	that it is documented is not meant to imply that it will
	remain unchanged in future versions.
    </li>
  </ul>
*/

#include <stdlib.h>

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QMessageBox>
#include <QPixmap>
#include <QSplashScreen>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTranslator>
/*
#include <QWindowsStyle>
#include <QCleanlooksStyle>

#ifdef Q_WS_WIN
#include <windows.h>
#include <QStyleFactory>
#endif

#ifdef Q_WS_MACX
#include <QMacStyle>
#endif
*/

#include <dbtools.h>
#include <parameter.h>
#include <widgets.h>

#include "login2.h"

#include "guiclient.h"
#include "version.h"
#include "metrics.h"
#include "metricsenc.h"
#include "scripttoolbox.h"
#include "xmainwindow.h"

#include "sysLocale.h"

#include "splashconst.h"

#include <QtPlugin>
Q_IMPORT_PLUGIN(xTuplePlugin)

QString __password;

#define DEBUG false

int main(int argc, char *argv[])
{
  Q_INIT_RESOURCE(guiclient);

  QString username;
  QString databaseURL;
  QString passwd;
  bool    haveUsername    = FALSE;
  bool    haveDatabaseURL = FALSE;
  bool    loggedIn        = FALSE;
  bool    haveEnhancedAuth= false;
  bool    _enhancedAuth   = false;
  bool    haveRequireSSL  = false;
  bool    _requireSSL     = false;
  bool    havePasswd      = false;

/*
#ifdef XQ_WS_WIN
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(1, 1), &wsaData))
  {
    qDebug("Error starting up Windows Socket system... Database services are not avalable.");
    exit(-1);
  }
#endif
*/

  QApplication app(argc, argv);

#if QT_VERSION >= 0x040400
  // This is the correct place for this call but on versions less
  // than 4.4 it causes a crash for an unknown reason so it is
  // called later on earlier versions.
  QCoreApplication::addLibraryPath(QString("."));
#endif

#ifndef Q_WS_MACX
  QApplication::setWindowIcon(QIcon(":/images/icon32x32.png"));
#endif

/*
#ifdef Q_WS_WIN
  if (QSysInfo::WindowsVersion == QSysInfo::WV_XP)
    QApplication::setStyle(QStyleFactory::create("windowsxpstyle"));
#if QT_VERSION >= 0x040300 
  else if (QSysInfo::WindowsVersion == QSysInfo::WV_VISTA)
    QApplication::setStyle(QStyleFactory::create("windowsvistastyle"));
#endif
  else
    QApplication::setStyle(new QWindowsStyle);
#elif defined Q_WS_MACX
#if QT_VERSION >= 0x040400
  // Again for some reason on earlier version this causes a crash
  // despite this being the correct place for it.
  QApplication::setStyle(new QMacStyle);
#endif
#elif defined Q_WS_X11
  QApplication::setStyle(new QCleanlooksStyle);
#endif

#if QT_VERSION < 0x040400
  // This is here because of some crash problem using it before the
  // QApplication object is instantiated even though it shouldn't be
  // needed. This only appears to affect versions lower than 4.4.
  QCoreApplication::addLibraryPath(QString("."));
#if defined Q_WS_MACX
  // Just as above this has to be done after the QApplication is
  // instantiated to prevent a crash on earlier Qt Versions.
  QApplication::setStyle(new QMacStyle);
#endif
#endif
*/

  // Try and load a default translation file and install it
  QTranslator defaultTranslator(&app);
  if (defaultTranslator.load("default.qm", app.applicationDirPath()))
    app.installTranslator(&defaultTranslator);

  app.processEvents();

  if (argc > 1)
  {
    for (int intCounter = 1; intCounter < argc; intCounter++)
    {
      QString argument(argv[intCounter]);

      if (argument.contains("-databaseURL=", Qt::CaseInsensitive))
      {
        haveDatabaseURL = TRUE;
        databaseURL = argument.right(argument.length() - 13);
      }
      else if (argument.contains("-username=", Qt::CaseInsensitive))
      {
        haveUsername = TRUE;
        username = argument.right(argument.length() - 10);
      }
      else if (argument.contains("-passwd=", Qt::CaseInsensitive))
      {
        havePasswd = TRUE;
        passwd     = argument.right(argument.length() - 8);
      }
      else if (argument.contains("-noAuth", Qt::CaseInsensitive))
      {
        haveUsername = TRUE;
        havePasswd   = TRUE;
      } 
      else if (argument.contains("-enhancedAuth", Qt::CaseInsensitive))
      {
        haveEnhancedAuth = true;
        _enhancedAuth = true;
        if(argument.contains("=no", Qt::CaseInsensitive) || argument.contains("=false", Qt::CaseInsensitive))
          _enhancedAuth = false;
      }
      else if (argument.contains("-requireSSL", Qt::CaseInsensitive))
      {
        haveRequireSSL = true;
        _requireSSL = true;
        if(argument.contains("=no", Qt::CaseInsensitive) || argument.contains("=false", Qt::CaseInsensitive))
          _requireSSL = false;
      }
    }
  }

  _splash = new QSplashScreen();
  _splash->setPixmap(QPixmap(":/images/splashEmpty.png"));

  _evaluation = FALSE;

  if (!loggedIn)
  {
    ParameterList params;
    params.append("copyright", _Copyright);
    params.append("version", _Version);
    params.append("build", QString("%1 %2").arg(__DATE__).arg(__TIME__));

    if (haveUsername)
      params.append("username", username);

    if (havePasswd)
      params.append("password", passwd);

    if (haveDatabaseURL)
      params.append("databaseURL", databaseURL);

    if (haveEnhancedAuth)
      params.append("enhancedAuth", _enhancedAuth);
    
    if (haveRequireSSL)
      params.append("requireSSL", _requireSSL);

    if (_evaluation)
      params.append("evaluation");

    if ( (haveDatabaseURL) && (haveUsername) && (havePasswd) )
      params.append("login");

    login2 newdlg(0, "", TRUE);
    newdlg.set(params, _splash);

    if(newdlg.result() != QDialog::Accepted)
    {
      if (newdlg.exec() == QDialog::Rejected)
        return -1;
      else
      {
        databaseURL = newdlg._databaseURL;
        username = newdlg.username();
        __password = newdlg.password();
      }
    }
  }

  XSqlQuery metric;
  metric.exec("SELECT metric_value"
           "  FROM metric"
           " WHERE (metric_name = 'Application')" );
  if(!metric.first() || (metric.value("metric_value").toString() == "Standard"))
  {
    // check if the xtmfg package is installed
    metric.exec("SELECT pkghead_name FROM pkghead WHERE pkghead_name='xtmfg'");
    if(metric.first())
    {
      _splash->setPixmap(QPixmap(":/images/splashMfgEdition.png"));
      _Name = _Name.arg("Manufacturing");
    }
    else
    {
      _splash->setPixmap(QPixmap(":/images/splashStdEdition.png"));
      _Name = _Name.arg("Standard");
    }
  }
  else
  {
    _splash->setPixmap(QPixmap(":/images/splashPostBooks.png"));
    _Name = _Name.arg("PostBooks");
  }

  metric.exec("SELECT metric_value"
           "  FROM metric"
           " WHERE (metric_name = 'ServerVersion')" );
  if(!metric.first() || (metric.value("metric_value").toString() != _dbVersion))
  {
    bool disallowMismatch = false;
    metric.exec("SELECT metric_value FROM metric WHERE(metric_name='DisallowMismatchClientVersion')");
    if(metric.first() && (metric.value("metric_value").toString() == "t"))
      disallowMismatch = true;
    
    _splash->hide();
    int result;
    if(disallowMismatch)
      result = QMessageBox::warning( 0, QObject::tr("Version Mismatch"),
        QObject::tr("<p>The version of the database you are connecting to is "
                    "not the version this client was designed to work against. "
                    "This client was designed to work against the database "
                    "version %1. The system has been configured to disallow "
                    "access in this case.<p>Please contact your systems "
                    "administrator.").arg(_dbVersion),
                 QMessageBox::Ok | QMessageBox::Escape | QMessageBox::Default );
    else
      result = QMessageBox::warning( 0, QObject::tr("Version Mismatch"),
        QObject::tr("<p>The version of the database you are connecting to is "
                    "not the version this client was designed to work against. "
                    "This client was designed to work against the database "
                    "version %1. If you continue some or all functionality may "
                    "not work properly or at all. You may also cause other "
                    "problems on the database.<p>Do you want to continue "
                    "anyway?").arg(_dbVersion),
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
  _privileges = new Privileges();

  // Load the translator and set the locale from the User's preferences
  _splash->showMessage(QObject::tr("Loading Translation Dictionary"), SplashTextAlignment, SplashTextColor);
  qApp->processEvents();
  XSqlQuery langq("SELECT * "
                  "FROM usr, locale LEFT OUTER JOIN"
                  "     lang ON (locale_lang_id=lang_id) LEFT OUTER JOIN"
                  "     country ON (locale_country_id=country_id) "
                  "WHERE ( (usr_username=CURRENT_USER)"
                  " AND (usr_locale_id=locale_id) );" );
  if (langq.first())
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
    
    QStringList files;
    if (!langq.value("locale_lang_file").toString().isEmpty())
      files << langq.value("locale_lang_file").toString();

    QString langext;
    if (!langq.value("lang_abbr2").toString().isEmpty() && 
        !langq.value("country_abbr").toString().isEmpty())
    {
      langext = langq.value("lang_abbr2").toString() + "_" +
                langq.value("country_abbr").toString().toLower();
    }
    else if (!langq.value("lang_abbr2").toString().isEmpty())
    {
      langext = langq.value("lang_abbr2").toString();
    }

    if(!langext.isEmpty())
    {
      files << "xTuple." + langext;
      files << "openrpt." + langext;

      XSqlQuery pkglist("SELECT pkghead_name FROM pkghead WHERE packageIsEnabled(pkghead_name);");
      while(pkglist.next())
      {
        files << pkglist.value("pkghead_name").toString() + "." + langext;
      }
    }

    if (files.size() > 0)
    {
      bool langFound = false;

      QTranslator *translator = new QTranslator(&app);
      for (QStringList::Iterator fit = files.begin(); fit != files.end(); ++fit)
      {
        for(QStringList::Iterator pit = paths.begin(); pit != paths.end(); ++pit)
        {
          if (DEBUG)
            qDebug("looking for %s in %s",
                   (*fit).toAscii().data(), (*pit).toAscii().data());
          if (translator->load(*fit, *pit))
          {
            app.installTranslator(translator);
            langFound = true;
            qDebug("installed %s/%s",
                   (*pit).toAscii().data(), (*fit).toAscii().data());
            translator = new QTranslator(&app);
            break;
          }
        }
      }

      if (!langFound && !_preferences->boolean("IngoreMissingTranslationFiles"))
        QMessageBox::warning( 0, QObject::tr("Cannot Load Dictionary"),
                              QObject::tr("<p>The Translation Dictionaries %1 "
                                          "cannot be loaded. Reverting "
                                          "to the default dictionary." )
                                       .arg(files.join(QObject::tr(", "))));
    }

    /* set the locale to langabbr_countryabbr, langabbr, {lang# country#}, or
       lang#, depending on what information is available
     */
    QString langAbbr = langq.value("lang_abbr2").toString();
    QString cntryAbbr = langq.value("country_abbr").toString().toUpper();
    if(cntryAbbr == "UK")
      cntryAbbr = "GB";
    if (! langAbbr.isEmpty() &&
        ! cntryAbbr.isEmpty())
      QLocale::setDefault(QLocale(langAbbr + "_" + cntryAbbr));
    else if (! langAbbr.isEmpty())
      QLocale::setDefault(QLocale(langq.value("lang_abbr2").toString()));
    else if (langq.value("lang_qt_number").toInt() &&
             langq.value("country_qt_number").toInt())
      QLocale::setDefault(
          QLocale(QLocale::Language(langq.value("lang_qt_number").toInt()),
                  QLocale::Country(langq.value("country_qt_number").toInt())));
    else
      QLocale::setDefault(QLocale::system());

    qDebug("Locale set to language %s and country %s",
           QLocale().languageToString(QLocale().language()).toAscii().data(),
           QLocale().countryToString(QLocale().country()).toAscii().data());

  }
  else if (langq.lastError().type() != QSqlError::NoError)
  {
    systemError(0, langq.lastError().databaseText(), __FILE__, __LINE__);
  }

  qApp->processEvents();
  QString key;

  // TODO: Add code to check a few locations - Hopefully done

  QString keypath;
  QString keyname;
  QString keytogether;
  
#ifdef Q_WS_WIN
  keypath = _metrics->value("CCWinEncKey");
#elif defined Q_WS_MACX
  keypath = _metrics->value("CCMacEncKey");
#elif defined Q_WS_X11
  keypath = _metrics->value("CCLinEncKey");
#endif
  
  if (keypath.isEmpty())
    keypath = app.applicationDirPath();

  if (! keypath.endsWith(QDir::separator()))
    keypath += QDir::separator();

  keyname = _metrics->value("CCEncKeyName");
  if (keyname.isEmpty())
  {
    keyname = "xTuple.key";
    keytogether = keypath + keyname;
    QFile kn(keytogether);
    if(!kn.exists())
      keyname = "OpenMFG.key";
  }
  
  keytogether = keypath + keyname;
  
  // qDebug("keytogether: %s", keytogether.toAscii().data());
  QFile keyFile(keytogether);

  if(keyFile.exists())
  {
    if(keyFile.open(QIODevice::ReadOnly))
    {
      key = keyFile.readLine(1024);
      // strip off any newline characters
      key = key.trimmed();
    }
  }

  omfgThis = 0;
  omfgThis = new GUIClient(databaseURL, username);
  omfgThis->_key = key;

// qDebug("Encryption Key: %s", key.toAscii().data() );
  
  if (key.length() > 0) {
	_splash->showMessage(QObject::tr("Loading Database Encryption Metrics"), SplashTextAlignment, SplashTextColor);
	qApp->processEvents();
	_metricsenc = new Metricsenc(key);
  }
  
  initializePlugin(_preferences, _metrics, _privileges, omfgThis->workspace());

// START code for updating the locale settings if they haven't been already
  XSqlQuery lc;
  lc.exec("SELECT count(*) FROM metric WHERE metric_name='AutoUpdateLocaleHasRun';");
  lc.first();
  if(lc.value(0).toInt() == 0)
  {
    lc.exec("INSERT INTO metric (metric_name, metric_value) values('AutoUpdateLocaleHasRun', 't');");
    lc.exec("SELECT locale_id from locale;");
    while(lc.next())
    {
      ParameterList params;
      params.append("mode","edit");
      params.append("locale_id", lc.value(0));
      sysLocale lcdlg;
      lcdlg.set(params);
      lcdlg.sSave();
    }
  }
// END code for updating locale settings

  QObject::connect(&app, SIGNAL(aboutToQuit()), &app, SLOT(closeAllWindows()));
  if (omfgThis->_singleWindow.isEmpty())
  {
    omfgThis->setAttribute(Qt::WA_DeleteOnClose);
    omfgThis->show();
  }
  // keep this synchronized with GUIClient and user.ui.h
  else if (omfgThis->_singleWindow == "woTimeClock")
  {
    ScriptToolbox sb(0);
    QWidget* newdlg = sb.openWindow("woTimeClock");
    if(newdlg)
    {
      XMainWindow *mw = qobject_cast<XMainWindow*>(newdlg);
      if(mw)
      {
        ParameterList params;
        params.append("captive");
        mw->set(params);
      }
      newdlg->setAttribute(Qt::WA_DeleteOnClose);
      QObject::connect(omfgThis, SIGNAL(destroyed(QObject*)), &app, SLOT(quit()));
      newdlg->show();
    }
    else
    {
      qDebug("Failed to initialize woTimeClock window.");
      return -2;
    }
  }

  if(!omfgThis->singleCurrency())
  {
    // Check for the gain/loss and discrep accounts
    q.prepare("SELECT COALESCE((SELECT TRUE"
              "                   FROM accnt, metric"
              "                  WHERE ((CAST(accnt_id AS text)=metric_value)"
              "                    AND  (metric_name='CurrencyGainLossAccount'))), FALSE)"
              "   AND COALESCE((SELECT TRUE"
              "                   FROM accnt, metric"
              "                  WHERE ((CAST(accnt_id AS text)=metric_value)"
              "                    AND  (metric_name='GLSeriesDiscrepancyAccount'))), FALSE) AS result; ");
    q.exec();
    if(q.first() && q.value("result").toBool() != true)
      QMessageBox::warning( omfgThis, QObject::tr("Additional Configuration Required"),
        QObject::tr("<p>Your system is configured to use multiple Currencies, "
                    "but the Currency Gain/Loss Account and/or the G/L Series "
                    "Discrepancy Account does not appear to be configured "
                    "correctly. You should define these Accounts in 'System | "
                    "Configure Modules | Configure G/L...' before posting any "
                    "transactions in the system.") );
  }

  app.exec();

//  Clean up
  delete _metrics;
  delete _preferences;
  delete _privileges;
  if (0 != _metricsenc)
    delete _metricsenc;

/*
#ifdef XQ_WS_WIN
  WSACleanup();
#endif
*/

  return 0;
}
