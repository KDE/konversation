/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagewebbrowser.cpp    -  Configuration tab for Web Browser
  begin:     Thu Jan 8 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qvbuttongroup.h>

#include <klocale.h>

#include "prefspagewebbrowser.h"
#include "preferences.h"

PrefsPageWebBrowser::PrefsPageWebBrowser(QFrame* newParent,Preferences* newPreferences) :
                 PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the Web Browser pane
  QGridLayout* wbLayout=new QGridLayout(parentFrame,1,1,marginHint(),spacingHint(),"wb_settings_layout");

  // Set up web browser widgets
  QVButtonGroup* wbActionsBox = new QVButtonGroup(i18n("When URL Is Clicked"), parentFrame, "wb_actions_group");
  wbActionsBox->setRadioButtonExclusive(true);
  wbUseKdeDefault = new QRadioButton(i18n("Use default KDE web browser"), wbActionsBox, "wb_use_kde_default");
  QHBox* wbCustomBox = new QHBox(wbActionsBox);
  wbUseCustomCmd = new QRadioButton(i18n("Use this command:"), wbCustomBox, "wb_use_custom_cmd");
  wbCustomCmd = new KLineEdit(preferences->getWebBrowserCmd(),wbCustomBox, "wb_custom_cmd");
  wbActionsBox->insert(wbUseCustomCmd);
  if (preferences->getWebBrowserUseKdeDefault())
  {
    wbActionsBox -> setButton(wbActionsBox->id(wbUseKdeDefault));
    wbCustomCmd->setEnabled(false);
  }
  else
  {
    wbActionsBox -> setButton(wbActionsBox->id(wbUseCustomCmd));
    wbCustomCmd->setEnabled(true);
  }
  
  // Define the layout
  // Position radio button group at top of frame, as wide as the frame.
  wbLayout->addWidget(wbActionsBox, 0, 0, Qt::AlignTop);

  connect(wbActionsBox,SIGNAL (clicked(int)),this,SLOT (wbActionsBoxClickedSlot()) );
}

PrefsPageWebBrowser::~PrefsPageWebBrowser()
{
}

void PrefsPageWebBrowser::wbActionsBoxClickedSlot()
{
  wbCustomCmd->setEnabled(wbUseCustomCmd->isOn());
}

void PrefsPageWebBrowser::applyPreferences()
{
  preferences->setWebBrowserUseKdeDefault(wbUseKdeDefault->isOn());
  preferences->setWebBrowserCmd(wbCustomCmd->text());
}

#include "prefspagewebbrowser.moc"
