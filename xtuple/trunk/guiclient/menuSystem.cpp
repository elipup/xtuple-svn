/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include <QAction>
#include <QApplication>
#include <QAssistantClient>
#include <QDir>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QPixmap>
#include <QWorkspace>

#include "xtsettings.h"
#include "guiclient.h"

#include <parameter.h>
#include <openreports.h>
#include <../common/batchManager.h>

#include "version.h"

#include "menuSystem.h"

#include "systemMessage.h"
#include "eventManager.h"
#include "users.h"
#include "submitAction.h"
#include "userPreferences.h"
#include "hotkeys.h"
#include "errorLog.h"

#include "databaseInformation.h"

#include "accountNumbers.h"
#include "calendars.h"
#include "commentTypes.h"
#include "countries.h"
#include "currencies.h"
#include "currencyConversions.h"
#include "customCommands.h"
#include "departments.h"
#include "employee.h"
#include "employees.h"
#include "empGroup.h"
#include "empGroups.h"
#include "searchForEmp.h"
#include "forms.h"
#include "groups.h"
#include "images.h"
#include "labelForms.h"
#include "locales.h"
#include "package.h"
#include "packages.h"
#include "reports.h"
#include "scripts.h"
#include "uiforms.h"

#include "fixACL.h"
#include "fixSerial.h"
#include "importXML.h"

#include "configureIE.h"
#include "configureIM.h"
#include "configurePD.h"
#include "configureMS.h"
#include "configureWO.h"
#include "configureSO.h"
#include "configurePO.h"
#include "configureGL.h"
#include "configureEncryption.h"
#include "configureCC.h"
#include "configureCRM.h"

#include "registration.h"

extern QString __path;

