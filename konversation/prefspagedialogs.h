/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagedialogs.h  -  Page to manage "do not show again" dialogs
  begin:     Don Mai 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGEDIALOGS_H
#define PREFSPAGEDIALOGS_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class KListView;
class QCheckListItem;

class PrefsPageDialogs : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageDialogs(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageDialogs();

  public slots:
    void applyPreferences();

  protected:
    KListView* dialogListView;
    QCheckListItem* item;
};

#endif
