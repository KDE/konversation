/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagedialogs.cpp  -  Page to manage "do not show again" dialogs
  begin:     Don Mai 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlayout.h>

#include <klistview.h>
#include <klocale.h>
#include <kdebug.h>

#include "prefspagedialogs.h"
#include "preferences.h"

PrefsPageDialogs::PrefsPageDialogs(QFrame* newParent,Preferences* newPreferences) :
                  PrefsPage(newParent,newPreferences)
{
  QStringList dialogDefinitions;
  flagNames = "Invitation,SaveLogfileNote,ClearLogfileQuestion,CloseQueryAfterIgnore,ResumeTransfer,QuitServerTab,ChannelListNoServerSelected,HideMenuBarWarning,ChannelListWarning,LargePaste,RemoveDCCReceivedFile";

  dialogDefinitions.append(i18n("Automatically join channel on invite"));
  dialogDefinitions.append(i18n("Notice that saving logfiles will save whole file"));
  dialogDefinitions.append(i18n("Question before deleting logfile contents"));
  dialogDefinitions.append(i18n("Question on closing queries after ignoring the nickname"));
  dialogDefinitions.append(i18n("Question on what to do on DCC resume"));
  dialogDefinitions.append(i18n("Quit server when you hit the tab's close button"));
  dialogDefinitions.append(i18n("The channel list can only be opened from server-aware tabs"));
  dialogDefinitions.append(i18n("Warning on hiding the main window menu"));
  dialogDefinitions.append(i18n("Warning on high traffic with channel list"));
  dialogDefinitions.append(i18n("Warning on pasting large portions of text"));
  dialogDefinitions.append(i18n("Warning on deleting file received on DCC"));

  QVBoxLayout* dialogsLayout=new QVBoxLayout(parentFrame,marginHint(),spacingHint(),"dialogs_layout");

  dialogListView=new KListView(parentFrame,"dialog_list_view");
  dialogListView->addColumn(i18n("Select Warning Dialogs to Show"));
  dialogListView->setAllColumnsShowFocus(true);

  for(unsigned int index=0; index<11 ;index++)
  {
    item=new QCheckListItem(dialogListView,dialogDefinitions[index],QCheckListItem::CheckBox);
    item->setText(1,dialogDefinitions[index]);

    if(preferences->getDialogFlag(flagNames.section(",",index,index))) item->setOn(true);
  } // endfor

  dialogsLayout->addWidget(dialogListView);
}

PrefsPageDialogs::~PrefsPageDialogs()
{
  delete item;
}

void PrefsPageDialogs::applyPreferences()
{
  QCheckListItem* item=static_cast<QCheckListItem*>(dialogListView->itemAtIndex(0));
  int i=0;
  while(item)
  {
    preferences->setDialogFlag(flagNames.section(",",i,i),item->isOn());
    item=static_cast<QCheckListItem*>(item->itemBelow());
    ++i;
  }
}

#include "prefspagedialogs.moc"
