/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagetabbehavior.cpp  -  Provides a GUI for tab behavior
  begin:     Sun Nov 16 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>

#include "prefspagetabbehavior.h"
#include "preferences.h"

PrefsPageTabBehavior::PrefsPageTabBehavior(QFrame* newParent,Preferences* newPreferences) :
                      PrefsPage(newParent,newPreferences)
{
  QGridLayout* tabBehaviorLayout=new QGridLayout(parentFrame,2,2,marginHint(),spacingHint(),"tabbehavior_layout");

  // close buttons on tabs
  closeButtonsCheck=new QCheckBox(i18n("Show close &widgets on tabs"),parentFrame,"tab_close_widgets_check");
  closeButtonsCheck->setChecked(preferences->getCloseButtonsOnTabs());

  // Display close buttons on which side
  closeButtonsAlignRight=new QCheckBox(i18n("Place close widgets on the &right side"),parentFrame,"tab_close_widgets_align_right");
  closeButtonsAlignRight->setChecked(preferences->getCloseButtonsAlignRight());

  // Take care of ghosting / unghosting close button checkboxes
  closeButtonsChanged(preferences->getCloseButtonsOnTabs() ? 2 : 0);

  int row=0;
  tabBehaviorLayout->addWidget(closeButtonsCheck,row,0);
  tabBehaviorLayout->addWidget(closeButtonsAlignRight,row,1);

  connect(closeButtonsCheck,SIGNAL (stateChanged(int)),this,SLOT (closeButtonsChanged(int)) );
}

PrefsPageTabBehavior::~PrefsPageTabBehavior()
{
}

void PrefsPageTabBehavior::closeButtonsChanged(int state)
{
  closeButtonsCheck->setChecked(state);
  closeButtonsAlignRight->setEnabled(state==2);
}

void PrefsPageTabBehavior::applyPreferences()
{
  preferences->setCloseButtonsOnTabs(closeButtonsCheck->isChecked());
  preferences->setCloseButtonsAlignRight(closeButtonsAlignRight->isChecked());
}

#include "prefspagetabbehavior.moc"
