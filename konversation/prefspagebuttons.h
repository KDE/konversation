/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides an interface to edit the quick buttons
  begin:     Mon Jun 9 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGEBUTTONS_H
#define PREFSPAGEBUTTONS_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class KListView;

class PrefsPageButtons : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageButtons(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageButtons();

  public slots:
    void applyPreferences();

  protected:
    QStringList getButtonList();

    KListView* buttonListView;
};

#endif
