/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Page to manage "do not show again" dialogs
  begin:     Don Mai 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGEDIALOGS_H
#define PREFSPAGEDIALOGS_H

#include "warnings_preferences.h"

class Preferences;
class QCheckListItem;

class PrefsPageDialogs : public Warnings_Config
{
  Q_OBJECT

  public:
    PrefsPageDialogs(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageDialogs();
    QString flagNames;

  public slots:
    void applyPreferences();

  protected:
    Preferences* preferences;
    QCheckListItem* item;
    
};

#endif
