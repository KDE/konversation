/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
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
