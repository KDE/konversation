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
#include <qhbox.h>

#include <klocale.h>
#include <kiconloader.h>
#include <klineedit.h>

#include "preferences.h"
#include "valuelistviewitem.h"

PrefsPageChatWinBehavior::PrefsPageChatWinBehavior(QFrame* newParent, Preferences* newPreferences)
  : ChatwindowBehaviour_Config( newParent )
{
  preferences = newPreferences;

  kcfg_BeepOnAsciiBel->setChecked(preferences->getBeep());
  kcfg_HideJoinPart->setChecked(preferences->getHideUnimportantEvents());
  kcfg_DisableExpansions->setChecked(preferences->getDisableExpansion());
  kcfg_ShowRememberLine->setChecked(preferences->getShowRememberLineInAllWindows());
  kcfg_RedirectStatusMessages->setChecked(preferences->getRedirectToStatusPane());


  kcfg_ScrollBackLimit->setValue(preferences->getScrollbackMax());
  kcfg_AutoWhoLimit->setValue(preferences->getAutoWhoNicksLimit());

  kcfg_autoWhoContinuous->setChecked(preferences->getAutoWhoContinuousEnabled());
  kcfg_WhoInterval->setValue(preferences->getAutoWhoContinuousInterval());

  kcfg_DoubleClickCommand->setText(preferences->getChannelDoubleClickAction());
  kcfg_SortUserStatus->setChecked(preferences->getSortByStatus());
  kcfg_SortCaseInsensitive->setChecked(preferences->getSortCaseInsensitive());

  kcfg_SortOrder->addColumn("");
  kcfg_SortOrder->header()->hide();
  kcfg_SortOrder->setSorting(-1);
  kcfg_SortOrder->setMaximumHeight(kcfg_SortOrder->fontMetrics().height()*7);

  for(int index = 64; index != 0; index >>= 1)
  {
    if(preferences->getNoRightsValue() == index) new ValueListViewItem(0, kcfg_SortOrder, i18n("Normal Users"));
    if(preferences->getAwayValue() == index)     new ValueListViewItem(1, kcfg_SortOrder, i18n("Away Users"));
    if(preferences->getVoiceValue() == index)    new ValueListViewItem(2, kcfg_SortOrder, i18n("Voice (+v)"));
    if(preferences->getHalfopValue() == index)   new ValueListViewItem(3, kcfg_SortOrder, i18n("Halfops (+h)"));
    if(preferences->getOpValue() == index)       new ValueListViewItem(4, kcfg_SortOrder, i18n("Operators (+o)"));
    if(preferences->getOwnerValue() == index)    new ValueListViewItem(5, kcfg_SortOrder, i18n("Channel Owners"));
    if(preferences->getAdminValue() == index)    new ValueListViewItem(6, kcfg_SortOrder, i18n("Channel Admins"));
  }

  kcfg_UpButton->setIconSet(SmallIconSet("up"));
  kcfg_DownButton->setIconSet(SmallIconSet("down"));

  connect(kcfg_SortUserStatus,SIGNAL (stateChanged(int)),this,SLOT (sortByStatusChanged(int)) );
  connect(kcfg_UpButton,SIGNAL (clicked()),this,SLOT (moveUp()) );
  connect(kcfg_DownButton,SIGNAL (clicked()),this,SLOT (moveDown()) );

}

PrefsPageChatWinBehavior::~PrefsPageChatWinBehavior()
{
}

void PrefsPageChatWinBehavior::sortByStatusChanged(int state)
{
  kcfg_SortOrder->setEnabled(state == 2);
}

void PrefsPageChatWinBehavior::moveUp()
{
  QListViewItem* item = kcfg_SortOrder->selectedItem();

  if(item)
  {
    int pos = kcfg_SortOrder->itemIndex(item);
    if(pos) item->itemAbove()->moveItem(item);
  }
}

void PrefsPageChatWinBehavior::moveDown()
{
  QListViewItem* item = kcfg_SortOrder->selectedItem();

  if(item && item != kcfg_SortOrder->lastItem()) item->moveItem(item->itemBelow());
}

void PrefsPageChatWinBehavior::applyPreferences()
{
  preferences->setBeep(kcfg_BeepOnAsciiBel->isChecked());
  preferences->setHideUnimportantEvents(kcfg_HideJoinPart->isChecked());
  preferences->setDisableExpansion(kcfg_DisableExpansions->isChecked());
  preferences->setShowRememberLineInAllWindows(kcfg_ShowRememberLine->isChecked());
  preferences->setRedirectToStatusPane(kcfg_RedirectStatusMessages->isChecked());
  preferences->setScrollbackMax(kcfg_ScrollBackLimit->value());
  preferences->setAutoWhoNicksLimit(kcfg_AutoWhoLimit->value());
  preferences->setAutoWhoContinuousEnabled(kcfg_autoWhoContinuous->isChecked());
  preferences->setAutoWhoContinuousInterval(kcfg_WhoInterval->value());
  preferences->setChannelDoubleClickAction(kcfg_DoubleClickCommand->text());
  preferences->setSortByStatus(kcfg_SortUserStatus->isChecked());
  preferences->setSortCaseInsensitive(kcfg_SortCaseInsensitive->isChecked());

  int flag = 1;

  for(int index = 0; index < 7; index++)
  {
    ValueListViewItem* item = static_cast<ValueListViewItem*>(kcfg_SortOrder->itemAtIndex(index));
    int value = item->getValue();

    if(value == 0) preferences->setNoRightsValue(flag);
    else if(value == 1) preferences->setAwayValue(flag);
    else if(value == 2) preferences->setVoiceValue(flag);
    else if(value == 3) preferences->setHalfopValue(flag);
    else if(value == 4) preferences->setOpValue(flag);
    else if(value == 5) preferences->setOwnerValue(flag);
    else if(value == 6) preferences->setAdminValue(flag);

    flag <<= 1;
  }
}

#include "prefspagechatwinbehavior.moc"
