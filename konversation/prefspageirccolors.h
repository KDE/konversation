/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageirccolors.h  -  Provides a user interface to customize IRC colors
  begin:     Wed 8 July 2003
  copyright: (C) 2003 Peter Simonsson
  email:     psn@linux.se
*/
#ifndef PREFSPAGEIRCCOLORS_H
#define PREFSPAGEIRCCOLORS_H

#include "prefspage.h"

class PrefsPageIRCColorsUI;

class PrefsPageIRCColors : public PrefsPage
{
  Q_OBJECT
  public:
    PrefsPageIRCColors(QFrame* newParent, Preferences* newPreferences);

  public slots:
    void applyPreferences();

  protected:
    PrefsPageIRCColorsUI* m_widget;
};
#endif
