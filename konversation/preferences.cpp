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

  $Id$
*/

#include <ktoolbar.h>
#include <kstddirs.h>

#include "preferences.h"
#include "prefsdialog.h"

Preferences::Preferences()
{
  /* Presets */
  serverWindowToolBarPos=KToolBar::Top;
  serverWindowStatusBarStatus=true;
  serverList.setAutoDelete(true);

  ident="konversation";
  realname="Konversation User";

	defaultChannelMessageColor = "000000";
  defaultQueryMessageColor = "0000ff";
	defaultServerMessageColor = "91640a";
	defaultActionMessageColor = "0000ff";
	defaultBacklogMessageColor = "aaaaaa";
	defaultLinkMessageColor = "0000ff";
	defaultCommandMessageColor = "960096";

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

  notifyDelay=20;
  useNotify=true;
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

int Preferences::addServer(const QString& serverString)
{
  ServerEntry* newEntry=new ServerEntry(serverString);
  serverList.append(newEntry);

  return newEntry->getId();
}

void Preferences::removeServer(int id)
{
  /* Deletes the object, too */
  serverList.remove(getServerEntryById(id));
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

void Preferences::setIgnoreList(QPtrList<Ignore> newList)
{
  ignoreList.clear();
  ignoreList=newList;
}

void Preferences::addIgnore(QString newIgnore)
{
  QStringList ignore=QStringList::split(',',newIgnore);
  ignoreList.append(new Ignore(ignore[0],ignore[1].toInt()));
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

/* Accessor methods */

void Preferences::clearServerList() { serverList.clear(); }

void Preferences::setLog(bool state) { log=state; }
bool Preferences::getLog() { return log; }

int Preferences::getNotifyDelay() { return notifyDelay; }
void Preferences::setNotifyDelay(int delay) { notifyDelay=delay; }
bool Preferences::getUseNotify() { return useNotify; }
void Preferences::setUseNotify(bool use) { useNotify=use; }
void Preferences::setNotifyList(QStringList newList) { notifyList=newList; }
QStringList Preferences::getNotifyList() { return notifyList; }
QString Preferences::getNotifyString() { return notifyList.join(" "); }

QStringList Preferences::getButtonList() { return buttonList; }

QString Preferences::getPartReason() { return partReason; }
void Preferences::setPartReason(QString newReason) { partReason=newReason; }

QString Preferences::getKickReason() { return kickReason; }
void Preferences::setKickReason(QString newReason) { kickReason=newReason; }

void Preferences::clearIgnoreList() { ignoreList.clear(); }
QPtrList<Ignore> Preferences::getIgnoreList() { return ignoreList; }

QString Preferences::getNickname(int index) { return nicknameList[index]; }
QStringList Preferences::getNicknameList() { return nicknameList; }
void Preferences::setNickname(int index,QString newName) { nicknameList[index]=newName; }
void Preferences::setNicknameList(QStringList newList) { nicknameList=newList; }

void Preferences::setBlinkingTabs(bool blink) { blinkingTabs=blink; }
bool Preferences::getBlinkingTabs() { return blinkingTabs; }

// get/set message font colors

QString Preferences::getActionMessageColor() {return actionMessageColor;}
void Preferences::setActionMessageColor(QString passed_actionMessageColor) {actionMessageColor = passed_actionMessageColor;}

QString Preferences::getBacklogMessageColor() {return backlogMessageColor;}
void Preferences::setBacklogMessageColor(QString passed_backlogMessageColor) {backlogMessageColor = passed_backlogMessageColor;}

QString Preferences::getChannelMessageColor() {return channelMessageColor;}
void Preferences::setChannelMessageColor(QString passed_channelMessageColor) {channelMessageColor = passed_channelMessageColor;}

QString Preferences::getCommandMessageColor() {return commandMessageColor;}
void Preferences::setCommandMessageColor(QString passed_commandMessageColor) {commandMessageColor = passed_commandMessageColor;}

QString Preferences::getLinkMessageColor() {return linkMessageColor;}
void Preferences::setLinkMessageColor(QString passed_linkMessageColor) {linkMessageColor = passed_linkMessageColor;}

QString Preferences::getQueryMessageColor() {return queryMessageColor;}
void Preferences::setQueryMessageColor(QString passed_queryMessageColor) {queryMessageColor = passed_queryMessageColor;}

QString Preferences::getServerMessageColor() {return serverMessageColor;}
void Preferences::setServerMessageColor(QString passed_serverMessageColor) {serverMessageColor = passed_serverMessageColor;}

/* Geometry functions */
QSize Preferences::getServerWindowSize() 				{ return serverWindowSize; };
QSize& Preferences::getHilightSize()     				{ return hilightSize; };
QSize& Preferences::getButtonsSize()     				{ return buttonsSize; };
QSize& Preferences::getIgnoreSize()      				{ return ignoreSize; };
QSize& Preferences::getNotifySize()      				{ return notifySize; };
QSize& Preferences::getNicknameSize()   			  { return nicknameSize; };
QSize& Preferences::getColorConfigurationSize() {return colorConfigurationSize;}

void Preferences::setServerWindowSize(QSize newSize)			  { serverWindowSize=newSize; };
void Preferences::setHilightSize(QSize newSize)     				{ hilightSize=newSize; };
void Preferences::setButtonsSize(QSize newSize)     				{ buttonsSize=newSize; };
void Preferences::setIgnoreSize(QSize newSize)    			    { ignoreSize=newSize; };
void Preferences::setNotifySize(QSize newSize)    				  { notifySize=newSize; };
void Preferences::setNicknameSize(QSize newSize) 				    { nicknameSize=newSize; };
void Preferences::setColorConfigurationSize(QSize newSize)  {colorConfigurationSize = newSize;}
