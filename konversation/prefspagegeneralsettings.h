/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagegeneralsettings.h  -  description
  begin:     Fre Nov 15 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef PREFSPAGEGENERALSETTINGS_H
#define PREFSPAGEGENERALSETTINGS_H

#include <prefspage.h>

/*
  @author Dario Abatianni
*/

class PrefsPageGeneralSettings : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageGeneralSettings(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageGeneralSettings();

  protected slots:
    void commandCharChanged(const QString& newChar);
    void suffixStartChanged(const QString& newSuffix);
    void suffixMiddleChanged(const QString& newSuffix);
    void autoReconnectChanged(int state);
    void autoRejoinChanged(int state);
    void blinkingTabsChanged(int state);
    void fixedMOTDChanged(int state);
};

#endif
