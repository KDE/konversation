/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2005 by Peter Simonsson
*/
#ifndef PREFSPAGEFONTSAPPEARANCE_H
#define PREFSPAGEFONTSAPPEARANCE_H

#include <fontappearance_preferences.h>

class Preferences;

class PrefsPageFontsAppearance : public FontAppearance_Config
{
    Q_OBJECT
        public:
        PrefsPageFontsAppearance(QWidget* newParent, Preferences* newPreferences);

    public slots:
        void applyPreferences();

    protected:
        Preferences* preferences;
};
#endif
