/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The preferences panel that holds the behaviour settings
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004-2005 by Peter Simonsson
*/
#include "prefspagebehaviour.h"

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qcombobox.h>

#include <klineedit.h>

#include "preferences.h"

PrefsPageBehaviour::PrefsPageBehaviour(QWidget* newParent, Preferences* newPreferences)
  : GeneralBehavior_Config(newParent)
{
  preferences = newPreferences;

  systrayGBox->setChecked(preferences->getShowTrayIcon());
  trayNotifyCheck->setChecked(preferences->getTrayNotify());
  trayNotifyOwnNickOnlyCheck->setChecked(preferences->trayNotifyOnlyOwnNick());
  trayOnlyCheck->setChecked(preferences->getSystrayOnly());

  showServerList->setChecked(preferences->getShowServerList());

  m_disableNotifyWhileAwayCheck->setChecked(preferences->disableNotifyWhileAway());

  useCustomBrowserCheck->setChecked(!preferences->getWebBrowserUseKdeDefault());
  browserCmdInput->setText(preferences->getWebBrowserCmd());

  commandCharInput->setText(preferences->getCommandChar());

  ctcpVersionInput->setText(preferences->getVersionReply());

  autojoinOnInviteCheck->setChecked(preferences->getAutojoinOnInvite());

  completionModeCBox->setCurrentItem(preferences->getNickCompletionMode());
  suffixStartInput->setText(preferences->getNickCompleteSuffixStart());
  suffixMiddleInput->setText(preferences->getNickCompleteSuffixMiddle());
  m_nickCompletionCaseChBox->setChecked(preferences->nickCompletionCaseSensitive());
}

PrefsPageBehaviour::~PrefsPageBehaviour()
{
}

void PrefsPageBehaviour::applyPreferences()
{
  preferences->setShowTrayIcon(systrayGBox->isChecked());
  preferences->setSystrayOnly(trayOnlyCheck->isChecked());
  preferences->setTrayNotify(trayNotifyCheck->isChecked());
  preferences->setTrayNotifyOnlyOwnNick(trayNotifyOwnNickOnlyCheck->isChecked());

  preferences->setShowServerList(showServerList->isChecked());
  preferences->setDisableNotifyWhileAway(m_disableNotifyWhileAwayCheck->isChecked());
  preferences->setWebBrowserUseKdeDefault(!useCustomBrowserCheck->isChecked());
  preferences->setWebBrowserCmd(browserCmdInput->text());

  if(!commandCharInput->text().isEmpty()) {
    preferences->setCommandChar(commandCharInput->text());
  } else {
    preferences->setCommandChar("/");
  }

  preferences->setVersionReply(ctcpVersionInput->text());
  preferences->setAutojoinOnInvite(autojoinOnInviteCheck->isChecked());

  preferences->setNickCompletionMode(completionModeCBox->currentItem());
  preferences->setNickCompleteSuffixStart(suffixStartInput->text());
  preferences->setNickCompleteSuffixMiddle(suffixMiddleInput->text());
  preferences->setNickCompletionCaseSensitive(m_nickCompletionCaseChBox->isChecked());
}

#include "prefspagebehaviour.moc"
