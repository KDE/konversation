/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagetabbehavior.h  -  Provides a GUI for tab behavior
  begin:     Sun Nov 16 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGETABBEHAVIOR_H
#define PREFSPAGETABBEHAVIOR_H

#include <qcheckbox.h>

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class PrefsPageTabBehavior : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageTabBehavior(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageTabBehavior();

  public slots:
    void applyPreferences();

  protected slots:
    void closeButtonsChanged(int state);

  protected:
    QCheckBox* tabPlacementCheck;
    QCheckBox* blinkingTabsCheck;
    QCheckBox* bringToFrontCheck;
    QCheckBox* closeButtonsCheck;
    QCheckBox* closeButtonsAlignRight;
};

#endif
