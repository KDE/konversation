/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides a user interface to customize logfile settings
  begin:     Son Okt 27 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGELOG_H
#define PREFSPAGELOG_H

#include "log_preferences.h"
class Preferences;

/*
 *@author Dario Abatianni
*/

class PrefsPageLog : public Log_Config
{
  Q_OBJECT

  public:
    PrefsPageLog(QFrame* newParent,Preferences* newPreferences);

  public slots:
    void applyPreferences();
    
protected:
    Preferences* preferences;

};

#endif
