/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspage.h  -  This is the base class used for settings user interfaces
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "qobject.h"

#ifndef PREFSPAGE_H
#define PREFSPAGE_H

/*
  @author Dario Abatianni
*/

class QFrame;
class Preferences;

class PrefsPage : public QObject
{
  Q_OBJECT

  public:
    PrefsPage(QFrame* newParent,Preferences* preferences);
    ~PrefsPage();

    int marginHint() const;
    int spacingHint() const;

  protected:
    QFrame* parentFrame;
    Preferences* preferences;
};

#endif
