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
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qtoolbutton.h>

#include <klineedit.h>
#include <klocale.h>
#include <kdirselectdialog.h>
#include <kiconloader.h>

#include "prefspagelog.h"
#include "preferences.h"

PrefsPageLog::PrefsPageLog(QFrame* newParent,Preferences* newPreferences) :
              PrefsPage(newParent,newPreferences)
{

  QSpacerItem *spacey=0;

  // Add a Layout to the Log Settings pane
  QVBoxLayout *outer=new QVBoxLayout(parentFrame);
  outer->setSpacing(spacingHint());

  loggingBox = new QGroupBox(i18n("Enable &Logging"),parentFrame);
  loggingBox->setColumnLayout(0, Qt::Vertical);
  loggingBox->setMargin(marginHint());
  loggingBox->setCheckable(TRUE);
  loggingBox->setChecked(preferences->getLog());
  loggingBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

  QGridLayout* logSettingsLayout=new QGridLayout(loggingBox->layout(),4,2,spacingHint(),"log_settings_layout");

  lowerLog=new QCheckBox(i18n("&Use lower case logfile names"),loggingBox,"lower_log_checkbox");
  logFollowsNick=new QCheckBox(i18n("&Follow nick changes"),loggingBox,"follow_nickchanges_checkbox");

  QHBox* logPathBox = new QHBox(loggingBox);
  logPathBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
  logPathBox->setSpacing(spacingHint());
  logPathLabel = new QLabel(i18n("Logfile &path:"), logPathBox);
  logPathInput = new KLineEdit(preferences->getLogPath(), logPathBox, "logPathInput");
  logPathLabel->setBuddy(logPathInput);
  QToolButton* logPathBtn = new QToolButton(logPathBox, "logPathBtn");
  logPathBtn->setIconSet(SmallIconSet("folder"));
  connect(logPathBtn, SIGNAL(clicked()), this, SLOT(selectLogPath()));

  lowerLog->setChecked(preferences->getLowerLog());
  logFollowsNick->setChecked(preferences->getLogFollowsNick());

  int row=0;

  logSettingsLayout->addWidget(lowerLog,row,0);
  logSettingsLayout->addWidget(logFollowsNick,row,1);
  row++;

  logSettingsLayout->addMultiCellWidget(logPathBox,row,row,0,1);
  row++;

  outer->addWidget(loggingBox);
  spacey=new QSpacerItem(spacingHint(),spacingHint());
  outer->addItem(spacey);

}

PrefsPageLog::~PrefsPageLog()
{
}

void PrefsPageLog::applyPreferences()
{
  preferences->setLog(loggingBox->isChecked());
  preferences->setLowerLog(lowerLog->isChecked());
  preferences->setLogFollowsNick(logFollowsNick->isChecked());
  preferences->setLogPath(logPathInput->text());
}

void PrefsPageLog::selectLogPath()
{
  KURL url = KDirSelectDialog::selectDirectory(logPathInput->text(), false, parentFrame);

  if(!url.isEmpty()) {
    logPathInput->setText(url.path());
  }
}

#include "prefspagelog.moc"
