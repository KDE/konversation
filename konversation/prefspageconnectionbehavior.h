/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2005 by Peter Simonsson <psn@linux.se>
*/
#ifndef PREFSPAGECONNECTIONBEHAVIOR_H
#define PREFSPAGECONNECTIONBEHAVIOR_H

#include "connectionbehavior_preferences.h"

class Preferences;

class PrefsPageConnectionBehavior : public ConnectionBehavior_Config
{
  Q_OBJECT
  public:
    PrefsPageConnectionBehavior(QWidget* newParent, Preferences* newPreferences);
    ~PrefsPageConnectionBehavior();

  public slots:
    void applyPreferences();

  private:
    Preferences* preferences;
};

#endif
