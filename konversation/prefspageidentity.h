/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageidentity.h  -  description
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef PREFSPAGEIDENTITY_H
#define PREFSPAGEIDENTITY_H

#include <prefspage.h>

/*
  @author Dario Abatianni
*/

class PrefsPageIdentity : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageIdentity(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageIdentity();

  protected slots:
    void realNameChanged(const QString& newRealName);
    void loginChanged(const QString& newlogin);
    void nick0Changed(const QString& newNick);
    void nick1Changed(const QString& newNick);
    void nick2Changed(const QString& newNick);
    void nick3Changed(const QString& newNick);
};

#endif
