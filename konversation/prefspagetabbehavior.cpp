/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides a GUI for tab behavior
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
  QGridLayout* tabsLayout = new QGridLayout(parentFrame, 4, 2, marginHint(), spacingHint());

  closeButtonsCheck = new QCheckBox(i18n("Show close b&utton on tabs"), parentFrame, "tab_close_widgets_check");
  closeButtonsCheck->setChecked(preferences->getCloseButtonsOnTabs());
  closeButtonsAlignRight = new QCheckBox(i18n("Place close button on the &right side of the tab"),
    parentFrame, "tab_close_widgets_align_right");
  closeButtonsAlignRight->setChecked(preferences->getCloseButtonsAlignRight());
  connect(closeButtonsCheck, SIGNAL(stateChanged(int)), this, SLOT(closeButtonsChanged(int)));
  closeButtonsChanged(preferences->getCloseButtonsOnTabs() ? 2 : 0);

  tabPlacementCheck = new QCheckBox(i18n("Place tab labels on &top"), parentFrame, "tab_placement_check");
  tabPlacementCheck->setChecked(preferences->getTabPlacement() == Preferences::Top);
  blinkingTabsCheck = new QCheckBox(i18n("&Blinking tabs"), parentFrame, "blinking_tabs_check");
  blinkingTabsCheck->setChecked(preferences->getBlinkingTabs());
  bringToFrontCheck = new QCheckBox(i18n("Bring new tabs to &front"), parentFrame, "bring_to_front_check");
  bringToFrontCheck->setChecked(preferences->getBringToFront());

  focusNewQueries = new QCheckBox(i18n("When someone queries you, focus new &query"), parentFrame, "focus_new_queries");
  focusNewQueries->setChecked(preferences->getFocusNewQueries());
  connect(bringToFrontCheck, SIGNAL(stateChanged(int)), this, SLOT(bringToFrontCheckChanged(int)));

  
  tabBarCloseButtonCheck = new QCheckBox(i18n("&Show a close tab button to the right in the tab bar"),
    parentFrame, "tab_bar_close_button");
  tabBarCloseButtonCheck->setChecked(preferences->getShowTabBarCloseButton());

  int row = 0;
  tabsLayout->addMultiCellWidget(closeButtonsCheck, row, row, 0, 1);
  row++;
  tabsLayout->addMultiCellWidget(closeButtonsAlignRight, row, row, 0, 1);
  row++;
  tabsLayout->addMultiCellWidget(tabPlacementCheck, row, row, 0, 1);
  row++;
  tabsLayout->addMultiCellWidget(blinkingTabsCheck, row, row, 0, 1);
  row++;
  tabsLayout->addMultiCellWidget(bringToFrontCheck, row, row, 0, 1);
  row++;
  tabsLayout->addMultiCellWidget(focusNewQueries, row, row, 0, 2);
  row++;
  tabsLayout->addMultiCellWidget(tabBarCloseButtonCheck, row, row, 0, 1);
  row++;
  tabsLayout->setRowStretch(row, 10);
}

PrefsPageTabBehavior::~PrefsPageTabBehavior()
{
}

void PrefsPageTabBehavior::closeButtonsChanged(int state)
{
  closeButtonsCheck->setChecked(state);
  closeButtonsAlignRight->setEnabled(state==2);
}

void PrefsPageTabBehavior::bringToFrontCheckChanged(int state)
{
  bringToFrontCheck->setChecked(state);
  focusNewQueries->setEnabled(state==2);
}

void PrefsPageTabBehavior::applyPreferences()
{
  preferences->setTabPlacement(tabPlacementCheck->isChecked() ? Preferences::Top : Preferences::Bottom);
  preferences->setBlinkingTabs(blinkingTabsCheck->isChecked());
  preferences->setBringToFront(bringToFrontCheck->isChecked());
  preferences->setFocusNewQueries(focusNewQueries->isChecked());
  preferences->setCloseButtonsOnTabs(closeButtonsCheck->isChecked());
  preferences->setCloseButtonsAlignRight(closeButtonsAlignRight->isChecked());
  preferences->setShowTabBarCloseButton(tabBarCloseButtonCheck->isChecked());
}

#include "prefspagetabbehavior.moc"
