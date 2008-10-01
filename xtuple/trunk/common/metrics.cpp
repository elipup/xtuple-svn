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


#include "metrics.h"
#include <QVariant>
#include "xsqlquery.h"

Parameters::Parameters(QObject * parent)
  : QObject(parent)
{
  _dirty = FALSE;
}

void Parameters::load()
{
  _values.clear();

  XSqlQuery q;
  q.prepare(_readSql);
  q.bindValue(":username", _username);
  q.exec();
  while (q.next())
    _values[q.value("key").toString()] = q.value("value").toString();

  _dirty = FALSE;
}

QString Parameters::value(const char *pName)
{
  return value(QString(pName));
}

QString Parameters::value(const QString &pName)
{
  MetricMap::iterator it = _values.find(pName);
  if (it == _values.end())
    return QString::null;
  else
    return it.data();
}

bool Parameters::boolean(const char *pName)
{
  return boolean(QString(pName));
}

bool Parameters::boolean(const QString &pName)
{
  MetricMap::iterator it = _values.find(pName);
  if (it == _values.end())
    return FALSE;
  else if (it.data() == "t")
    return TRUE;

  return FALSE;
}

void Parameters::set(const char *pName, bool pValue)
{
  set(pName, QString(pValue ? "t" : "f"));
}

void Parameters::set(const QString &pName, bool pValue)
{
  set(pName, QString(pValue ? "t" : "f"));
}

void Parameters::set(const char *pName, int pValue)
{
  set(QString(pName), pValue);
}

void Parameters::set(const QString &pName, int pValue)
{
  set(QString(pName), QString::number(pValue));
}

void Parameters::set(const char *pName, const QString &pValue)
{
  set(QString(pName), pValue);
}

void Parameters::set(const QString &pName, const QString &pValue)
{
  MetricMap::iterator it = _values.find(pName);
  if (it != _values.end())
  {
    if (it.data() == pValue)
      return;
    else
      _values.replace(pName, pValue);
  }
  else
    _values[pName] = pValue;

  _set(pName, pValue);
}

void Parameters::_set(const QString &pName, QVariant pValue)
{
  XSqlQuery q;
  q.prepare(_setSql);
  q.bindValue(":username", _username);
  q.bindValue(":name", pName);
  q.bindValue(":value", pValue);
  q.exec();

  _dirty = TRUE;
}

QString Parameters::parent(const QString &pValue)
{
  for (MetricMap::iterator it = _values.begin(); it != _values.end(); it++)
    if (it.data() == pValue)
      return it.key();

  return QString::null;
}


Metrics::Metrics()
{
  _readSql = "SELECT metric_name AS key, metric_value AS value FROM metric;";
  _setSql  = "SELECT setMetric(:name, :value);";

  load();
}


Preferences::Preferences(const QString &pUsername)
{
  _readSql  = "SELECT usrpref_name AS key, usrpref_value AS value "
              "FROM usrpref "
              "WHERE (usrpref_username=:username);";
  _setSql   = "SELECT setUserPreference(:username, :name, :value);";
  _username = pUsername;

  load();
}

void Preferences::remove(const QString &pPrefName)
{
  XSqlQuery q;
  q.prepare("SELECT deleteUserPreference(:prefname);");
  q.bindValue(":prefname", pPrefName);
  q.exec();

  _dirty = TRUE;
}


Privileges::Privileges()
{
  _readSql = "SELECT priv_name AS key, TEXT('t') AS value "
             "  FROM usrpriv, priv "
             " WHERE((usrpriv_priv_id=priv_id)"
             "   AND (usrpriv_username=CURRENT_USER)) "
             " UNION "
             "SELECT priv_name AS key, TEXT('t') AS value "
             "  FROM priv, grppriv, usrgrp"
             " WHERE((usrgrp_grp_id=grppriv_grp_id)"
             "   AND (grppriv_priv_id=priv_id)"
             "   AND (usrgrp_username=CURRENT_USER));";

  load();
}

bool Privileges::check(const QString &pName)
{
  MetricMap::iterator it = _values.find(pName);
  if (it == _values.end())
    return FALSE;
  else
    return TRUE;
}