menuSystem::menuSystem(GUIClient *Pparent) :
 QObject(Pparent)
{
  setObjectName("sysModule");
  parent = Pparent;

  errorLogListener::initialize();

  cascade = tile = closeActive = closeAll = _rememberPos = _rememberSize = 0;
  _lastActive = 0;
  geometryMenu = 0;

  systemMenu		= new QMenu(parent);
  configModulesMenu	= new QMenu(parent);
  masterInfoMenu	= new QMenu(parent);
  sysUtilsMenu		= new QMenu(parent);
  windowMenu		= new QMenu(parent);
  helpMenu		= new QMenu(parent);
  designMenu            = new QMenu(parent);
  employeeMenu          = new QMenu(parent);

  systemMenu->setObjectName("menu.sys");
  configModulesMenu->setObjectName("menu.sys.configmodules");
  masterInfoMenu->setObjectName("menu.sys.masterinfo");
  sysUtilsMenu->setObjectName("menu.sys.utilities");
  windowMenu->setObjectName("menu.window");
  helpMenu->setObjectName("menu.help");
  designMenu->setObjectName("menu.sys.design");

//  Window
  windowMenu->setCheckable(TRUE);

  cascade = new Action( parent, "window.cascade", tr("&Cascade"), parent->workspace(), SLOT(cascade()), windowMenu, true);
  parent->actions.append( cascade );

  tile = new Action( parent, "window.tile", tr("&Tile"), parent->workspace(), SLOT(tile()), windowMenu, true);
  parent->actions.append( tile );

  closeActive = new Action( parent, "window.closeActiveWindow", tr("Close &Active Window"), this, SLOT(sCloseActive()), windowMenu, true);
  parent->actions.append( closeActive );

  closeAll = new Action( parent, "window.closeAllWindows", tr("Close A&ll Windows"), this, SLOT(sCloseAll()), windowMenu, true);
  parent->actions.append( closeAll );

  _rememberPos = new Action( parent, "window.rememberPositionToggle", tr("Remember Position"), this, SLOT(sRememberPositionToggle()), windowMenu, true);
  _rememberPos->setCheckable(true);

  _rememberSize = new Action( parent, "window.rememberSizeToggle", tr("Remember Size"), this, SLOT(sRememberSizeToggle()), windowMenu, true);
  _rememberSize->setCheckable(true);

  QAction * m = parent->menuBar()->addMenu(windowMenu );
  if(m)
    m->setText(tr("&Window"));
  connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(sPrepareWindowMenu()));
  connect(windowMenu, SIGNAL(aboutToHide()), this, SLOT(sHideWindowMenu()));
  

  actionProperties acts[] = {

    { "sys.scheduleSystemMessage",    tr("Schedule S&ystem Message..."),    SLOT(sScheduleSystemMessage()),    systemMenu, "IssueSystemMessages", NULL, NULL, _metrics->boolean("EnableBatchManager") },
    { "separator",                    NULL,                                 NULL,                              systemMenu, "true",                                      NULL, NULL, _metrics->boolean("EnableBatchManager") },
    { "sys.eventManager",             tr("E&vent Manager..."),              SLOT(sEventManager()),             systemMenu, "true",                                      NULL, NULL, true },
    { "sys.batchManager",             tr("&Batch Manager..."),              SLOT(sBatchManager()),             systemMenu, "true",                                      NULL, NULL, _metrics->boolean("EnableBatchManager") },
    { "sys.viewDatabaseLog",          tr("View Database &Log..."),          SLOT(sErrorLog()),                 systemMenu, "true",                                      NULL, NULL, true },
    { "separator",                    NULL,                                 NULL,                              systemMenu, "true",                                      NULL, NULL, true },
    { "sys.preferences",              tr("P&references..."),                SLOT(sPreferences()),              systemMenu, "MaintainPreferencesSelf MaintainPreferencesOthers",  NULL,   NULL,   true },
    { "sys.hotkeys",                  tr("&Hot Keys..."),                   SLOT(sHotKeys()),                  systemMenu, "true",  NULL,   NULL,   !(_privileges->check("MaintainPreferencesSelf") || _privileges->check("MaintainPreferencesOthers")) },
    { "sys.rescanPrivileges",         tr("Rescan &Privileges"),             SLOT(sRescanPrivileges()),         systemMenu, "true",                                      NULL, NULL, true },
    { "separator",                    NULL,                                 NULL,                              systemMenu, "true",                                      NULL, NULL, true },
    { "sys.maintainUsers",            tr("Maintain &Users..."),             SLOT(sMaintainUsers()),            systemMenu, "MaintainUsers",       NULL, NULL, true },
    { "sys.maintainGroups",           tr("Maintain &Groups..."),            SLOT(sMaintainGroups()),           systemMenu, "MaintainGroups",      NULL, NULL, true },

    { "menu",                         tr("&Employees"),                     (char*)employeeMenu,               systemMenu, "true",                                      NULL, NULL, true },
    { "sys.employee",                 tr("&New..."),               	    SLOT(sNewEmployee()),            employeeMenu, "MaintainEmployees",               NULL, NULL, true },
    { "sys.listEmployees",            tr("&List..."),             	    SLOT(sListEmployees()),          employeeMenu, "ViewEmployees MaintainEmployees",                   NULL, NULL, true },
    { "sys.searchEmployees",          tr("&Search..."),       		    SLOT(sSearchEmployees()),        employeeMenu, "ViewEmployees MaintainEmployees",               NULL, NULL, true },
    { "separator",                    NULL,                                 NULL,                            employeeMenu, "true",                                      NULL, NULL, true },
    { "sys.employeeGroups",           tr("Employee &Groups..."),            SLOT(sEmployeeGroups()),         employeeMenu, "ViewEmployeeGroups MaintainEmployeeGroups",                   NULL, NULL, true },

    { "separator",                    NULL,                                 NULL,                              systemMenu, "true",                                      NULL, NULL, true },
    { "sys.scheduleServerMaintenance",tr("Schedule Server Mai&ntenance..."),SLOT(sScheduleServerMaintenance()),systemMenu, "MaintainServer",      NULL, NULL, _metrics->boolean("EnableBatchManager") },
    { "separator",                    NULL,                                 NULL,                              systemMenu, "true",                                      NULL, NULL, _metrics->boolean("EnableBatchManager") },

  //  System | Configure Modules
    { "menu",			tr("&Configure Modules"),(char*)configModulesMenu,systemMenu,		"true",					NULL,	NULL,	true	},
    { "sys.configurePD",	tr("&Products..."),	SLOT(sConfigurePD()),	configModulesMenu,	"ConfigurePD",	NULL,	NULL,	true	},
    { "sys.configureIM",	tr("&Inventory..."),	SLOT(sConfigureIM()),	configModulesMenu,	"ConfigureIM",	NULL,	NULL,	true	},
    { "sys.configurePO",	tr("P&urchase..."),	SLOT(sConfigurePO()),	configModulesMenu,	"ConfigurePO",	NULL,	NULL,	true	},
    { "sys.configureMS",	tr("Sch&edule..."),	SLOT(sConfigureMS()),	configModulesMenu,	"ConfigureMS",	NULL,	NULL,	 (_metrics->value("Application") != "PostBooks")	},
    { "sys.configureWO",	tr("&Manufacture..."),	SLOT(sConfigureWO()),	configModulesMenu,	"ConfigureWO",	NULL,	NULL,	true	},
    { "sys.configureCRM",	tr("&CRM..."),		SLOT(sConfigureCRM()),	configModulesMenu,	"ConfigureCRM",	NULL,	NULL,	true	},
    { "sys.configureSO",	tr("&Sales..."),	SLOT(sConfigureSO()),	configModulesMenu,	"ConfigureSO",	NULL,	NULL,	true	},
    { "sys.configureGL",	tr("&Accounting..."),	SLOT(sConfigureGL()),	configModulesMenu,	"ConfigureGL",	NULL,	NULL,	true	},

  //  Master Information
    { "menu",				tr("&Master Information"),	(char*)masterInfoMenu,		systemMenu,	"true",						NULL,	NULL,	true	},
    { "sys.databaseInformation",	tr("&Database Information..."),	SLOT(sDatabaseInformation()),	masterInfoMenu,	"ConfigDatabaseInfo",	NULL,	NULL,	true	},
    { "separator",		NULL,			NULL,			masterInfoMenu,	"true",					NULL,	NULL,	true	},
    { "sys.images",		tr("&Images..."),	SLOT(sImages()),	masterInfoMenu,	"MaintainImages",	NULL,	NULL,	true	},
    { "sys.forms",		tr("&Forms..."),	SLOT(sForms()),		masterInfoMenu,	"MaintainForms",	NULL,	NULL,	true	},
    { "sys.labelForms",		tr("&Label Forms..."),	SLOT(sLabelForms()),	masterInfoMenu,	"MaintainForms",	NULL,	NULL,	true	},
    { "sys.calendars",		tr("C&alendars..."),	SLOT(sCalendars()),	masterInfoMenu,	"MaintainCalendars",	NULL,	NULL,	true	},
    { "sys.currencies",		tr("Curre&ncies..."),	SLOT(sCurrencies()),	masterInfoMenu,	"CreateNewCurrency",	NULL,	NULL,	true	},
    { "sys.exchangeRates",	tr("&Exchange Rates..."),SLOT(sExchangeRates()),masterInfoMenu,	"MaintainCurrencyRates ViewCurrencyRates",
																	NULL,	NULL,	true	},
    { "sys.encryption",         tr("Encr&yption..."),   SLOT(sConfigureEncryption()), masterInfoMenu, "ConfigureCC ConfigureEncryption",
                                                                                                                                        NULL,	NULL,	true	},
    { "sys.configureCC",	tr("&Credit Cards..."),	SLOT(sConfigureCC()),	masterInfoMenu,	"ConfigureCC",	NULL,	NULL,	true	},
    { "sys.countries",		tr("Co&untries..."),	SLOT(sCountries()),	masterInfoMenu,	"MaintainCountries",	NULL,	NULL,	true	},
    { "sys.locales",		tr("L&ocales..."),	SLOT(sLocales()),	masterInfoMenu,	"MaintainLocales",	NULL,	NULL,	true	},
    { "sys.commentTypes",	tr("Comment &Types..."),SLOT(sCommentTypes()),	masterInfoMenu,	"MaintainCommentTypes", NULL, NULL,	true	},
    { "sys.departments",	tr("Depart&ments..."),	SLOT(sDepartments()),	masterInfoMenu,	"ViewDepartments MaintainDepartments",	NULL,	NULL,	true	},
    { "sys.configureIE",	tr("Configure Data Import and E&xport..."),	SLOT(sConfigureIE()),	 masterInfoMenu, "ConfigureImportExport",	NULL, NULL, true },

  //  Design
    { "menu",		   tr("&Design"),	     (char*)designMenu,	     systemMenu, "true",				        NULL, NULL, true },
    { "sys.reports",	   tr("&Reports..."),	     SLOT(sReports()),       designMenu, "MaintainReports",	NULL, NULL, true },
    { "separator",	   NULL,		     NULL,		     designMenu, "true",					NULL, NULL, true },
    { "sys.uiforms",       tr("Screens..."),         SLOT(sUIForms()),       designMenu, "MaintainScreens",	NULL, NULL, true },
    { "sys.scripts",       tr("Scripts..."),	     SLOT(sScripts()),       designMenu, "MaintainScripts",	NULL, NULL, true },
    { "separator",	   NULL,		     NULL,		     designMenu, "true",					NULL, NULL, true },
    { "sys.customCommands",tr("Custom Command&s..."),SLOT(sCustomCommands()),designMenu, "MaintainCustomCommands", NULL, NULL, true },
    { "separator",	   NULL,		     NULL,		     designMenu, "true",					NULL, NULL, true },
    { "sys.packages",      tr("&Packages..."),	     SLOT(sPackages()),      designMenu, "ViewPackages #superuser",           NULL, NULL, true },

  // Utilities
    { "menu",              tr("&System Utilities"),(char*)sysUtilsMenu, systemMenu,    "true",                            NULL, NULL, true },
    { "sys.fixACL",        tr("&Access Control"),  SLOT(sFixACL()),     sysUtilsMenu,  "fixACL",           NULL, NULL, true },
    { "sys.fixSerial",     tr("&Serial Columns"),  SLOT(sFixSerial()),  sysUtilsMenu,  "FixSerial", NULL, NULL, true },
    { "sys.importXML",     tr("&Import XML"),      SLOT(sImportXML()),  sysUtilsMenu,  "ImportXML",        NULL, NULL, true },

    { "separator",		NULL,				NULL,				systemMenu,	"true",	NULL,	NULL,	true	},
    { "sys.printAlignmentPage",	tr("Print &Alignment Page..."),	SLOT(sPrintAlignment()),	systemMenu,	"true",	NULL,	NULL,	true	},
    { "separator",		NULL,				NULL,				systemMenu,	"true",	NULL,	NULL,	true	},
    { "sys.exit",	tr("E&xit xTuple ERP..."), SLOT(sExit()),				systemMenu,	"true",	NULL,	NULL,	true	},

  };

  addActionsToMenu(acts, sizeof(acts) / sizeof(acts[0]));
  parent->populateCustomMenu(systemMenu, "System");
  m = parent->menuBar()->addMenu(systemMenu);
  if(m)
    m->setText(tr("S&ystem"));

  // Community
  communityMenu = new QMenu();
  actionProperties community[] = {
    { "community.home",        tr("xTuple.org &Home"),             SLOT(sCommunityHome()),        communityMenu, "true", NULL, NULL, true },
    { "separator",	       NULL,				   NULL,		          communityMenu, "true", NULL, NULL, true	},
    { "community.register",    tr("&Register"),      SLOT(sRegister()),                           communityMenu, "true", QPixmap(":images/dspRegister.png"), NULL, _metrics->value("Application") == "PostBooks" },
    { "community.editAccount", tr("My Online User &Account"),      SLOT(sCommunityEditAccount()), communityMenu, "true", NULL, NULL, true },
    { "community.support",     tr("Online Customer &Support"),     SLOT(sCommunitySupport()),     communityMenu, "true", NULL, NULL, true },
    { "community.wiki",        tr("Online Documentation / &Wiki"), SLOT(sCommunityWiki()),        communityMenu, "true", NULL, NULL, true },
    { "separator",	       NULL,				   NULL,		          communityMenu, "true", NULL, NULL, true	},
    { "community.forums",      tr("Discussion &Forums"),           SLOT(sCommunityForums()),      communityMenu, "true", NULL, NULL, true },
    { "community.issues",      tr("&Bugs and Feature Requests"),   SLOT(sCommunityIssues()),      communityMenu, "true", NULL, NULL, true },
    { "community.downloads",   tr("&Downloads"),                   SLOT(sCommunityDownloads()),   communityMenu, "true", NULL, NULL, true },
    { "community.blogs",       tr("Bl&ogs"),                       SLOT(sCommunityBlogs()),       communityMenu, "true", NULL, NULL, true },
    { "community.translation", tr("&Translation Portal"),          SLOT(sCommunityTranslation()), communityMenu, "true", NULL, NULL, true },
  };
  addActionsToMenu(community, sizeof(community) / sizeof(community[0]));
  m = parent->menuBar()->addMenu(communityMenu);
  if(m)
    m->setText(tr("C&ommunity"));

  //  Help
  helpMenu = new QMenu();
  actionProperties help[] = {
    { "help.about",		tr("&About..."),		SLOT(sAbout()),	helpMenu, "true", NULL, NULL, true	},
#ifndef Q_WS_MACX
    { "separator",		NULL,				NULL,		helpMenu, "true", NULL, NULL, true	},
#endif
    { "help.tableOfContents",	tr("Table of &Contents..."),	SLOT(sTOC()),	helpMenu, "true", NULL, NULL, true	}
  };
  addActionsToMenu(help, sizeof(help) / sizeof(help[0]));

  m = parent->menuBar()->addMenu(helpMenu);
  if(m)
    m->setText(tr("&Help"));
}

