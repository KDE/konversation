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

#ifndef PREFSPAGETABBEHAVIOR_H
#define PREFSPAGETABBEHAVIOR_H

#include "tabbar_preferences.h"
class Preferences;

/*
  @author Dario Abatianni
*/

class PrefsPageTabBehavior : public TabBar_Config
{
  Q_OBJECT

  public:
    PrefsPageTabBehavior(QWidget* newParent,Preferences* newPreferences);

  public slots:
    void applyPreferences();

  protected slots:
    void closeButtonsChanged(int state);
    void bringToFrontCheckChanged(int state);

	protected:
		Preferences* preferences;
		
};

#endif
