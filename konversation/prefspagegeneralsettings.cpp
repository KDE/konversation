/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagegeneralsettings.cpp  -  description
  begin:     Fre Nov 15 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qcheckbox.h>
#include <qlabel.h>

#include <klineedit.h>

#include "prefspagegeneralsettings.h"

PrefsPageGeneralSettings::PrefsPageGeneralSettings(QFrame* newParent,Preferences* newPreferences) :
                   PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the General Settings pane
  QGridLayout* generalSettingsLayout=new QGridLayout(parentFrame,5,2,marginHint(),spacingHint(),"general_settings_layout");

  QLabel* commandCharLabel=new QLabel(i18n("Command Char:"),parentFrame);
  KLineEdit* commandCharInput=new KLineEdit(preferences->getCommandChar(),parentFrame);
  QCheckBox* autoReconnectCheck=new QCheckBox(i18n("Auto Reconnect"),parentFrame,"auto_reconnect_check");
  QCheckBox* autoRejoinCheck=new QCheckBox(i18n("Auto Rejoin"),parentFrame,"auto_rejoin_check");
  QCheckBox* blinkingTabsCheck=new QCheckBox(i18n("Blinking Tabs"),parentFrame,"blinking_tabs_check");
  QCheckBox* fixedMOTDCheck=new QCheckBox(i18n("Show MOTD in fixed font"),parentFrame,"fixed_motd_check");

  autoReconnectCheck->setChecked(preferences->getAutoReconnect());
  autoRejoinCheck->setChecked(preferences->getAutoRejoin());
  blinkingTabsCheck->setChecked(preferences->getBlinkingTabs());
  fixedMOTDCheck->setChecked(preferences->getFixedMOTD());

  QHBox* generalSpacer=new QHBox(parentFrame);

  int row=0;
  generalSettingsLayout->addWidget(commandCharLabel,row,0);
  generalSettingsLayout->addWidget(commandCharInput,row,1);

  row++;
  generalSettingsLayout->addMultiCellWidget(autoReconnectCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(autoRejoinCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(blinkingTabsCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(fixedMOTDCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(generalSpacer,row,row,0,1);
  generalSettingsLayout->setRowStretch(row,10);

  connect(commandCharInput,SIGNAL (textChanged(const QString&)),this,SLOT (commandCharChanged(const QString&)) );
  connect(autoReconnectCheck,SIGNAL (stateChanged(int)),this,SLOT (autoReconnectChanged(int)) );
  connect(autoRejoinCheck,SIGNAL (stateChanged(int)),this,SLOT (autoRejoinChanged(int)) );
  connect(blinkingTabsCheck,SIGNAL (stateChanged(int)),this,SLOT (blinkingTabsChanged(int)) );
  connect(fixedMOTDCheck,SIGNAL (stateChanged(int)),this,SLOT (fixedMOTDChanged(int)) );
}

PrefsPageGeneralSettings::~PrefsPageGeneralSettings()
{
}

void PrefsPageGeneralSettings::commandCharChanged(const QString& newChar)
{
  preferences->setCommandChar(newChar);
}

void PrefsPageGeneralSettings::autoReconnectChanged(int state)
{
  preferences->setAutoReconnect(state==2);
}

void PrefsPageGeneralSettings::autoRejoinChanged(int state)
{
  preferences->setAutoRejoin(state==2);
}

void PrefsPageGeneralSettings::blinkingTabsChanged(int state)
{
  preferences->setBlinkingTabs(state==2);
}

void PrefsPageGeneralSettings::fixedMOTDChanged(int state)
{
  preferences->setFixedMOTD(state==2);
}

