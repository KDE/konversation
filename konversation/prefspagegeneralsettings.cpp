/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagegeneralsettings.cpp  -  Provides a user interface to customize general settings
  begin:     Fre Nov 15 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlayout.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qwhatsthis.h>

#include <klineedit.h>
#include <klocale.h>

#include "prefspagegeneralsettings.h"
#include "preferences.h"

PrefsPageGeneralSettings::PrefsPageGeneralSettings(QFrame* newParent,Preferences* newPreferences) :
                   PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the General Settings pane
  QGridLayout* generalSettingsLayout=new QGridLayout(parentFrame,5,3,marginHint(),spacingHint(),"general_settings_layout");

  QHBox* commandCharBox=new QHBox(parentFrame);
  commandCharBox->setSpacing(spacingHint());

  QLabel* commandCharLabel=new QLabel(i18n("&Command char:"),commandCharBox);
  commandCharInput=new KLineEdit(preferences->getCommandChar(),commandCharBox);
  commandCharInput->setMaxLength(1);
  commandCharLabel->setBuddy(commandCharInput);

  QLabel* ctcpVersionLabel=new QLabel(i18n("Custom &version reply:"),commandCharBox);
  ctcpVersionInput=new KLineEdit(preferences->getVersionReply(),commandCharBox);
  ctcpVersionLabel->setBuddy(ctcpVersionInput);
  QString msg = i18n("<qt>Here you can set a custom reply for <b>CTCP <i>VERSION</i></b> requests.</qt>");
  QWhatsThis::add(ctcpVersionLabel,msg);

  QHBox* generalSpacer=new QHBox(parentFrame);

  int row=0;
  generalSettingsLayout->addMultiCellWidget(commandCharBox,row,row,0,2);
  row++;
  generalSettingsLayout->addMultiCellWidget(generalSpacer,row,row,0,2);
  generalSettingsLayout->setRowStretch(row,10);
}

PrefsPageGeneralSettings::~PrefsPageGeneralSettings()
{
}

void PrefsPageGeneralSettings::applyPreferences()
{
  preferences->setCommandChar(commandCharInput->text());
  preferences->setVersionReply(ctcpVersionInput->text());
}

#include "prefspagegeneralsettings.moc"
