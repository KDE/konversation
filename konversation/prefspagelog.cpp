/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagelog.cpp  -  Provides a user interface to customize logfile settings
  begin:     Son Okt 27 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qcheckbox.h>
#include <klineedit.h>
#include <klocale.h>

#include "prefspagelog.h"
#include "preferences.h"

PrefsPageLog::PrefsPageLog(QFrame* newParent,Preferences* newPreferences) :
              PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the Log Settings pane
  QGridLayout* logSettingsLayout=new QGridLayout(parentFrame,4,2,marginHint(),spacingHint(),"log_settings_layout");

  useLog=new QCheckBox(i18n("Enable logging"),parentFrame,"use_log_checkbox");
  lowerLog=new QCheckBox(i18n("Use lower case logfile names"),parentFrame,"lower_log_checkbox");
  logFollowsNick=new QCheckBox(i18n("Follow nick changes"),parentFrame,"follow_nickchanges_checkbox");

  QHBox* logPathBox=new QHBox(parentFrame);
  logPathLabel=new QLabel(i18n("Logfile path:"),logPathBox);
  logPathInput=new KLineEdit(preferences->getLogPath(),logPathBox,"log_path_input");

  useLog->setChecked(preferences->getLog());
  lowerLog->setChecked(preferences->getLowerLog());
  logFollowsNick->setChecked(preferences->getLogFollowsNick());

  updateLogWidgets(preferences->getLog());

  QHBox* logSpacer=new QHBox(parentFrame);

  int row=0;

  logSettingsLayout->addMultiCellWidget(useLog,row,row,0,1);
  row++;

  logSettingsLayout->addWidget(lowerLog,row,0);
  logSettingsLayout->addWidget(logFollowsNick,row,1);
  row++;

  logSettingsLayout->addMultiCellWidget(logPathBox,row,row,0,1);
  row++;

  logSettingsLayout->addMultiCellWidget(logSpacer,row,row,0,1);
  logSettingsLayout->setRowStretch(row,10);

  // Set up signals / slots for Log Setup page
  connect(useLog,SIGNAL (stateChanged(int)),this,SLOT (useLogChanged(int)) );
}

PrefsPageLog::~PrefsPageLog()
{
}

void PrefsPageLog::useLogChanged(int state)
{
  updateLogWidgets(state==2);
}

void PrefsPageLog::updateLogWidgets(bool enable)
{
  lowerLog->setEnabled(enable);
  logFollowsNick->setEnabled(enable);
  logPathLabel->setEnabled(enable);
  logPathInput->setEnabled(enable);
}

void PrefsPageLog::applyPreferences()
{
  preferences->setLog(useLog->isChecked());
  preferences->setLowerLog(lowerLog->isChecked());
  preferences->setLogFollowsNick(logFollowsNick->isChecked());
  preferences->setLogPath(logPathInput->text());
}

#include "prefspagelog.moc"
