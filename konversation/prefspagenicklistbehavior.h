/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagebehaviour.h  -  The preferences panel that holds the behaviour settings
  copyright: (C) 2005 by Peter Simonsson
*/
#ifndef PREFSPAGENICKLISTBEHAVIOR_H
#define PREFSPAGENICKLISTBEHAVIOR_H

#include <nicklistbehavior_preferences.h>

class Preferences;

class PrefsPageNicklistBehavior : public NicklistBehavior_Config
{
  Q_OBJECT
  public:
    PrefsPageNicklistBehavior(QWidget* newParent, Preferences* newPreferences);

  public slots:
    void applyPreferences();

  protected slots:
    void updateArrows();
    void moveUp();
    void moveDown();

  private:
    Preferences* preferences;
};

#endif
