/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagelog.h  -  Provides a user interface to customize logfile settings
  begin:     Son Okt 27 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGELOG_H
#define PREFSPAGELOG_H

#include "prefspage.h"

/*
 *@author Dario Abatianni
*/

class QCheckBox;
class QLabel;
class KLineEdit;

class PrefsPageLog : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageLog(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageLog();

  public slots:
    void applyPreferences();

  protected slots:
    void useLogChanged(int state);

  protected:
    void updateLogWidgets(bool state);

    QCheckBox* useLog;
    QCheckBox* lowerLog;
    QCheckBox* logFollowsNick;
    QLabel* logPathLabel;
    KLineEdit* logPathInput;

};

#endif
