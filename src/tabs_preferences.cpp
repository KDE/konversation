/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#include <qcombobox.h>
#include <qcheckbox.h>

#include "tabs_preferences.h"
#include "tabs_preferencesui.h"

Tabs_Config::Tabs_Config(QWidget *parent, const char *name)
 : Tabs_PreferencesUI(parent, name)
{
    connect(kcfg_TabPlacement, SIGNAL(activated(int)), this, SLOT(toggleCheckBoxes(int)));
}

Tabs_Config::~Tabs_Config()
{
}

void Tabs_Config::show()
{
    QWidget::show();

    if (kcfg_TabPlacement->currentItem() == 0 || kcfg_TabPlacement->currentItem() == 1)
    {
        kcfg_ShowTabBarCloseButton->setEnabled(true);
        kcfg_UseMaxSizedTabs->setEnabled(true);
    }
    else
    {
        kcfg_ShowTabBarCloseButton->setEnabled(false);
        kcfg_UseMaxSizedTabs->setEnabled(false);
    }
}

void Tabs_Config::toggleCheckBoxes(int activated)
{
    if (activated == 0 || activated == 1)
    {
        kcfg_ShowTabBarCloseButton->setEnabled(true);
        kcfg_UseMaxSizedTabs->setEnabled(true);
    }
    else
    {
        kcfg_ShowTabBarCloseButton->setEnabled(false);
        kcfg_UseMaxSizedTabs->setEnabled(false);
    }
}

#include "tabs_preferences.moc"
