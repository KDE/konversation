/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  preferences.cpp  -  description
  begin:     Tue Feb 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <ktoolbar.h>
#include <kstddirs.h>

#include "preferences.h"
#include "prefsdialog.h"

Preferences::Preferences()
{
  /* Presets */
  serverWindowToolBarPos=KToolBar::Top;
  serverList.setAutoDelete(true);

  ident="dummy";
  realname="Arno Nym";

  nicknameList.append("KonvIRC");
  nicknameList.append("_KonvIRC");
  nicknameList.append("KonvIRC_");
  nicknameList.append("_KonvIRC_");

  addServer("FurNet,yiff.furry.de,6667,,#pantherchat,");
  addServer("FurNet,irc.critter.net,6667,,#wolfsrudel,");
  addServer("DALNet,irc.dal.net,6667,key,#linux,");
  addServer("IRCNet,irc.net,7000,,#kde,key");

  buttonList.append("Op,/OP %u%n");
  buttonList.append("DeOp,/DEOP %u%n");
  buttonList.append("WhoIs,/WHOIS %s,%%u%n");
  buttonList.append("Version,/CTCP %s,%%u VERSION%n");
  buttonList.append("Kick,/KICK %u%n");
  buttonList.append("Ban,/BAN %u%n");
  buttonList.append("Part,/PART %c :KDE Rules!%n");
  buttonList.append("Quit,/QUIT :KDE Rules!%n");

  partReason="Konversation terminated!";
  kickReason="User terminated!";

  hilightColor="0000ff";

  setHilightSize(QSize(260,235));
  setHilightSize(QSize(500,200));

  KStandardDirs kstddir;

  logPath=kstddir.saveLocation("data","konversation/logs");
  log=true;
  blinkingTabs=true;
  prefsDialog=0;
}

Preferences::~Preferences()
{
}

QString Preferences::getServerByIndex(unsigned int index)
{
  if(index>=serverList.count()) return 0;
  ServerEntry* entry=serverList.at(index);

  return entry->getDefinition();
}

QString Preferences::getServerById(int id)
{
  for(unsigned int index=0;index<serverList.count();index++)
  {
    ServerEntry* entry=serverList.at(index);
    if(entry->getId()==id) return entry->getDefinition();
  }
  return 0;
}

ServerEntry* Preferences::getServerEntryById(int id)
{
  for(unsigned int index=0;index<serverList.count();index++)
  {
    ServerEntry* entry=serverList.at(index);
    if(entry->getId()==id) return entry;
  }
  return 0;
}

int Preferences::getServerIdByIndex(unsigned int index)
{
  if(index>=serverList.count()) return -1;
  ServerEntry* entry=serverList.at(index);

  return entry->getId();
}

void Preferences::addServer(const QString& serverString)
{
  serverList.append(new ServerEntry(serverString));
}

QStringList& Preferences::getHilightList()
{
  return hilightList;
}

void Preferences::setHilightList(QStringList& newList)
{
  hilightList.clear();
  hilightList=newList;
}

void Preferences::setHilightColor(const QString& color)
{
  hilightColor=color;
}

QString Preferences::getHilightColor()
{
  return hilightColor;
}

void Preferences::addHilight(QString& newHilight)
{
  hilightList.append(newHilight);
}

void Preferences::setButtonList(QStringList newList)
{
  buttonList.clear();
  buttonList=newList;
}

void Preferences::changeServerProperty(int serverId,int property,const QString& value)
{
  ServerEntry* entry=getServerEntryById(serverId);
  if(entry) entry->updateProperty(property,value);
}

void Preferences::updateServer(int serverId,const QString& newDefinition)
{
  ServerEntry* entry=getServerEntryById(serverId);
  if(entry) entry->setDefinition(newDefinition);
}

void Preferences::clearServerList()
{
  serverList.clear();
}

void Preferences::openPrefsDialog()
{
  if(prefsDialog==0)
  {
    prefsDialog=new PrefsDialog(this,false);

    connect(prefsDialog,SIGNAL (connectToServer(int)),this,SLOT (connectToServer(int)) );
    connect(prefsDialog,SIGNAL (cancelClicked()),this,SLOT (closePrefsDialog()) );
    connect(prefsDialog,SIGNAL (closed()),this,SLOT (clearPrefsDialog()) );
    connect(prefsDialog,SIGNAL (prefsChanged()),this,SLOT (saveOptions()) );

    serverList.setAutoDelete(true);     // delete items when they are removed

    prefsDialog->show();
  }
}

void Preferences::connectToServer(int number)
{
  emit requestServerConnection(number);
}

void Preferences::saveOptions()
{
  emit requestSaveOptions();
}

void Preferences::closePrefsDialog()
{
  delete prefsDialog;
  clearPrefsDialog();
}

void Preferences::clearPrefsDialog()
{
  prefsDialog=0;
}
