/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
	     (C) 2004 by İsmail Dönmez
*/
#ifndef PREFSPAGECOLORSAPPEARANCE_H
#define PREFSPAGECOLORSAPPEARANCE_H

#include <qframe.h>

#include "colorsappearance_preferences.h"

class Preferences;

class PrefsPageColorsAppearance : public ColorsAppearance_Config
{
  Q_OBJECT
  public:
    PrefsPageColorsAppearance(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageColorsAppearance();

  public slots:
    void applyPreferences();

  protected:
    Preferences* preferences;
};

#endif
