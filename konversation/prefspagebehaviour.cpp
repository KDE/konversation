/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagebehaviour.cpp  -  The preferences panel that holds the behaviour settings
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#include "prefspagebehaviour.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qcombobox.h>
#include <qwhatsthis.h>
#include <qspinbox.h>

#include <klocale.h>
#include <klineedit.h>

#include "preferences.h"

PrefsPageBehaviour::PrefsPageBehaviour(QFrame* newParent, Preferences* newPreferences)
  : PrefsPage(newParent, newPreferences)
{
  QGridLayout* generalLayout = new QGridLayout(parentFrame, 4, 2, marginHint(), spacingHint());

  trayIconCheck = new QCheckBox(i18n("Show icon in s&ystem tray"), parentFrame, "tray_icon_check");
  trayIconCheck->setChecked(preferences->getShowTrayIcon());
  trayNotifyCheck = new QCheckBox(i18n("Use sys&tem tray for new message notification"), parentFrame,"tray_notify_check");
  trayNotifyCheck->setEnabled(trayIconCheck->isChecked());
  trayNotifyCheck->setChecked(preferences->getTrayNotify());
  connect(trayIconCheck, SIGNAL(toggled(bool)), trayNotifyCheck, SLOT(setEnabled(bool)));

  rawLogCheck = new QCheckBox(i18n("Show ra&w log window on startup"), parentFrame, "raw_log_check");
  rawLogCheck->setChecked(preferences->getRawLog());
      
  useCustomBrowserCheck = new QCheckBox(i18n("Use custom web browser:"), parentFrame, "useCustomBrowserCheck");
  useCustomBrowserCheck->setChecked(!preferences->getWebBrowserUseKdeDefault());
  browserCmdInput = new KLineEdit(parentFrame, "browserCmdInput");
  browserCmdInput->setEnabled(useCustomBrowserCheck->isChecked());
  browserCmdInput->setText(preferences->getWebBrowserCmd());
  connect(useCustomBrowserCheck, SIGNAL(toggled(bool)), browserCmdInput, SLOT(setEnabled(bool)));
  
  QGroupBox* connectionGroup = new QGroupBox(i18n("Connection"), parentFrame, "connectionGroup");
  connectionGroup->setColumnLayout(0, Qt::Vertical);
  connectionGroup->setMargin(marginHint());
  QGridLayout* connectionLayout = new QGridLayout(connectionGroup->layout(), 3, 2, spacingHint());

  autoReconnectCheck = new QCheckBox(i18n("A&uto reconnect"), connectionGroup, "auto_reconnect_check");
  autoReconnectCheck->setChecked(preferences->getAutoReconnect());

  QLabel* reconnectTimeoutLabel = new QLabel(i18n("&Reconnect timeout:"), connectionGroup);
  reconnectTimeoutLabel->setEnabled(autoReconnectCheck->isChecked());
  reconnectTimeoutSpin = new QSpinBox(1, 9999, 1, connectionGroup, "reconnect_timeout_spin");
  reconnectTimeoutSpin->setEnabled(autoReconnectCheck->isChecked());
  reconnectTimeoutSpin->setValue(preferences->getMaximumLagTime());
  reconnectTimeoutSpin->setSuffix(i18n(" seconds"));
  reconnectTimeoutLabel->setBuddy(reconnectTimeoutSpin);
  connect(autoReconnectCheck, SIGNAL(toggled(bool)), reconnectTimeoutLabel, SLOT(setEnabled(bool)));
  connect(autoReconnectCheck, SIGNAL(toggled(bool)), reconnectTimeoutSpin, SLOT(setEnabled(bool)));

  autoRejoinCheck = new QCheckBox(i18n("Auto re&join"), connectionGroup, "auto_rejoin_check");
  autojoinOnInviteCheck = new QCheckBox(i18n("Autojoin channel on &invite"), connectionGroup, "autojoin_on_invite_check");
  autoRejoinCheck->setChecked(preferences->getAutoRejoin());
  autojoinOnInviteCheck->setChecked(preferences->getAutojoinOnInvite());
  
  int row = 0;
  connectionLayout->addMultiCellWidget(autoReconnectCheck, row, row, 0, 1);
  row++;
  connectionLayout->addWidget(reconnectTimeoutLabel, row, 0);
  connectionLayout->addWidget(reconnectTimeoutSpin, row, 1);
  row++;
  connectionLayout->addMultiCellWidget(autoRejoinCheck, row, row, 0, 1);
  row++;
  connectionLayout->addMultiCellWidget(autojoinOnInviteCheck, row, row, 0, 1);
  connectionLayout->setColStretch(1, 10);
    
  QGroupBox* nickCompletionGroup = new QGroupBox(i18n("Nickname Completion"), parentFrame, "nickCompletionGroup");
  nickCompletionGroup->setMargin(marginHint());
  nickCompletionGroup->setColumnLayout(0, Qt::Vertical);
  QGridLayout* nickCompletionLayout = new QGridLayout(nickCompletionGroup->layout(), 2, 4, spacingHint());

  QLabel* modeLbl = new QLabel(i18n("Completion &mode:"), nickCompletionGroup);
  completionModeCBox = new QComboBox(nickCompletionGroup, "completionModeCBox");
  completionModeCBox->insertItem(i18n("Cycle Nicklist"));
  completionModeCBox->insertItem(i18n("Shell-Like"));
  completionModeCBox->insertItem(i18n("Shell-Like with Completion Box"));
  completionModeCBox->setCurrentItem(preferences->getNickCompletionMode());
  modeLbl->setBuddy(completionModeCBox);

  QLabel* startOfLineLabel = new QLabel(i18n("Suffix at &start of line:"), nickCompletionGroup);
  suffixStartInput = new KLineEdit(preferences->getNickCompleteSuffixStart(), nickCompletionGroup);
  startOfLineLabel->setBuddy(suffixStartInput);

  QLabel* middleLabel = new QLabel(i18n("&Elsewhere:"), nickCompletionGroup);
  middleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  suffixMiddleInput = new KLineEdit(preferences->getNickCompleteSuffixMiddle(), nickCompletionGroup);
  middleLabel->setBuddy(suffixMiddleInput);
    
  row = 0;
  nickCompletionLayout->addWidget(modeLbl, row, 0);
  nickCompletionLayout->addWidget(completionModeCBox, row, 1);
  row++;
  nickCompletionLayout->addWidget(startOfLineLabel, row, 0);
  nickCompletionLayout->addWidget(suffixStartInput, row, 1);
  nickCompletionLayout->addWidget(middleLabel, row, 2);
  nickCompletionLayout->addWidget(suffixMiddleInput, row, 3);
  
  row = 0;
  generalLayout->addMultiCellWidget(trayIconCheck, row, row, 0, 1);
  row++;
  generalLayout->addMultiCellWidget(trayNotifyCheck, row, row, 0, 1);
  row++;
  generalLayout->addMultiCellWidget(rawLogCheck, row, row, 0, 1);
  row++;
  generalLayout->addWidget(useCustomBrowserCheck, row, 0);
  generalLayout->addWidget(browserCmdInput, row, 1);
  row++;
  generalLayout->addMultiCellWidget(connectionGroup, row, row, 0, 1);
  row++;
  generalLayout->addMultiCellWidget(nickCompletionGroup, row, row, 0, 1);
  row++;
  generalLayout->setRowStretch(row, 10);
}

PrefsPageBehaviour::~PrefsPageBehaviour()
{
}

void PrefsPageBehaviour::applyPreferences()
{
  preferences->setShowTrayIcon(trayIconCheck->isChecked());
  preferences->setTrayNotify(trayNotifyCheck->isChecked());
  preferences->setRawLog(rawLogCheck->isChecked());
  preferences->setWebBrowserUseKdeDefault(!useCustomBrowserCheck->isChecked());
  preferences->setWebBrowserCmd(browserCmdInput->text());

  preferences->setAutoReconnect(autoReconnectCheck->isChecked());
  preferences->setAutoRejoin(autoRejoinCheck->isChecked());
  preferences->setAutojoinOnInvite(autojoinOnInviteCheck->isChecked());
  preferences->setMaximumLagTime(reconnectTimeoutSpin->value());
    
  preferences->setNickCompletionMode(completionModeCBox->currentItem());
  preferences->setNickCompleteSuffixStart(suffixStartInput->text());
  preferences->setNickCompleteSuffixMiddle(suffixMiddleInput->text());
}

#include "prefspagebehaviour.moc"
