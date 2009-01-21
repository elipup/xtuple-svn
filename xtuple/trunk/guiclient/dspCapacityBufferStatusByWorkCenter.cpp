/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspCapacityBufferStatusByWorkCenter.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>
#include <parameter.h>

dspCapacityBufferStatusByWorkCenter::dspCapacityBufferStatusByWorkCenter(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sQuery()));

  _wrkcnt->populate( "SELECT wrkcnt_id, (wrkcnt_code || '-' || wrkcnt_descrip) "
                     "FROM wrkcnt "
                     "ORDER BY wrkcnt_code;" );

  _roughCut->addColumn(tr("Site"),          _whsColumn,  Qt::AlignCenter,true, "warehous_code");
  _roughCut->addColumn(tr("Work Center"),   -1,          Qt::AlignLeft,  true, "wrkcnt_code");
  _roughCut->addColumn(tr("Total Setup"),   _timeColumn, Qt::AlignRight, true, "sutime");
  _roughCut->addColumn(tr("Total Run"),     _timeColumn, Qt::AlignRight, true, "rntime");
  _roughCut->addColumn(tr("Daily Capacity"),_uomColumn,  Qt::AlignRight, true, "dailycap");
  _roughCut->addColumn(tr("Days Load"),     _uomColumn,  Qt::AlignRight, true, "daysload");
  _roughCut->addColumn(tr("Buffer Status"), _uomColumn,  Qt::AlignRight, true, "bufrsts");
}

dspCapacityBufferStatusByWorkCenter::~dspCapacityBufferStatusByWorkCenter()
{
    // no need to delete child widgets, Qt does it all for us
}

void dspCapacityBufferStatusByWorkCenter::languageChange()
{
    retranslateUi(this);
}

void dspCapacityBufferStatusByWorkCenter::sPrint()
{
  ParameterList params;

  if (_selectedWorkCenter->isChecked())
    params.append("wrkcnt_id", _wrkcnt->id());

  _warehouse->appendValue(params);
  params.append("maxdaysload", _maxdaysload->value());

  orReport report("CapacityBufferStatusByWorkCenter", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspCapacityBufferStatusByWorkCenter::sQuery()
{
  QString sql( "SELECT *, "
               "       CASE WHEN bufrsts > 66 THEN 'error'"
               "       END AS bufrsts_qtforegroundrole,"
               "       '1' AS dailycap_xtnumericrole,"
               "       '1' AS daysload_xtnumericrole,"
               "      'N/A' AS bfrsts_xtnullrole "
               "FROM ("
               "SELECT wrkcnt_id, warehous_code, wrkcnt_code,"
               "       SUM(CASE WHEN wooper_sucomplete THEN 0"
               "                ELSE NoNeg((wooper_sutime-wooper_suconsumed))"
               "           END) AS sutime,"
               "       SUM(NoNeg(wooper_rntime-wooper_rnconsumed)) AS rntime,"
               "       (wrkcnt_dailycap *"
               "                 CASE WHEN(COALESCE(wrkcnt_efficfactor,0.0)=0.0) THEN 1.0"
               "                      ELSE wrkcnt_efficfactor END) AS dailycap,"
               "       SUM((CASE WHEN wooper_sucomplete THEN 0"
               "                 ELSE NoNeg(wooper_sutime-wooper_suconsumed)"
               "            END)+NoNeg(wooper_rntime-wooper_rnconsumed)) / "
               "          (CASE WHEN(COALESCE(wrkcnt_dailycap,0.0)=0.0) THEN 1.0"
               "                ELSE wrkcnt_dailycap"
               "           END *"
               "           CASE WHEN(COALESCE(wrkcnt_efficfactor,0.0)=0.0) THEN 1.0"
               "                ELSE wrkcnt_efficfactor END) AS daysload,"

               "         CASE WHEN (:maxdaysload <= 0) THEN NULL "
               "              ELSE calcbufferstatus(:maxdaysload,"
               "                                    :maxdaysload-(SUM("
               "                           (CASE WHEN wooper_sucomplete THEN 0"
               "                                 ELSE NoNeg(wooper_sutime-wooper_suconsumed)"
               "                            END)"
               "                          +NoNeg(wooper_rntime-wooper_rnconsumed)"
               "                         ) / (CASE WHEN(COALESCE(wrkcnt_dailycap,0.0)=0.0) THEN 1.0 ELSE wrkcnt_dailycap END*CASE WHEN(COALESCE(wrkcnt_efficfactor,0.0)=0.0) THEN 1.0 ELSE wrkcnt_efficfactor END)))"
               "         END AS bufrsts "
               "  FROM wooper, wo, wrkcnt, warehous "
               " WHERE ( (wooper_wo_id=wo_id)"
               "   AND   (wooper_wrkcnt_id=wrkcnt_id)"
               "   AND   (wrkcnt_warehous_id=warehous_id)"
               "   AND   (wo_status<>'C')"
               "   AND   NOT wooper_rncomplete"
               "   AND   (((CASE WHEN wooper_sucomplete THEN 0"
               "                 ELSE NoNeg((wooper_sutime-wooper_suconsumed))"
               "            END) + NoNeg(wooper_rntime-wooper_rnconsumed)) > 0)");

  if (_selectedWorkCenter->isChecked())
    sql += " AND (wrkcnt_id=:wrkcnt_id)";

  if (_warehouse->isSelected())
    sql += " AND (warehous_id=:warehous_id)";

  sql += ")"
         " GROUP BY wrkcnt_id, warehous_code, wrkcnt_code, wrkcnt_dailycap, wrkcnt_efficfactor"
         " ) AS dummy "
         "ORDER BY bufrsts DESC, warehous_code, wrkcnt_code";

  q.prepare(sql);
  q.bindValue(":maxdaysload", _maxdaysload->value());
  _warehouse->bindValue(q);
  q.bindValue(":wrkcnt_id", _wrkcnt->id());
  q.exec();
  _roughCut->populate(q);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
