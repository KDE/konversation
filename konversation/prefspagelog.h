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

#include "prefspage.h"

/*
 *@author Dario Abatianni
*/

class QCheckBox;
class QLabel;
class KURLRequester;
class QSpinBox;
class QGroupBox;
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
    void selectLogPath();

  protected:
    QGroupBox* loggingBox;
    QCheckBox* lowerLog;
    QCheckBox* logFollowsNick;
    QLabel* logPathLabel;
    KLineEdit* logPathInput;
};

#endif
