/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2005 by Peter Simonsson
*/
#include "prefspagefontsappearance.h"

#include <qcheckbox.h>

#include <kfontrequester.h>

#include "preferences.h"

PrefsPageFontsAppearance::PrefsPageFontsAppearance(QWidget* newParent, Preferences* newPreferences)
: FontAppearance_Config(newParent)
{
    preferences = newPreferences;

    kcfg_TextFont->setFont(preferences->getTextFont());
    kcfg_ListFont->setFont(preferences->getListFont());
    kcfg_FixedMOTD->setChecked(preferences->getFixedMOTD());
    kcfg_UseBoldNicks->setChecked(preferences->getUseBoldNicks());
}

void PrefsPageFontsAppearance::applyPreferences()
{
    preferences->setTextFont(kcfg_TextFont->font());
    preferences->setListFont(kcfg_ListFont->font());
    preferences->setFixedMOTD(kcfg_FixedMOTD->isChecked());
    preferences->setUseBoldNicks(kcfg_UseBoldNicks->isChecked());
}

#include "prefspagefontsappearance.moc"
