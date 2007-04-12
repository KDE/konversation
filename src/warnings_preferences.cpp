/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/


#include "warnings_preferences.h"
#include "konviconfigdialog.h"

#include <qlistview.h>

#include <kdebug.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <klistview.h>


Warnings_Config::Warnings_Config( QWidget* parent, const char* name, WFlags fl )
    : Warnings_ConfigUI( parent, name, fl )
{
  dialogListView->setSorting(1);
  loadSettings();
  connect(dialogListView, SIGNAL(clicked(QListViewItem *)), this, SIGNAL(modified()));
}

Warnings_Config::~Warnings_Config()
{
}

void Warnings_Config::restorePageToDefaults()
{
  
  QCheckListItem* item=static_cast<QCheckListItem*>(dialogListView->itemAtIndex(0));
  bool changed=false;
  while(item)
  {
    if(!item->isOn()) {
      item->setOn(true);
      changed=true;
    }
    item=static_cast<QCheckListItem*>(item->itemBelow());
  }
  if(changed) {
    emit modified();
  }
}

void Warnings_Config::saveSettings()
{
  KConfig* config = kapp->config();
  config->setGroup("Notification Messages");

  // prepare list
  QString warningsChecked;

  QCheckListItem* item=static_cast<QCheckListItem*>(dialogListView->itemAtIndex(0));
  int i = 0;
  while(item)
  {
    // save state of this item in hasChanged() list
    warningsChecked+=item->isOn();

    if (item->text(2) == "LargePaste")
    {
        if (item->isOn())
        {
            config->writeEntry(item->text(2), 1);
        }
        else
        {
            QString state = config->readEntry(item->text(2));

            if (!state.isEmpty() && (state == "yes" || state == "no"))
                config->writeEntry(item->text(2), state);
            else
                config->writeEntry(item->text(2), "yes");
        }
    }
    else
    {
        config->writeEntry(item->text(2),item->isOn() ? "1" : "0");
    }

    item=static_cast<QCheckListItem*>(item->itemBelow());
    ++i;
  }

  // remember checkbox state for hasChanged()
  m_oldWarningsChecked=warningsChecked;
}

void Warnings_Config::loadSettings()
{
  QStringList dialogDefinitions;
  QString flagNames = "Invitation,SaveLogfileNote,ClearLogfileQuestion,CloseQueryAfterIgnore,ReconnectDifferentServer,QuitServerTab,QuitChannelTab,QuitQueryTab,ChannelListNoServerSelected,RemoveDCCReceivedFile,HideMenuBarWarning,ChannelListWarning,LargePaste,systemtrayquitKonversation,IgnoreNick,UnignoreNick";
  dialogDefinitions.append(i18n("Automatically join channel on invite"));
  dialogDefinitions.append(i18n("Notice that saving logfiles will save whole file"));
  dialogDefinitions.append(i18n("Ask before deleting logfile contents"));
  dialogDefinitions.append(i18n("Ask about closing queries after ignoring the nickname"));
  dialogDefinitions.append(i18n("Ask before connecting to a different server in the network"));
  dialogDefinitions.append(i18n("Close server tab"));
  dialogDefinitions.append(i18n("Close channel tab"));
  dialogDefinitions.append(i18n("Close query tab"));
  dialogDefinitions.append(i18n("The channel list can only be opened from server-aware tabs"));
  dialogDefinitions.append(i18n("Warning on deleting file received on DCC"));
  dialogDefinitions.append(i18n("Warning on hiding the main window menu"));
  dialogDefinitions.append(i18n("Warning on high traffic with channel list"));
  dialogDefinitions.append(i18n("Warning on pasting large portions of text"));
  dialogDefinitions.append(i18n("Warning on quitting Konversation"));
  dialogDefinitions.append(i18n("Ignore"));
  dialogDefinitions.append(i18n("Unignore"));
  QCheckListItem *item;
  dialogListView->clear();

  KConfig* config = kapp->config();
  config->setGroup("Notification Messages");
  QString flagName; 
  for(unsigned int i=0; i<dialogDefinitions.count() ;i++)
  {
    item=new QCheckListItem(dialogListView,dialogDefinitions[i],QCheckListItem::CheckBox);
    item->setText(1,dialogDefinitions[i]);
    flagName = flagNames.section(",",i,i);
    item->setText(2,flagName);

    if (flagName == "LargePaste")
    {
        QString state = config->readEntry(flagName);

        if (state == "yes" || state == "no")
            item->setOn(false);
        else
            item->setOn(true);
    }
    else
    {
        item->setOn(config->readBoolEntry(flagName,true));
    }
  }
  // remember checkbox state for hasChanged()
  m_oldWarningsChecked=currentWarningsChecked();
}

// get a list of checked/unchecked items for hasChanged()
QString Warnings_Config::currentWarningsChecked()
{
  // prepare list
  QString newList;

  // get first checklist item
  QListViewItem* item=dialogListView->firstChild();
  while(item)
  {
    // save state of this item in hasChanged() list
    newList+=(static_cast<QCheckListItem*>(item)->isOn()) ? "1" : "0";
    item=item->itemBelow();
  }
  // return list
  return newList;
}

bool Warnings_Config::hasChanged()
{
  return(m_oldWarningsChecked!=currentWarningsChecked());
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void Warnings_Config::languageChange()
{
  loadSettings();
}

#include "warnings_preferences.moc"
