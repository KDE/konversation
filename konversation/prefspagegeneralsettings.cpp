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

  $Id$
*/

#include <qlayout.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qgrid.h>
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
  commandCharInput->setMaxLength(1);

  // double click actions
  QVBox* actionBox=new QVBox(parentFrame);
  new QLabel(i18n("Commands to execute when doubleclicked in"),actionBox);

  QGrid* actionEditBox=new QGrid(2,actionBox);
  actionEditBox->setSpacing(spacingHint());

  new QLabel(i18n("Nick list:"),actionEditBox);
  KLineEdit* channelActionInput=new KLineEdit(preferences->getChannelDoubleClickAction(),actionEditBox);

  new QLabel(i18n("Notify list:"),actionEditBox);
  KLineEdit* notifyActionInput=new KLineEdit(preferences->getNotifyDoubleClickAction(),actionEditBox);

  // nick completion special settings
  QVBox* suffixBox=new QVBox(parentFrame);
  new QLabel(i18n("Characters to add on nick completion:"),suffixBox);

  QHBox* suffixEditBox=new QHBox(suffixBox);
  suffixEditBox->setSpacing(spacingHint());
  new QLabel(i18n("at start of line:"),suffixEditBox);
  KLineEdit* suffixStartInput=new KLineEdit(preferences->getNickCompleteSuffixStart(),suffixEditBox);

  QLabel* middleLabel=new QLabel(i18n("Elsewhere:"),suffixEditBox);
  middleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  KLineEdit* suffixMiddleInput=new KLineEdit(preferences->getNickCompleteSuffixMiddle(),suffixEditBox);

  QCheckBox* autoReconnectCheck=new QCheckBox(i18n("Auto Reconnect"),parentFrame,"auto_reconnect_check");
  QCheckBox* autoRejoinCheck=new QCheckBox(i18n("Auto Rejoin"),parentFrame,"auto_rejoin_check");
  QCheckBox* blinkingTabsCheck=new QCheckBox(i18n("Blinking Tabs"),parentFrame,"blinking_tabs_check");
  QCheckBox* bringToFrontCheck=new QCheckBox(i18n("Bring new tabs to front"),parentFrame,"bring_to_front_check");
  QCheckBox* fixedMOTDCheck=new QCheckBox(i18n("Show MOTD in fixed font"),parentFrame,"fixed_motd_check");
  QCheckBox* beepCheck=new QCheckBox(i18n("Beep on incoming ASCII BEL"),parentFrame,"beep_check");
  QCheckBox* rawLogCheck=new QCheckBox(i18n("Show raw log window on startup"),parentFrame,"raw_log_check");

  autoReconnectCheck->setChecked(preferences->getAutoReconnect());
  autoRejoinCheck->setChecked(preferences->getAutoRejoin());
  blinkingTabsCheck->setChecked(preferences->getBlinkingTabs());
  bringToFrontCheck->setChecked(preferences->getBringToFront());
  fixedMOTDCheck->setChecked(preferences->getFixedMOTD());
  beepCheck->setChecked(preferences->getBeep());
  beepCheck->setChecked(preferences->getRawLog());

  QHBox* generalSpacer=new QHBox(parentFrame);

  int row=0;
  generalSettingsLayout->addWidget(commandCharLabel,row,0);
  generalSettingsLayout->addWidget(commandCharInput,row,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(suffixBox,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(actionBox,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(autoReconnectCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(autoRejoinCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(blinkingTabsCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(bringToFrontCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(fixedMOTDCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(beepCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(rawLogCheck,row,row,0,1);
  row++;
  generalSettingsLayout->addMultiCellWidget(generalSpacer,row,row,0,1);
  generalSettingsLayout->setRowStretch(row,10);

  connect(commandCharInput,SIGNAL (textChanged(const QString&)),this,SLOT (commandCharChanged(const QString&)) );

  connect(suffixStartInput,SIGNAL (textChanged(const QString&)),this,SLOT (suffixStartChanged(const QString&)) );
  connect(suffixMiddleInput,SIGNAL (textChanged(const QString&)),this,SLOT (suffixMiddleChanged(const QString&)) );

  connect(channelActionInput,SIGNAL (textChanged(const QString&)),this,SLOT (channelActionChanged(const QString&)) );
  connect(notifyActionInput,SIGNAL (textChanged(const QString&)),this,SLOT (notifyActionChanged(const QString&)) );

  connect(autoReconnectCheck,SIGNAL (stateChanged(int)),this,SLOT (autoReconnectChanged(int)) );
  connect(autoRejoinCheck,SIGNAL (stateChanged(int)),this,SLOT (autoRejoinChanged(int)) );

  connect(bringToFrontCheck,SIGNAL (stateChanged(int)),this,SLOT (bringToFrontChanged(int)) );
  connect(blinkingTabsCheck,SIGNAL (stateChanged(int)),this,SLOT (blinkingTabsChanged(int)) );

  connect(fixedMOTDCheck,SIGNAL (stateChanged(int)),this,SLOT (fixedMOTDChanged(int)) );

  connect(beepCheck,SIGNAL (stateChanged(int)),this,SLOT (beepChanged(int)) );
  
  connect(rawLogCheck,SIGNAL (stateChanged(int)),this,SLOT (rawLogChanged(int)) );
}

PrefsPageGeneralSettings::~PrefsPageGeneralSettings()
{
}

void PrefsPageGeneralSettings::commandCharChanged(const QString& newChar)
{
  preferences->setCommandChar(newChar);
}

void PrefsPageGeneralSettings::suffixStartChanged(const QString& newSuffix)
{
  preferences->setNickCompleteSuffixStart(newSuffix);
}

void PrefsPageGeneralSettings::suffixMiddleChanged(const QString& newSuffix)
{
  preferences->setNickCompleteSuffixMiddle(newSuffix);
}

void PrefsPageGeneralSettings::channelActionChanged(const QString& newAction)
{
  preferences->setChannelDoubleClickAction(newAction);
}

void PrefsPageGeneralSettings::notifyActionChanged(const QString& newAction)
{
  preferences->setNotifyDoubleClickAction(newAction);
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

void PrefsPageGeneralSettings::bringToFrontChanged(int state)
{
  preferences->setBringToFront(state==2);
}

void PrefsPageGeneralSettings::fixedMOTDChanged(int state)
{
  preferences->setFixedMOTD(state==2);
}

void PrefsPageGeneralSettings::beepChanged(int state)
{
  preferences->setBeep(state==2);
}

void PrefsPageGeneralSettings::rawLogChanged(int state)
{
  preferences->setRawLog(state==2);
}

#include "prefspagegeneralsettings.moc"
