/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagebehaviour.h  -  The preferences panel that holds the behaviour settings
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004-2005 by Peter Simonsson
*/
#include "prefspagechatwinbehavior.h"

#include <qcheckbox.h>
#include <qspinbox.h>
#include <qgroupbox.h>

#include <klineedit.h>

#include "preferences.h"

PrefsPageChatWinBehavior::PrefsPageChatWinBehavior(QWidget* newParent, Preferences* newPreferences)
  : ChatwindowBehaviour_Config( newParent )
{
  preferences = newPreferences;

  kcfg_BeepOnAsciiBel->setChecked(preferences->getBeep());
  kcfg_HideJoinPart->setChecked(preferences->getHideUnimportantEvents());
  kcfg_DisableExpansions->setChecked(preferences->getDisableExpansion());
  kcfg_ShowRememberLine->setChecked(preferences->getShowRememberLineInAllWindows());
  kcfg_RedirectStatusMessages->setChecked(preferences->getRedirectToStatusPane());
  kcfg_UseLiteralModes->setChecked(preferences->getUseLiteralModes());

  kcfg_ScrollBackLimit->setValue(preferences->getScrollbackMax());
  kcfg_AutoWhoLimit->setValue(preferences->getAutoWhoNicksLimit());

  kcfg_autoWhoContinuous->setChecked(preferences->getAutoWhoContinuousEnabled());
  kcfg_WhoInterval->setValue(preferences->getAutoWhoContinuousInterval());
}

PrefsPageChatWinBehavior::~PrefsPageChatWinBehavior()
{
}

void PrefsPageChatWinBehavior::applyPreferences()
{
  preferences->setBeep(kcfg_BeepOnAsciiBel->isChecked());
  preferences->setHideUnimportantEvents(kcfg_HideJoinPart->isChecked());
  preferences->setDisableExpansion(kcfg_DisableExpansions->isChecked());
  preferences->setShowRememberLineInAllWindows(kcfg_ShowRememberLine->isChecked());
  preferences->setRedirectToStatusPane(kcfg_RedirectStatusMessages->isChecked());
  preferences->setUseLiteralModes(kcfg_UseLiteralModes->isChecked());
  preferences->setScrollbackMax(kcfg_ScrollBackLimit->value());
  preferences->setAutoWhoNicksLimit(kcfg_AutoWhoLimit->value());
  preferences->setAutoWhoContinuousEnabled(kcfg_autoWhoContinuous->isChecked());
  preferences->setAutoWhoContinuousInterval(kcfg_WhoInterval->value());
}

#include "prefspagechatwinbehavior.moc"
