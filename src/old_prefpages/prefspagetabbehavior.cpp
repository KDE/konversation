/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides a GUI for tab behavior
  begin:     Sun Nov 16 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <klocale.h>

#include "prefspagetabbehavior.h"
#include "preferences.h"

PrefsPageTabBehavior::PrefsPageTabBehavior(QWidget* newParent,Preferences* newPreferences) :
TabBar_Config(newParent)
{
    preferences = newPreferences;

    kcfg_CloseButtonsOnTabs->setChecked(preferences->getCloseButtonsOnTabs());
    kcfg_CloseButtonsAlignRight->setChecked(preferences->getCloseButtonsAlignRight());
    connect(kcfg_CloseButtonsOnTabs, SIGNAL(stateChanged(int)), this, SLOT(closeButtonsChanged(int)));
    closeButtonsChanged(preferences->getCloseButtonsOnTabs() ? 2 : 0);

    kcfg_TabPlacement->setChecked(preferences->getTabPlacement() == Preferences::Top);
    kcfg_BlinkingTabs->setChecked(preferences->getBlinkingTabs());
    kcfg_BringToFront->setChecked(preferences->getBringToFront());
    kcfg_FocusNewQueries->setChecked(preferences->getFocusNewQueries());
    connect(kcfg_BringToFront, SIGNAL(stateChanged(int)), this, SLOT(bringToFrontCheckChanged(int)));

    kcfg_ShowTabBarCloseButton->setChecked(preferences->getShowTabBarCloseButton());
}

void PrefsPageTabBehavior::closeButtonsChanged(int state)
{
    kcfg_CloseButtonsOnTabs->setChecked(state);
    kcfg_CloseButtonsAlignRight->setEnabled(state==2);
}

void PrefsPageTabBehavior::bringToFrontCheckChanged(int state)
{
    kcfg_BringToFront->setChecked(state);
    kcfg_FocusNewQueries->setEnabled(state==2);
}

void PrefsPageTabBehavior::applyPreferences()
{
    preferences->setTabPlacement(kcfg_TabPlacement->isChecked() ? Preferences::Top : Preferences::Bottom);
    preferences->setBlinkingTabs(kcfg_BlinkingTabs->isChecked());
    preferences->setBringToFront(kcfg_BringToFront->isChecked());
    preferences->setFocusNewQueries(kcfg_FocusNewQueries->isChecked());
    preferences->setCloseButtonsOnTabs(kcfg_CloseButtonsOnTabs->isChecked());
    preferences->setCloseButtonsAlignRight(kcfg_CloseButtonsAlignRight->isChecked());
    preferences->setShowTabBarCloseButton(kcfg_ShowTabBarCloseButton->isChecked());
}

#include "prefspagetabbehavior.moc"
