/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagedialogs.cpp  -  description
  begin:     Don Mai 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>

#include <klistview.h>

#include "serverlistitem.h"
#include "prefspagedialogs.h"

PrefsPageDialogs::PrefsPageDialogs(QFrame* newParent,Preferences* newPreferences) :
                  PrefsPage(newParent,newPreferences)
{
  QStringList dialogDefinitions;
  dialogDefinitions.append("HideMenuBarWarning "       +i18n("Warning on hiding the main window menu"));
  dialogDefinitions.append("LargePaste "               +i18n("Warning on pasting large portions of text"));
  dialogDefinitions.append("DuplicateHighlightWarning "+i18n("Warning on inserting a duplicate highlight pattern"));
  dialogDefinitions.append("ResumeTransfer "           +i18n("Question on what to do on DCC resume"));
  dialogDefinitions.append("Invitation "               +i18n("Automatically join channel on invite"));
  dialogDefinitions.append("ChannelListWarning "       +i18n("Warning on high traffic with channel list"));
  dialogDefinitions.append("QuitServerOnTabClose "     +i18n("Quit server when you hit the tab's close button"));

  QVBoxLayout* dialogsLayout=new QVBoxLayout(parentFrame,marginHint(),spacingHint(),"dialogs_layout");

  KListView* dialogListView=new KListView(parentFrame,"dialog_list_view");

  dialogListView->addColumn(i18n("Show"));
  dialogListView->addColumn(i18n("Name"));
  dialogListView->addColumn(i18n("Description"));

  dialogListView->setAllColumnsShowFocus(true);

  for(unsigned int index=0;index<dialogDefinitions.count();index++)
  {
    QString flagName(dialogDefinitions[index].section(' ',0,0));
    ServerListItem* item=new ServerListItem(dialogListView,index,flagName,dialogDefinitions[index].section(' ',1));

    connect(item,SIGNAL(stateChanged(ServerListItem*,bool)),
            this,SLOT  (stateChanged(ServerListItem*,bool)) );

    if(preferences->getDialogFlag(flagName)) item->setOn(true);
  } // for

  dialogsLayout->addWidget(dialogListView);
}

PrefsPageDialogs::~PrefsPageDialogs()
{
}

void PrefsPageDialogs::stateChanged(ServerListItem* item,bool state)
{
  preferences->setDialogFlag(item->text(1),state);
}

#include "prefspagedialogs.moc"