void menuSystem::addActionsToMenu(actionProperties acts[], unsigned int numElems)
{
  QAction * m = 0;
  for (unsigned int i = 0; i < numElems; i++)
  {
    if (! acts[i].visible)
    {
      continue;
    }
    else if (acts[i].actionName == QString("menu"))
    {
      m = acts[i].menu->addMenu((QMenu*)(acts[i].slot));
      if(m)
        m->setText(acts[i].actionTitle);
    }
    else if (acts[i].actionName == QString("separator"))
    {
      acts[i].menu->addSeparator();
    }
    else if (acts[i].toolBar != NULL)
    {
      parent->actions.append( new Action( parent,
					  acts[i].actionName,
					  acts[i].actionTitle,
					  this,
					  acts[i].slot,
					  acts[i].menu,
					  acts[i].priv,
					  (acts[i].pixmap),
					  acts[i].toolBar) );
    }
    else
    {
      parent->actions.append( new Action( parent,
					  acts[i].actionName,
					  acts[i].actionTitle,
					  this,
					  acts[i].slot,
					  acts[i].menu,
					  acts[i].priv ) );
    }
  }
}

void menuSystem::sPrepareWindowMenu()
{
  windowMenu->clear();

  if(!omfgThis->showTopLevel())
  {
    cascade->addTo(windowMenu);
    tile->addTo(windowMenu);
  }
  closeActive->addTo(windowMenu);
  closeAll->addTo(windowMenu);

  QWidgetList windows = omfgThis->windowList();

  bool b = !windows.isEmpty();
  cascade->setEnabled(b);
  tile->setEnabled(b);
  closeActive->setEnabled(b);
  closeAll->setEnabled(b);

  windowMenu->insertSeparator();

  QWidget * activeWindow = parent->workspace()->activeWindow();
  if(omfgThis->showTopLevel())
  {
    //activeWindow = qApp->activeWindow();
    activeWindow = omfgThis->myActiveWindow();
  }
  _lastActive = activeWindow;

  if(activeWindow)
  {
    if(!geometryMenu)
      geometryMenu = new QMenu();

    geometryMenu->clear();
    geometryMenu->setTitle(activeWindow->caption());

    QString objName = activeWindow->objectName();
    
    _rememberPos->setChecked(xtsettingsValue(objName + "/geometry/rememberPos", true).toBool());
    _rememberPos->addTo(geometryMenu);
    _rememberSize->setChecked(xtsettingsValue(objName + "/geometry/rememberSize", true).toBool());
    _rememberSize->addTo(geometryMenu);

    windowMenu->addMenu(geometryMenu);
    windowMenu->insertSeparator();
  }

  QAction * m = 0;
  for (int cursor = 0; cursor < windows.count(); cursor++)
  {
    m = windowMenu->addAction(windows.at(cursor)->caption(), this, SLOT(sActivateWindow()));
    if(m)
    {
      m->setData(cursor);
      m->setCheckable(true);
      m->setChecked((activeWindow == windows.at(cursor)));
    }
  }
}

