/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The preferences GUI managing aliases
  begin:     Mon Jul 14 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGEALIASES_H
#define PREFSPAGEALIASES_H

#include "alias_preferences.h"

class Preferences;
class QFrame;

class PrefsPageAliases : public Alias_Config
{
  Q_OBJECT

  public:
    PrefsPageAliases(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageAliases();

  public slots:
    void applyPreferences();

  protected slots:
    void newAlias();
    void removeAlias();

  protected:
    Preferences* preferences;
    QFrame* parentFrame;

};

#endif
