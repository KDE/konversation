/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagelog.h  -  description
  begin:     Son Okt 27 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef PREFSPAGELOG_H
#define PREFSPAGELOG_H

#include <qcheckbox.h>

#include <klineedit.h>

#include "prefspage.h"

/*
 *@author Dario Abatianni
*/

class PrefsPageLog : public PrefsPage
{
  Q_OBJECT

  public: 
    PrefsPageLog(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageLog();

  protected slots:
    void useLogChanged(int state);
    void lowerLogChanged(int state);
    void logFollowsNickChanged(int state);
    void logPathInputChanged(const QString& path);

  protected:
    QCheckBox* useLog;
    QCheckBox* lowerLog;
    QCheckBox* logFollowsNick;
    KLineEdit* logPathInput;

};

#endif
