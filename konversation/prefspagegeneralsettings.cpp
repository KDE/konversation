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

  autoReconnectCheck=new QCheckBox(i18n("A&uto reconnect"),parentFrame,"auto_reconnect_check");
  autoRejoinCheck=new QCheckBox(i18n("Auto re&join"),parentFrame,"auto_rejoin_check");
  autojoinOnInviteCheck=new QCheckBox(i18n("Autojoin channel on &invite"),parentFrame,"autojoin_on_invite_check");

  reconnectTimeoutLabel=new QLabel(i18n("&Reconnect timeout:"),parentFrame);
  reconnectTimeoutLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  reconnectTimeoutSpin=new QSpinBox(1,9999,1,parentFrame,"reconnect_timeout_spin");
  reconnectTimeoutSpin->setValue(preferences->getMaximumLagTime());
  reconnectTimeoutSpin->setSuffix(i18n(" seconds"));
  reconnectTimeoutLabel->setBuddy(reconnectTimeoutSpin);
  

  autoReconnectCheck->setChecked(preferences->getAutoReconnect());
  // handle ghosting of timeout widget
  autoReconnectChanged(preferences->getAutoReconnect() ? 2 : 0);

  autoRejoinCheck->setChecked(preferences->getAutoRejoin());
  autojoinOnInviteCheck->setChecked(preferences->getAutojoinOnInvite());
  

  QHBox* generalSpacer=new QHBox(parentFrame);

  int row=0;
  generalSettingsLayout->addMultiCellWidget(commandCharBox,row,row,0,2);
  row++;
  generalSettingsLayout->addWidget(autoReconnectCheck,row,0);
  generalSettingsLayout->addWidget(reconnectTimeoutLabel,row,1);
  generalSettingsLayout->addWidget(reconnectTimeoutSpin,row,2);
  row++;
  generalSettingsLayout->addMultiCellWidget(autoRejoinCheck,row,row,0,2);
  row++;
  generalSettingsLayout->addMultiCellWidget(autojoinOnInviteCheck,row,row,0,2);
  row++;
  generalSettingsLayout->addMultiCellWidget(generalSpacer,row,row,0,2);
  generalSettingsLayout->setRowStretch(row,10);

  connect(autoReconnectCheck,SIGNAL (stateChanged(int)),this,SLOT (autoReconnectChanged(int)) );
}

PrefsPageGeneralSettings::~PrefsPageGeneralSettings()
{
}

void PrefsPageGeneralSettings::autoReconnectChanged(int state)
{
  reconnectTimeoutLabel->setEnabled(state==2);
  reconnectTimeoutSpin->setEnabled(state==2);
}

void PrefsPageGeneralSettings::applyPreferences()
{
  preferences->setCommandChar(commandCharInput->text());

  preferences->setAutoReconnect(autoReconnectCheck->isChecked());
  preferences->setAutoRejoin(autoRejoinCheck->isChecked());
  preferences->setAutojoinOnInvite(autojoinOnInviteCheck->isChecked());
  preferences->setVersionReply(ctcpVersionInput->text());

  preferences->setMaximumLagTime(reconnectTimeoutSpin->value());
  
}

#include "prefspagegeneralsettings.moc"
