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

#include "prefspagedialogs.h"
#include "preferences.h"

PrefsPageDialogs::PrefsPageDialogs(QFrame* newParent,Preferences* newPreferences) :
                  PrefsPage(newParent,newPreferences)
{
  QStringList dialogDefinitions;
  dialogDefinitions.append("HideMenuBarWarning "         +i18n("Warning on hiding the main window menu"));
  dialogDefinitions.append("LargePaste "                 +i18n("Warning on pasting large portions of text"));
  dialogDefinitions.append("ResumeTransfer "             +i18n("Question on what to do on DCC resume"));
  dialogDefinitions.append("Invitation "                 +i18n("Automatically join channel on invite"));
  dialogDefinitions.append("ChannelListWarning "         +i18n("Warning on high traffic with channel list"));
  dialogDefinitions.append("ChannelListNoServerSelected "+i18n("The channel list can only be opened from server-aware tabs"));
  dialogDefinitions.append("QuitServerTab "              +i18n("Quit server when you hit the tab's close button"));

  QVBoxLayout* dialogsLayout=new QVBoxLayout(parentFrame,marginHint(),spacingHint(),"dialogs_layout");

  dialogListView=new KListView(parentFrame,"dialog_list_view");

  dialogListView->addColumn(i18n("Show"));
  dialogListView->addColumn(i18n("Name"));
  dialogListView->addColumn(i18n("Description"));

  dialogListView->setAllColumnsShowFocus(true);

  for(unsigned int index=0;index<dialogDefinitions.count();index++)
  {
    QString flagName(dialogDefinitions[index].section(' ',0,0));
    item=new QCheckListItem(dialogListView,flagName,QCheckListItem::CheckBox);
    item->setText(1,dialogDefinitions[index].section(' ',1));

    if(preferences->getDialogFlag(flagName)) item->setOn(true);
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
  while(item)
  {
    preferences->setDialogFlag(item->text(0),item->isOn());
    item=static_cast<QCheckListItem*>(item->itemBelow());
  }
}

#include "prefspagedialogs.moc"