void menuSystem::sHideWindowMenu()
{
  cascade->setEnabled(true);
  tile->setEnabled(true);
  closeActive->setEnabled(true);
  closeAll->setEnabled(true);
}

void menuSystem::sActivateWindow()
{
  int intWindowid = -1;
  QAction * m = qobject_cast<QAction*>(sender());
  if(m)
    intWindowid = m->data().toInt();
  QWidgetList windows = parent->workspace()->windowList();
  if(omfgThis->showTopLevel())
    windows = omfgThis->windowList();
  QWidget *window = windows.at(intWindowid);
  if (window)
  {
    if(omfgThis->showTopLevel())
      window->activateWindow();
    else
      window->setFocus();
  }
}

void menuSystem::sRememberPositionToggle()
{
  if(!_lastActive)
    return;

  QString objName = _lastActive->objectName();
  xtsettingsSetValue(objName + "/geometry/rememberPos", _rememberPos->isChecked());
}

void menuSystem::sRememberSizeToggle()
{
  if(!_lastActive)
    return;

  QString objName = _lastActive->objectName();
  xtsettingsSetValue(objName + "/geometry/rememberSize", _rememberSize->isChecked());
}

void menuSystem::sCloseAll()
{
  if(omfgThis->showTopLevel())
  {
    foreach(QWidget * w, omfgThis->windowList())
    {
      w->close();
    }
  }
  else
    parent->workspace()->closeAllWindows();
}

