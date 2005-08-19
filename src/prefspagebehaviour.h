/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The preferences panel that holds the behaviour settings
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#ifndef PREFSPAGEBEHAVIOUR_H
#define PREFSPAGEBEHAVIOUR_H

#include "generalbehavior_preferences.h"

class Preferences;

class PrefsPageBehaviour : public GeneralBehavior_Config
{
    Q_OBJECT
        public:
        PrefsPageBehaviour(QWidget* newParent, Preferences* newPreferences);
        ~PrefsPageBehaviour();

    public slots:
        void applyPreferences();

    private:
        Preferences* preferences;
};
#endif
