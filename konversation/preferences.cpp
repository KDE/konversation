/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  preferences.cpp  -  Class for application wide preferences
  begin:     Tue Feb 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <ktoolbar.h>
#include <kstddirs.h>
#include <kdebug.h>

#include <qpalette.h>
#include <qtextcodec.h>

#include "preferences.h"
#include "prefsdialog.h"

Preferences::Preferences()
{
  // Presets
  serverWindowToolBarPos=KToolBar::Top;
  serverWindowStatusBarStatus=true;
  serverList.setAutoDelete(true);

  // create default identity
  Identity* identity=new Identity();
  identity->setName(i18n("Default identity"));
  addIdentity(identity);

  setIdent("konversation");
  setRealName("Konversation User");

  setNickname(0,"KonvIRC");
  setNickname(1,"_KonvIRC");
  setNickname(2,"KonvIRC_");
  setNickname(3,"_KonvIRC_");

  setPartReason("Konversation terminated!");
  setKickReason("User terminated!");

  setShowAwayMessage(false);
  setAwayMessage("/me is away: %s");
  setUnAwayMessage("/me is back.");

  setChannelMessageColor("000000");
  setQueryMessageColor("0000ff");
  setServerMessageColor("91640a");
  setActionMessageColor("0000ff");
  setBacklogMessageColor("aaaaaa");
  setLinkMessageColor("0000ff");
  setCommandMessageColor("960096");
  setTimeColor("709070");
  setTextViewBackground("");        // will be set on the first run of an ircview
  setNickCompleteSuffixStart(": ");
  setNickCompleteSuffixMiddle(" ");

  addServer("IRCNet,irc.kde.org,6667,,#kde-users,,0");

  buttonList.append("Op,/OP %u%n");
  buttonList.append("DeOp,/DEOP %u%n");
  buttonList.append("WhoIs,/WHOIS %s,%%u%n");
  buttonList.append("Version,/CTCP %s,%%u VERSION%n");
  buttonList.append("Kick,/KICK %u%n");
  buttonList.append("Ban,/BAN %u%n");
  buttonList.append("Part,/PART %c KDE Rules!%n");
  buttonList.append("Quit,/QUIT KDE Rules!%n");

  setShowQuickButtons(true);
  setShowModeButtons(true);
  setShowServerList(true);

  channelSplitter.append(10);
  channelSplitter.append(1);
  
  setAutoReconnect(true);
  setAutoRejoin(true);

  setFixedMOTD(true);

  setDccPath("");
  setDccAddPartner(true);
  setDccCreateFolder(false);
  setDccAutoGet(false);
  setDccBufferSize(1024);
  setDccRollback(1024);

  KStandardDirs kstddir;
  setLogPath(kstddir.saveLocation("data","konversation/logs"));

  setLog(true);
  setLowerLog(true);
  setLogFollowsNick(true);

  setBlinkingTabs(true);
  setBringToFront(true);

  setNotifyDelay(20);
  setUseNotify(true);

  setHilightNick(true);
  setHilightOwnLines(false);
  setHilightNickColor("#ff0000");
  setHilightOwnLinesColor("#ff0000");

  setOpLedColor(1);
  setVoiceLedColor(2);
  setNoRightsLedColor(3);

  setTimestamping(true);
  setTimestampFormat("hh:mm");

  setCodec(QTextCodec::codecForLocale()->name());

  setCommandChar("/");
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

QValueList<int> Preferences::getAutoConnectServerIDs()
{
  QValueList<int> list;

  ServerEntry* lookServer=serverList.first();
  while(lookServer)
  {
    if(lookServer->getAutoConnect()) list.append(lookServer->getId());
    lookServer=serverList.next();
  }

  return list;
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

QPtrList<Highlight> Preferences::getHilightList()
{
  return hilightList;
}

void Preferences::setHilightList(QPtrList<Highlight> newList)
{
  hilightList.clear();
  hilightList=newList;
}

void Preferences::addHilight(QString newHilight,QColor newColor)
{
  hilightList.append(new Highlight(newHilight,newColor));
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

// Accessor methods

void Preferences::clearServerList() { serverList.clear(); }

void Preferences::setLog(bool state) { log=state; }
bool Preferences::getLog() { return log; }
void Preferences::setLowerLog(bool state) { lowerLog=state; }
bool Preferences::getLowerLog() { return lowerLog; }
void Preferences::setLogFollowsNick(bool state) { logFollowsNick=state; }
bool Preferences::getLogFollowsNick() { return logFollowsNick; }
void Preferences::setLogPath(QString path) { logPath=path; }
QString Preferences::getLogPath() { return logPath; }

void Preferences::setDccAddPartner(bool state) { dccAddPartner=state; }
bool Preferences::getDccAddPartner() { return dccAddPartner; }

void Preferences::setDccCreateFolder(bool state) { dccCreateFolder=state; }
bool Preferences::getDccCreateFolder() { return dccCreateFolder; }

void Preferences::setDccBufferSize(unsigned long size) { dccBufferSize=size; }
unsigned long Preferences::getDccBufferSize() { return dccBufferSize; }

void Preferences::setDccRollback(unsigned long bytes) { dccRollback=bytes; }
unsigned long Preferences::getDccRollback() { return dccRollback; }

void Preferences::setDccAutoGet(bool state) { dccAutoGet=state; }
bool Preferences::getDccAutoGet() { return dccAutoGet; }

void Preferences::setDccPath(QString path) { dccPath=path; }
QString Preferences::getDccPath() { return dccPath; }

void Preferences::setFixedMOTD(bool fixed) { fixedMOTD=fixed; }
bool Preferences::getFixedMOTD() { return fixedMOTD; }

void Preferences::setAutoReconnect(bool on) { autoReconnect=on; }
bool Preferences::getAutoReconnect() { return autoReconnect; }

void Preferences::setAutoRejoin(bool on) { autoRejoin=on; }
bool Preferences::getAutoRejoin() { return autoRejoin; }

int Preferences::getNotifyDelay() { return notifyDelay; }
void Preferences::setNotifyDelay(int delay) { notifyDelay=delay; }
bool Preferences::getUseNotify() { return useNotify; }
void Preferences::setUseNotify(bool use) { useNotify=use; }
void Preferences::setNotifyList(QStringList newList) { notifyList=newList; }
QStringList Preferences::getNotifyList() { return notifyList; }
QString Preferences::getNotifyString() { return notifyList.join(" "); }

QStringList Preferences::getButtonList() { return buttonList; }

// Default identity functions
void Preferences::addIdentity(Identity* identity) { identityList.append(identity); }
void Preferences::clearIdentityList() { identityList.clear(); }
QPtrList<Identity> Preferences::getIdentityList() { return identityList; }

QString Preferences::getRealName() { return identityList.at(0)->getRealName(); }
void Preferences::setRealName(QString name) { identityList.at(0)->setRealName(name); }

QString Preferences::getIdent() { return identityList.at(0)->getIdent(); }
void Preferences::setIdent(QString ident) { identityList.at(0)->setIdent(ident); }

QString Preferences::getPartReason() { return identityList.at(0)->getPartReason(); }
void Preferences::setPartReason(QString newReason) { identityList.at(0)->setPartReason(newReason); }

QString Preferences::getKickReason() { return identityList.at(0)->getKickReason(); }
void Preferences::setKickReason(QString newReason) { identityList.at(0)->setKickReason(newReason); }

bool Preferences::getShowAwayMessage() { return identityList.at(0)->getShowAwayMessage(); }
void Preferences::setShowAwayMessage(bool state) { identityList.at(0)->setShowAwayMessage(state); }

QString Preferences::getAwayMessage() { return identityList.at(0)->getAwayMessage(); }
void Preferences::setAwayMessage(QString newMessage) { identityList.at(0)->setAwayMessage(newMessage); }
QString Preferences::getUnAwayMessage() { return identityList.at(0)->getReturnMessage(); }
void Preferences::setUnAwayMessage(QString newMessage) { identityList.at(0)->setReturnMessage(newMessage); }

void Preferences::clearIgnoreList() { ignoreList.clear(); }
QPtrList<Ignore> Preferences::getIgnoreList() { return ignoreList; }

QString Preferences::getNickname(int index) { return identityList.at(0)->getNickname(index); }
QStringList Preferences::getNicknameList() { return identityList.at(0)->getNicknameList(); }
void Preferences::setNickname(int index,QString newName) { identityList.at(0)->setNickname(index,newName); }
void Preferences::setNicknameList(QStringList newList) { identityList.at(0)->setNicknameList(newList); }

void Preferences::setBlinkingTabs(bool blink) { blinkingTabs=blink; }
bool Preferences::getBlinkingTabs() { return blinkingTabs; }

void Preferences::setBringToFront(bool state) { bringToFront=state; }
bool Preferences::getBringToFront() { return bringToFront; }

void Preferences::setCommandChar(QString newCommandChar) { commandChar=newCommandChar; }
QString Preferences::getCommandChar() { return commandChar; }

// TODO: Make this a little simpler (use an array and enum)
//       get/set message font colors

QString Preferences::getActionMessageColor()  { return actionMessageColor; }
QString Preferences::getBacklogMessageColor() { return backlogMessageColor; }
QString Preferences::getChannelMessageColor() { return channelMessageColor; }
QString Preferences::getCommandMessageColor() { return commandMessageColor; }
QString Preferences::getLinkMessageColor()    { return linkMessageColor; }
QString Preferences::getQueryMessageColor()   { return queryMessageColor; }
QString Preferences::getServerMessageColor()  { return serverMessageColor; }
QString Preferences::getTimeColor()           { return timeColor; }
QString Preferences::getTextViewBackground()  { return textViewBackground; }

void Preferences::setActionMessageColor(QString passed_actionMessageColor) {actionMessageColor = passed_actionMessageColor;}
void Preferences::setBacklogMessageColor(QString passed_backlogMessageColor) {backlogMessageColor = passed_backlogMessageColor;}
void Preferences::setChannelMessageColor(QString passed_channelMessageColor) {channelMessageColor = passed_channelMessageColor;}
void Preferences::setCommandMessageColor(QString passed_commandMessageColor) {commandMessageColor = passed_commandMessageColor;}
void Preferences::setLinkMessageColor(QString passed_linkMessageColor) {linkMessageColor = passed_linkMessageColor;}
void Preferences::setQueryMessageColor(QString passed_queryMessageColor) {queryMessageColor = passed_queryMessageColor;}
void Preferences::setServerMessageColor(QString passed_serverMessageColor) {serverMessageColor = passed_serverMessageColor;}
void Preferences::setTimeColor(QString passed_timeColor) { timeColor = passed_timeColor; }
void Preferences::setTextViewBackground(QString passed_background) { textViewBackground = passed_background; }

QString Preferences::getNickCompleteSuffixStart() {return nickCompleteSuffixStart; }
QString Preferences::getNickCompleteSuffixMiddle() {return nickCompleteSuffixMiddle; }

void Preferences::setNickCompleteSuffixStart(QString suffix) { nickCompleteSuffixStart=suffix; }
void Preferences::setNickCompleteSuffixMiddle(QString suffix) { nickCompleteSuffixMiddle=suffix; }

int Preferences::getOpLedColor()       { return opLedColor; }
int Preferences::getVoiceLedColor()    { return voiceLedColor; }
int Preferences::getNoRightsLedColor() { return noRightsLedColor; }
void Preferences::setOpLedColor(int passed_color)       { opLedColor=passed_color; }
void Preferences::setVoiceLedColor(int passed_color)    { voiceLedColor=passed_color; }
void Preferences::setNoRightsLedColor(int passed_color) { noRightsLedColor=passed_color; }

// Geometry functions
QSize Preferences::getServerWindowSize()  { return serverWindowSize; };
QSize& Preferences::getHilightSize()      { return hilightSize; };
QSize& Preferences::getButtonsSize()      { return buttonsSize; };
QSize& Preferences::getIgnoreSize()       { return ignoreSize; };
QSize& Preferences::getNotifySize()       { return notifySize; };
QSize& Preferences::getNicksOnlineSize()  { return nicksOnlineSize; };
QSize& Preferences::getNicknameSize()     { return nicknameSize; };
QSize& Preferences::getColorConfigurationSize() {return colorConfigurationSize;}

void Preferences::setServerWindowSize(QSize newSize)       { serverWindowSize=newSize; };
void Preferences::setHilightSize(QSize newSize)            { hilightSize=newSize; };
void Preferences::setButtonsSize(QSize newSize)            { buttonsSize=newSize; };
void Preferences::setIgnoreSize(QSize newSize)             { ignoreSize=newSize; };
void Preferences::setNotifySize(QSize newSize)             { notifySize=newSize; };
void Preferences::setNicksOnlineSize(QSize newSize)        { nicksOnlineSize=newSize; };
void Preferences::setNicknameSize(QSize newSize)           { nicknameSize=newSize; };
void Preferences::setColorConfigurationSize(QSize newSize) { colorConfigurationSize = newSize; }

void Preferences::setHilightNick(bool state) { hilightNick=state; }
bool Preferences::getHilightNick() { return hilightNick; }

void Preferences::setHilightNickColor(QString newColor) { hilightNickColor.setNamedColor(newColor); }
QColor Preferences::getHilightNickColor() { return hilightNickColor; }

void Preferences::setHilightOwnLines(bool state) { hilightOwnLines=state; }
bool Preferences::getHilightOwnLines() { return hilightOwnLines; }

void Preferences::setHilightOwnLinesColor(QString newColor) { hilightOwnLinesColor.setNamedColor(newColor); }
QColor Preferences::getHilightOwnLinesColor() { return hilightOwnLinesColor; }

QFont Preferences::getTextFont() { return textFont; }
QFont Preferences::getListFont() { return listFont; }
void Preferences::setTextFont(QFont newFont) { textFont=newFont; }
void Preferences::setListFont(QFont newFont) { listFont=newFont; }
void Preferences::setTextFontRaw(QString rawFont) { textFont.fromString(rawFont); }
void Preferences::setListFontRaw(QString rawFont) { listFont.fromString(rawFont); }

void Preferences::setTimestamping(bool state) { timestamping=state; }
bool Preferences::getTimestamping() { return timestamping; }
void Preferences::setTimestampFormat(const QString& newFormat) { timestampFormat=newFormat; }
const QString& Preferences::getTimestampFormat() { return timestampFormat; }

void Preferences::setShowQuickButtons(bool state) { showQuickButtons=state; }
bool Preferences::getShowQuickButtons() { return showQuickButtons; }

void Preferences::setShowModeButtons(bool state) { showModeButtons=state; }
bool Preferences::getShowModeButtons() { return showModeButtons; }

void Preferences::setShowServerList(bool state) { showServerList=state; }
bool Preferences::getShowServerList() { return showServerList; }

void Preferences::setChannelSplitter(QValueList<int> sizes) { channelSplitter=sizes; }
QValueList<int> Preferences::getChannelSplitter() { return channelSplitter; }

void Preferences::setCodec(QString newCodec) { codec=newCodec; }
QString Preferences::getCodec() { return codec; }

#include "preferences.moc"
