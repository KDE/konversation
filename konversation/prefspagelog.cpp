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

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <kurlrequester.h>

#include "prefspagelog.h"
#include "preferences.h"

PrefsPageLog::PrefsPageLog(QFrame* newParent,Preferences* newPreferences) :
              Log_Config(newParent)
{
	preferences = newPreferences;

  kcfg_Log->setChecked(preferences->getLog());
  
  kcfg_LogfilePath->setURL(preferences->getLogPath());
	kcfg_LogfilePath->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );

  kcfg_LowerLog->setChecked(preferences->getLowerLog());
  kcfg_AddHostnameToLog->setChecked(preferences->getAddHostnameToLog());
  kcfg_LogFollowsNick->setChecked(preferences->getLogFollowsNick());
}

void PrefsPageLog::applyPreferences()
{
  preferences->setLog(kcfg_Log->isChecked());
  preferences->setLowerLog(kcfg_LowerLog->isChecked());
  preferences->setAddHostnameToLog(kcfg_AddHostnameToLog->isChecked());
  preferences->setLogFollowsNick(kcfg_LogFollowsNick->isChecked());
  preferences->setLogPath(kcfg_LogfilePath->url());
}

#include "prefspagelog.moc"
