/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagebehaviour.h  -  The preferences panel that holds the behaviour settings
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#include "prefspagechatwinbehavior.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qheader.h>
#include <qvgroupbox.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qspinbox.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kiconloader.h>

#include "preferences.h"
#include "valuelistviewitem.h"

PrefsPageChatWinBehavior::PrefsPageChatWinBehavior(QFrame* newParent, Preferences* newPreferences)
 : PrefsPage(newParent, newPreferences)
{
  QGridLayout* chatLayout = new QGridLayout(parentFrame, 4, 3, marginHint(), spacingHint());
  
  beepCheck = new QCheckBox(i18n("Bee&p on incoming ASCII BEL"), parentFrame, "beep_check");
  beepCheck->setChecked(preferences->getBeep());

  hideUnimportantCheck = new QCheckBox(i18n("&Hide Join/Part/Nick events"), parentFrame, "hide_unimportant_check");
  hideUnimportantCheck->setChecked(preferences->getHideUnimportantEvents());
  
  disableExpansionCheck = new QCheckBox(i18n("&Disable %C, %B, %G, etc. expansion"), parentFrame, "disable_expansion_check");
  disableExpansionCheck->setChecked(preferences->getDisableExpansion());
    
  showRememberLineInAllWindows = new QCheckBox(i18n("Show remember line in all channels/queries"), parentFrame,
   "show_remember_line_in_all_windows");
  showRememberLineInAllWindows->setChecked(preferences->getShowRememberLineInAllWindows());

  redirectToStatusPaneCheck = new QCheckBox(i18n("Redirect all status messages to the server status window"), parentFrame,
    "redirect_to_status_page_check");
  redirectToStatusPaneCheck->setChecked(preferences->getRedirectToStatusPane());

  QLabel* scrollbackMaxLabel = new QLabel(i18n("&Scrollback limit:"), parentFrame);
  scrollbackMaxSpin = new QSpinBox(0, 100000, 50, parentFrame, "scrollback_max_spin");
  scrollbackMaxSpin->setValue(preferences->getScrollbackMax());
  scrollbackMaxSpin->setSuffix(" " + i18n("lines"));
  scrollbackMaxSpin->setSpecialValueText(i18n("Unlimited"));
  scrollbackMaxLabel->setBuddy(scrollbackMaxSpin);
  QWhatsThis::add(scrollbackMaxSpin,i18n("How many lines to keep in buffers; 0=all (Unlimited)"));
  QToolTip::add(scrollbackMaxSpin,i18n("How many lines to keep in buffers; 0=all (Unlimited)"));
  QToolTip::add(scrollbackMaxLabel,i18n("How many lines to keep in buffers; 0=all (Unlimited)"));
    
  QVGroupBox* sortOptionsGroup=new QVGroupBox(i18n("Nickname List Sorting"), parentFrame, "sort_options_group");

  sortCaseInsensitiveCheck = new QCheckBox(i18n("Case &insensitive"),sortOptionsGroup,"sort_case_insensitive_check");
  sortByStatusCheck = new QCheckBox(i18n("By us&er status"),sortOptionsGroup,"sort_by_status_check");

  sortByStatusCheck->setChecked(preferences->getSortByStatus());
  sortCaseInsensitiveCheck->setChecked(preferences->getSortCaseInsensitive());
  
  sortOrderGroup = new QWidget(sortOptionsGroup, "sorting_order_widget");
  sortOrderGroup->setEnabled(preferences->getSortByStatus());

  sortingOrder = new KListView(sortOrderGroup,"sorting_order_view");
  sortingOrder->addColumn("");
  sortingOrder->setFullWidth(true);
  sortingOrder->header()->hide();
  sortingOrder->setSorting(-1);
  sortingOrder->setDragEnabled(true);
  sortingOrder->setAcceptDrops(true);
  sortingOrder->setMaximumHeight(sortingOrder->fontMetrics().height()*7);

  for(int index = 32; index != 0; index >>= 1)
  {
    if(preferences->getNoRightsValue() == index) new ValueListViewItem(0, sortingOrder, i18n("Normal Users"));
    if(preferences->getVoiceValue() == index)    new ValueListViewItem(1, sortingOrder, i18n("Voice (+v)"));
    if(preferences->getHalfopValue() == index)   new ValueListViewItem(2, sortingOrder, i18n("Halfops (+h)"));
    if(preferences->getOpValue() == index)       new ValueListViewItem(3, sortingOrder, i18n("Operators (+o)"));
    if(preferences->getOwnerValue() == index)    new ValueListViewItem(4, sortingOrder, i18n("Channel Owners"));
    if(preferences->getAdminValue() == index)    new ValueListViewItem(5, sortingOrder, i18n("Channel Admins"));
  }

  QToolButton* sortMoveUp=new QToolButton(sortOrderGroup,"sort_move_up_button");
  sortMoveUp->setIconSet(SmallIconSet("up"));
  sortMoveUp->setAutoRepeat(true);
  QToolButton* sortMoveDown=new QToolButton(sortOrderGroup,"sort_move_up_button");
  sortMoveDown->setIconSet(SmallIconSet("down"));
  sortMoveDown->setAutoRepeat(true);
  
  connect(sortByStatusCheck,SIGNAL (stateChanged(int)),this,SLOT (sortByStatusChanged(int)) );
  connect(sortMoveUp,SIGNAL (clicked()),this,SLOT (moveUp()) );
  connect(sortMoveDown,SIGNAL (clicked()),this,SLOT (moveDown()) );
  
  QGridLayout* sortOrderLayout = new QGridLayout(sortOrderGroup,4,2,0,spacingHint());
  sortOrderLayout->addMultiCellWidget(sortingOrder, 0, 3, 0, 0);
  sortOrderLayout->addWidget(sortMoveUp, 1, 1);
  sortOrderLayout->addWidget(sortMoveDown, 2, 1);
  sortOrderLayout->setRowStretch(0, 10);
  sortOrderLayout->setRowStretch(3, 10);
    
  int row = 0;
  chatLayout->addMultiCellWidget(beepCheck, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(hideUnimportantCheck, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(disableExpansionCheck, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(showRememberLineInAllWindows, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(redirectToStatusPaneCheck, row, row, 0, 2);
  row++;
  chatLayout->addWidget(scrollbackMaxLabel, row, 0);
  chatLayout->addWidget(scrollbackMaxSpin, row, 1);
  row++;
  chatLayout->addMultiCellWidget(sortOptionsGroup, row, row, 0, 2);
  row++;
  chatLayout->setRowStretch(row, 10);
  chatLayout->setColStretch(2, 10);
}

PrefsPageChatWinBehavior::~PrefsPageChatWinBehavior()
{
}

void PrefsPageChatWinBehavior::sortByStatusChanged(int state)
{
  sortOrderGroup->setEnabled(state == 2);
}

void PrefsPageChatWinBehavior::moveUp()
{
  QListViewItem* item = sortingOrder->selectedItem();
  
  if(item)
  {
    int pos = sortingOrder->itemIndex(item);
    if(pos) item->itemAbove()->moveItem(item);
  }
}

void PrefsPageChatWinBehavior::moveDown()
{
  QListViewItem* item = sortingOrder->selectedItem();
  
  if(item)
  {
    int pos = sortingOrder->itemIndex(item);
    if(pos != 2) item->moveItem(item->itemBelow());
  }
}

void PrefsPageChatWinBehavior::applyPreferences()
{
  preferences->setBeep(beepCheck->isChecked());
  preferences->setHideUnimportantEvents(hideUnimportantCheck->isChecked());
  preferences->setDisableExpansion(disableExpansionCheck->isChecked());
  preferences->setShowRememberLineInAllWindows(showRememberLineInAllWindows->isChecked());
  preferences->setRedirectToStatusPane(redirectToStatusPaneCheck->isChecked());
  preferences->setScrollbackMax(scrollbackMaxSpin->value());
  preferences->setSortByStatus(sortByStatusCheck->isChecked());
  preferences->setSortCaseInsensitive(sortCaseInsensitiveCheck->isChecked());

  int flag = 1;

  for(int index = 0; index < 3; index++)
  {
    ValueListViewItem* item = static_cast<ValueListViewItem*>(sortingOrder->itemAtIndex(index));
    int value = item->getValue();

    if(value == 0) preferences->setNoRightsValue(flag);
    else if(value == 1) preferences->setVoiceValue(flag);
    else if(value == 2) preferences->setHalfopValue(flag);
    else if(value == 3) preferences->setOpValue(flag);
    else if(value == 4) preferences->setOwnerValue(flag);
    else if(value == 5) preferences->setAdminValue(flag);

    flag <<= 1;
  }
}

#include "prefspagechatwinbehavior.moc"