void menuSystem::sCloseActive()
{
  if(omfgThis->showTopLevel())
  {
    if(omfgThis->windowList().contains(qApp->activeWindow()))
      qApp->activeWindow()->close();
  }
  else
    parent->workspace()->closeActiveWindow();
}

void menuSystem::sScheduleSystemMessage()
{
  ParameterList params;
  params.append("mode", "new");

  systemMessage newdlg(parent, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void menuSystem::sEventManager()
{
  omfgThis->handleNewWindow(new eventManager());
}

void menuSystem::sBatchManager()
{
  batchManager *newdlg = new batchManager();
  newdlg->setViewOtherEvents(_privileges->check("ViewOtherEvents"));
  omfgThis->handleNewWindow(newdlg);
}

void menuSystem::sPreferences()
{
  userPreferences(parent, "", TRUE).exec();
}

void menuSystem::sHotKeys()
{
  ParameterList params;

  params.append("currentUser");

  hotkeys newdlg(omfgThis, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec() == QDialog::Accepted)
    sRescanPrivileges();
}

void menuSystem::sRescanPrivileges()
{
  _privileges->load();
  omfgThis->saveToolbarPositions();
  _preferences->load();
  omfgThis->initMenuBar();
}

void menuSystem::sDatabaseInformation()
{
  databaseInformation(parent, "", TRUE).exec();
}

void menuSystem::sImages()
{
  omfgThis->handleNewWindow(new images());
}

void menuSystem::sReports()
{
  omfgThis->handleNewWindow(new reports());
}

void menuSystem::sForms()
{
  omfgThis->handleNewWindow(new forms());
}

void menuSystem::sLabelForms()
{
  omfgThis->handleNewWindow(new labelForms());
}

void menuSystem::sCalendars()
{
  omfgThis->handleNewWindow(new calendars());
}

void menuSystem::sCurrencies()
{
  omfgThis->handleNewWindow(new currencies());
}

void menuSystem::sExchangeRates()
{
  omfgThis->handleNewWindow(new currencyConversions());
}

void menuSystem::sCountries()
{
  omfgThis->handleNewWindow(new countries());
}

void menuSystem::sLocales()
{
  omfgThis->handleNewWindow(new locales());
}

void menuSystem::sCommentTypes()
{
  omfgThis->handleNewWindow(new commentTypes());
}

void menuSystem::sAccountNumbers()
{
  omfgThis->handleNewWindow(new accountNumbers());
}

void menuSystem::sDepartments()
{
  omfgThis->handleNewWindow(new departments());
}

void menuSystem::sConfigureIE()
{
  configureIE(parent, "", TRUE).exec();
}

void menuSystem::sCustomCommands()
{
  omfgThis->handleNewWindow(new customCommands());
}

void menuSystem::sPackages()
{
  omfgThis->handleNewWindow(new packages());
}

void menuSystem::sScripts()
{
  omfgThis->handleNewWindow(new scripts());
}

void menuSystem::sUIForms()
{
  omfgThis->handleNewWindow(new uiforms());
}

void menuSystem::sConfigureIM()
{
  configureIM(parent, "", TRUE).exec();
}

void menuSystem::sConfigurePD()
{
  configurePD(parent, "", TRUE).exec();
}

void menuSystem::sConfigureMS()
{
  configureMS(parent, "", TRUE).exec();
}

void menuSystem::sConfigureWO()
{
  configureWO(parent, "", TRUE).exec();
}

void menuSystem::sConfigurePO()
{
  configurePO(parent, "", TRUE).exec();
}

void menuSystem::sConfigureSO()
{
  configureSO(parent, "", TRUE).exec();
}

void menuSystem::sConfigureGL()
{
  configureGL(parent, "", TRUE).exec();
}

void menuSystem::sConfigureEncryption()
{
  configureEncryption(parent, "", TRUE).exec();
}

void menuSystem::sConfigureCC()
{
  configureCC(parent, "", TRUE).exec();
}

void menuSystem::sConfigureCRM()
{
  configureCRM(parent, "", TRUE).exec();
}

void menuSystem::sMaintainUsers()
{
  omfgThis->handleNewWindow(new users());
}

void menuSystem::sMaintainGroups()
{
  omfgThis->handleNewWindow(new groups());
}

void menuSystem::sNewEmployee()
{
  ParameterList params;
  employee newdlg(parent);
  params.append("mode", "new");
  newdlg.set(params);
  newdlg.exec();
}

void menuSystem::sListEmployees()
{
  omfgThis->handleNewWindow(new employees());
}

void menuSystem::sSearchEmployees()
{
  omfgThis->handleNewWindow(new searchForEmp());
}

void menuSystem::sEmployeeGroups()
{
  omfgThis->handleNewWindow(new empGroups());
}

void menuSystem::sScheduleServerMaintenance()
{
  ParameterList params;
  params.append("action_name", "ServerMaintenance");

  submitAction newdlg(parent, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void menuSystem::sErrorLog()
{
  omfgThis->handleNewWindow(new errorLog());
}

void menuSystem::sPrintAlignment()
{
  orReport report("Alignment");
  if (report.isValid())
    report.print();
  else
    report.reportError(omfgThis);
}

void menuSystem::sExit()
{
  parent->close();
}

void menuSystem::sAbout()
{
  QMessageBox::about( parent, tr("About..."),
                      tr( "%1\nVersion %2\n%3")
                          .arg(_Name)
                          .arg(_Version)
                          .arg(_Copyright) );
}

void menuSystem::sTOC()
{
  parent->_assClient->openAssistant();
}

void menuSystem::sFixACL()
{
  omfgThis->handleNewWindow(new fixACL());
}

void menuSystem::sFixSerial()
{
  omfgThis->handleNewWindow(new fixSerial());
}

void menuSystem::sImportXML()
{
  omfgThis->handleNewWindow(new importXML());
}

void menuSystem::sCommunityHome()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/");
}

void menuSystem::sRegister()
{
  registration newdlg(parent);
  newdlg.exec();
}

/*
void menuSystem::sCommunityNewAccount()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/index.php?option=com_registration&task=register");
}
*/

void menuSystem::sCommunityEditAccount()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/user");
}

void menuSystem::sCommunityForums()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/forum");
}

void menuSystem::sCommunityBlogs()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/blog");
}

void menuSystem::sCommunityIssues()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/issuetracker/view_all_bug_page.php");
}

void menuSystem::sCommunityWiki()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/docs");
}

void menuSystem::sCommunityDownloads()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.com/docs/download");
}

void menuSystem::sCommunitySupport()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/support");
}

void menuSystem::sCommunityTranslation()
{
  omfgThis->launchBrowser(omfgThis, "http://www.xtuple.org/translate");
}

