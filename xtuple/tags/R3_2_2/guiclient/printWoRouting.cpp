/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "printWoRouting.h"

#include <QMessageBox>
#include <QVariant>

#include <openreports.h>

/*
 *  Constructs a printWoRouting as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
printWoRouting::printWoRouting(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_wo, SIGNAL(valid(bool)), _print, SLOT(setEnabled(bool)));
    connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));

    _captive = FALSE;

    _wo->setType(cWoExploded | cWoIssued);

    connect(_close, SIGNAL(clicked()), this, SLOT(close()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
printWoRouting::~printWoRouting()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void printWoRouting::languageChange()
{
    retranslateUi(this);
}

enum SetResponse printWoRouting::set(const ParameterList &pParams)
{
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("wo_id", &valid);
  if (valid)
  {
    _wo->setId(param.toInt());
    _wo->setReadOnly(TRUE);
    _copies->setFocus();
  }

  if (pParams.inList("print"))
  {
    sPrint();
    return NoError_Print;
  }

  return NoError;
}

void printWoRouting::sPrint()
{
  QPrinter printer(QPrinter::HighResolution);

  ParameterList params;
  params.append("wo_id", _wo->id());

  orReport report("Routing", params);
  bool userCanceled = false;
  if (orReport::beginMultiPrint(&printer, userCanceled) == false)
  {
    if(!userCanceled)
      systemError(this, tr("Could not initialize printing system for multiple reports."));
    return;
  }
  if (report.isValid())
  {
    for (int counter = 0; counter < _copies->value(); counter++)
      if (!report.print(&printer, (counter == 0)))
      {
	report.reportError(this);
        break;
      }
  }
  orReport::endMultiPrint(&printer);

  if (_captive)
    accept();
  else
  {
    _close->setText(tr("&Close"));
    _wo->setId(-1);
    _wo->setFocus();
  }
}

