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
*/

#include <ktoolbar.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kurl.h>
#include <kuser.h>

#include <qpalette.h>

#include "preferences.h"
#include "prefsdialog.h"
#include "identity.h"
#include "ignore.h"
#include "highlight.h"
#include "serverentry.h"
#include "commit.h"
#include "version.h"


Preferences::Preferences()
{
  // Presets
  serverList.setAutoDelete(true);

  // create default identity
  identity=new Identity();
  identity->setName(i18n("Default Identity"));
  addIdentity(identity);

  KUser user(KUser::UseRealUserID);
  setIdent(user.loginName());
  setRealName(user.fullName());

  setNickname(0,user.loginName());
  setNickname(1,"_" + user.loginName());
  setNickname(2,user.loginName() + "_");
  setNickname(3,"_" + user.loginName() + "_");

  setPartReason("Konversation terminated!");
  setKickReason("User terminated!");

  setShowAwayMessage(false);
  setAwayMessage("/me is away: %s");
  setUnAwayMessage("/me is back.");

  setNickCompleteSuffixStart(": ");
  setNickCompleteSuffixMiddle(" ");

  addServer("Freenode,irc.kde.org,6667,,#kde-users,,0,");

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
  setShowTrayIcon(true);
  setShowBackgroundImage(false);
  setTrayNotify(false);

  setUseSpacing(false);
  setSpacing(2);
  setMargin(3);

  setUseParagraphSpacing(false);
  setParagraphSpacing(2);

  channelSplitter.append(10);
  channelSplitter.append(1);

  setAutoReconnect(true);
  setAutoRejoin(true);
  setAutojoinOnInvite(false);

  setMaximumLagTime(120);

  setFixedMOTD(true);
  setBeep(false);
  setRawLog(false);

  setVersionReply(i18n("Konversation %1 Build %2 (C)2002-2004 by the Konversation team").arg(VERSION).arg(COMMIT));
  setDccPath(user.homeDir()+"/dccrecv");
  setDccAddPartner(true);
  setDccCreateFolder(false);
  setDccSpecificSendPorts(false);
  setDccSendPortsFirst(0);
  setDccSendPortsLast(0);
  setDccSpecificChatPorts(false);
  setDccChatPortsFirst(0);
  setDccChatPortsLast(0);
  setDccGetIpFromServer(false);
  setDccAutoGet(false);
  setDccBufferSize(1024);
  setDccRollback(1024);

  KStandardDirs kstddir;
  setLogPath(kstddir.saveLocation("data","konversation/logs"));
  setScrollbackMax(1000);

  setLog(true);
  setLowerLog(true);
  setLogFollowsNick(true);

  setLogfileBufferSize(100);
  setLogfileReaderSize(QSize(400,200));

  setTabPlacement(Bottom);
  setBlinkingTabs(true);
  setCloseButtonsOnTabs(false);
  setCloseButtonsAlignRight(false);
  setBringToFront(true);

  setNotifyDelay(20);
  setUseNotify(true);


  setHilightNick(true);
  setHilightOwnLines(false);
  setHilightNickColor("#ff0000");
  setHilightOwnLinesColor("#ff0000");
  setHilightSoundEnabled(true);

  // On Screen Display
  setOSDUsage(true);
  setOSDShowOwnNick(true);
  setOSDShowQuery(true);
  setOSDShowChannelEvent(false);
  setOSDTextColor("#ffffff");
  setOSDDuration(3000);

  setColorInputFields(true);
  setBackgroundImageName(QString::null);

  setOpLedColor(1);
  setVoiceLedColor(2);
  setNoRightsLedColor(3);

  setTimestamping(true);
  setShowDate(false);
  setTimestampFormat("hh:mm");

  setCommandChar("/");
  setChannelDoubleClickAction("/QUERY %u%n");
  setNotifyDoubleClickAction("/WHOIS %u%n");

  setAdminValue(1);
  setOwnerValue(2);
  setOpValue(4);
  setHalfopValue(8);
  setVoiceValue(16);
  setNoRightsValue(32);

  setSortCaseInsensitive(true);
  setSortByStatus(false);

  setAutoUserhost(false);

  ircColorList.append("#ffffff");
  ircColorList.append("#000000");
  ircColorList.append("#000080");
  ircColorList.append("#008000");
  ircColorList.append("#ff0000");
  ircColorList.append("#a52a2a");
  ircColorList.append("#800080");
  ircColorList.append("#ff8000");
  ircColorList.append("#808000");
  ircColorList.append("#00ff00");
  ircColorList.append("#008080");
  ircColorList.append("#00ffff");
  ircColorList.append("#0000ff");
  ircColorList.append("#ffc0cb");
  ircColorList.append("#a0a0a0");
  ircColorList.append("#c0c0c0");
  setFilterColors(false);

  setNickCompletionMode(0);

#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 0)
  setShowToolBar(true);
#endif
  setShowMenuBar(true);
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 90)
  setShowStatusBar(true);
#endif
#if QT_VERSION >= 0x030200
  setShowTabBarCloseButton(false);
#endif

  setHideUnimportantEvents(false);
  setShowTopic(true);

  setShowRememberLineInAllWindows(false);
  setFocusNewQueries(true);

  // Web Browser
  setWebBrowserUseKdeDefault(true);
  setWebBrowserCmd("mozilla \'%u\'");

  setRedirectToStatusPane(false);
  
  setOpenWatchedNicksAtStartup(false);
}

Preferences::~Preferences()
{
  delete identity;
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

  // make sure that a new server entry gets at least the default identity
  const Identity* defaultIdentity=getIdentityList().at(0);
  QString defaultName=defaultIdentity->getName();

  if(newEntry->getIdentity().isEmpty())
    newEntry->setIdentity(defaultName);

  // put server entry into the list of servers
  serverList.append(newEntry);

  return newEntry->getId();
}

void Preferences::removeServer(int id)
{
  // Deletes the object, too
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

void Preferences::addHilight(const QString& newHilight,
                             bool regExp,
                             QColor newColor,
                             const QString& sound,
                             const QString& autoText)
{
  hilightList.append(new Highlight(newHilight,regExp,newColor,KURL(sound),autoText));
}

void Preferences::setHilightSoundEnabled(bool enabled)
{
  hilightSoundEnabled = enabled;
}

bool Preferences::getHilightSoundEnabled()
{
  return hilightSoundEnabled;
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

void Preferences::addIgnore(const QString &newIgnore)
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
void Preferences::setLogPath(const QString &path) { logPath=path; }
QString Preferences::getLogPath() { return logPath; }

void Preferences::setScrollbackMax(int max) { scrollbackMax=max; }
int Preferences::getScrollbackMax() { return scrollbackMax; }

void Preferences::setDccAddPartner(bool state) { dccAddPartner=state; }
bool Preferences::getDccAddPartner() { return dccAddPartner; }

void Preferences::setDccCreateFolder(bool state) { dccCreateFolder=state; }
bool Preferences::getDccCreateFolder() { return dccCreateFolder; }

void Preferences::setDccBufferSize(unsigned long size) { dccBufferSize=size; }
unsigned long Preferences::getDccBufferSize() { return dccBufferSize; }

void Preferences::setDccRollback(unsigned long bytes) { dccRollback=bytes; }
unsigned long Preferences::getDccRollback() { return dccRollback; }

void Preferences::setDccSpecificSendPorts(bool state) { dccSpecificSendPorts=state; }
bool Preferences::getDccSpecificSendPorts() { return dccSpecificSendPorts; }

void Preferences::setDccSendPortsFirst(unsigned long port)
{
  dccSendPortsFirst=port;
  if(getDccSendPortsLast() < port)
    setDccSendPortsLast(port);
}
unsigned int Preferences::getDccSendPortsFirst() { return dccSendPortsFirst; }

void Preferences::setDccSendPortsLast(unsigned long port)
{
  dccSendPortsLast=port;
  if(port < getDccSendPortsFirst())
    setDccSendPortsFirst(port);
}
unsigned int Preferences::getDccSendPortsLast() { return dccSendPortsLast; }

void Preferences::setDccSpecificChatPorts(bool state) { dccSpecificChatPorts=state; }
bool Preferences::getDccSpecificChatPorts() { return dccSpecificChatPorts; }

void Preferences::setDccChatPortsFirst(unsigned long port)
{
  dccChatPortsFirst=port;
  if(getDccChatPortsLast() < port)
    setDccChatPortsLast(port);
}
unsigned int Preferences::getDccChatPortsFirst() { return dccChatPortsFirst; }

void Preferences::setDccChatPortsLast(unsigned long port)
{
  dccChatPortsLast=port;
  if(port < getDccChatPortsFirst())
    setDccChatPortsFirst(port);
}
unsigned int Preferences::getDccChatPortsLast() { return dccChatPortsLast; }

void Preferences::setDccGetIpFromServer(bool state) { dccGetIpFromServer=state; }
bool Preferences::getDccGetIpFromServer() { return dccGetIpFromServer; }

void Preferences::setDccAutoGet(bool state) { dccAutoGet=state; }
bool Preferences::getDccAutoGet() { return dccAutoGet; }

void Preferences::setDccAutoResume(bool state) { dccAutoResume=state; }
bool Preferences::getDccAutoResume() { return dccAutoResume; }

void Preferences::setDccPath(const QString &path) { dccPath=path; }
QString Preferences::getDccPath() { return dccPath; }

void Preferences::setFixedMOTD(bool state) { fixedMOTD=state; }
bool Preferences::getFixedMOTD() { return fixedMOTD; }

void Preferences::setAutoReconnect(bool state) { autoReconnect=state; }
bool Preferences::getAutoReconnect() { return autoReconnect; }

void Preferences::setAutoRejoin(bool state) { autoRejoin=state; }
bool Preferences::getAutoRejoin() { return autoRejoin; }

void Preferences::setAutojoinOnInvite(bool state) { autojoinOnInvite=state; }
bool Preferences::getAutojoinOnInvite()           { return autojoinOnInvite; }

void Preferences::setBeep(bool state) { beep=state; }
bool Preferences::getBeep() { return beep; }

void Preferences::setRawLog(bool state) { rawLog=state; }
bool Preferences::getRawLog() { return rawLog; }

void    Preferences::setVersionReply(QString reply) { versionReply = reply; }
QString Preferences::getVersionReply() { return versionReply; }

int Preferences::getNotifyDelay() { return notifyDelay; }
void Preferences::setNotifyDelay(int delay) { notifyDelay=delay; }
bool Preferences::getUseNotify() { return useNotify; }
void Preferences::setUseNotify(bool use) { useNotify=use; }
void Preferences::setNotifyList(const QStringList &newList) { notifyList=newList; }
QStringList Preferences::getNotifyList() { return notifyList; }
QString Preferences::getNotifyString() { return notifyList.join(" "); }
bool Preferences::addNotify(const QString& newPattern)
{
  // don't add duplicates
  if(notifyList.findIndex(newPattern)==-1)
  {
    notifyList.append(newPattern);
    return true;
  }
  return false;
}
bool Preferences::removeNotify(const QString& pattern) { return (notifyList.remove(pattern)); }

QStringList Preferences::getButtonList() { return buttonList; }

// Default identity functions
void Preferences::addIdentity(Identity* identity) { identityList.append(identity); }
void Preferences::removeIdentity(Identity* identity) { identityList.remove(identity); }
void Preferences::clearIdentityList() { identityList.clear(); }
QPtrList<Identity> Preferences::getIdentityList() { return identityList; }

Identity * Preferences::getIdentityByName(const QString& name)
{
  QPtrList<Identity> identities=getIdentityList();
  Identity* identity=identities.first();
  while(identity)
  {
    if(identity->getName()==name) return identity;
    identity=identities.next();
  }
  // no matching identity found, return default identity
  identity=identities.first();
  return identity;
}

QString Preferences::getRealName() { return identityList.at(0)->getRealName(); }
void Preferences::setRealName(const QString &name) { identityList.at(0)->setRealName(name); }

QString Preferences::getIdent() { return identityList.at(0)->getIdent(); }
void Preferences::setIdent(const QString &ident) { identityList.at(0)->setIdent(ident); }

QString Preferences::getPartReason() { return identityList.at(0)->getPartReason(); }
void Preferences::setPartReason(const QString &newReason) { identityList.at(0)->setPartReason(newReason); }

QString Preferences::getKickReason() { return identityList.at(0)->getKickReason(); }
void Preferences::setKickReason(const QString &newReason) { identityList.at(0)->setKickReason(newReason); }

bool Preferences::getShowAwayMessage() { return identityList.at(0)->getShowAwayMessage(); }
void Preferences::setShowAwayMessage(bool state) { identityList.at(0)->setShowAwayMessage(state); }

QString Preferences::getAwayMessage() { return identityList.at(0)->getAwayMessage(); }
void Preferences::setAwayMessage(const QString &newMessage) { identityList.at(0)->setAwayMessage(newMessage); }
QString Preferences::getUnAwayMessage() { return identityList.at(0)->getReturnMessage(); }
void Preferences::setUnAwayMessage(const QString &newMessage) { identityList.at(0)->setReturnMessage(newMessage); }

void Preferences::clearIgnoreList() { ignoreList.clear(); }
QPtrList<Ignore> Preferences::getIgnoreList() { return ignoreList; }

QString Preferences::getNickname(int index) { return identityList.at(0)->getNickname(index); }
QStringList Preferences::getNicknameList() { return identityList.at(0)->getNicknameList(); }
void Preferences::setNickname(int index,const QString &newName) { identityList.at(0)->setNickname(index,newName); }
void Preferences::setNicknameList(const QStringList &newList) { identityList.at(0)->setNicknameList(newList); }

void Preferences::setTabPlacement(TabPlacement where) { tabPlacement=where; }
Preferences::TabPlacement Preferences::getTabPlacement() { return tabPlacement; }

void Preferences::setBlinkingTabs(bool blink) { blinkingTabs=blink; }
bool Preferences::getBlinkingTabs() { return blinkingTabs; }

void Preferences::setCloseButtonsOnTabs(bool state) { closeButtonsOnTabs=state; }
bool Preferences::getCloseButtonsOnTabs() { return closeButtonsOnTabs; }

void Preferences::setCloseButtonsAlignRight(bool state) { closeButtonsAlignRight=state; }
bool Preferences::getCloseButtonsAlignRight() { return closeButtonsAlignRight; }

void Preferences::setBringToFront(bool state) { bringToFront=state; }
bool Preferences::getBringToFront() { return bringToFront; }

void Preferences::setCommandChar(const QString &newCommandChar) { commandChar=newCommandChar; }
QString Preferences::getCommandChar() { return commandChar; }
void Preferences::setPreShellCommand(const QString& command) { preShellCommandStr=command; }
QString Preferences::getPreShellCommand() { return preShellCommandStr; }

// TODO: Make this a little simpler (use an array and enum)
//       get/set message font colors

QString Preferences::getColor(const QString& name)
{
  KConfig* config=KApplication::kApplication()->config();

  config->setGroup("Message Text Colors");
  QString color=config->readEntry(name,getDefaultColor(name));

  if(color.isEmpty()) color="000000";

  return color;
}

void Preferences::setColor(const QString& name,const QString& color)
{
  // if we get called from the KonversationApplication constructor kApplication is NULL
  KApplication* app=KApplication::kApplication();
  if(app)
  {
    KConfig* config=app->config();

    config->setGroup("Message Text Colors");
    config->writeEntry(name,color);
    config->sync();
  }
}

QString Preferences::getDefaultColor(const QString& name)
{
  if(name=="ChannelMessage")      return "000000";
  if(name=="QueryMessage")        return "0000ff";
  if(name=="ServerMessage")       return "91640a";
  if(name=="ActionMessage")       return "0000ff";
  if(name=="BacklogMessage")      return "aaaaaa";
  if(name=="LinkMessage")         return "0000ff";
  if(name=="CommandMessage")      return "960096";
  if(name=="Time")                return "709070";
  if(name=="TextViewBackground")  return "ffffff";
  if(name=="AlternateBackground") return "ffffff";

  return QString::null;
}

void Preferences::setColorInputFields(bool state) { colorInputFields=state; }
bool Preferences::getColorInputFields()           { return colorInputFields; }

void Preferences::setBackgroundImageName(const QString& name) { backgroundImage=name; }
const QString& Preferences::getBackgroundImageName() { return backgroundImage; }

QString Preferences::getNickCompleteSuffixStart() {return nickCompleteSuffixStart; }
QString Preferences::getNickCompleteSuffixMiddle() {return nickCompleteSuffixMiddle; }

void Preferences::setNickCompleteSuffixStart(const QString &suffix) { nickCompleteSuffixStart=suffix; }
void Preferences::setNickCompleteSuffixMiddle(const QString &suffix) { nickCompleteSuffixMiddle=suffix; }

int Preferences::getOpLedColor()       { return opLedColor; }
int Preferences::getVoiceLedColor()    { return voiceLedColor; }
int Preferences::getNoRightsLedColor() { return noRightsLedColor; }
void Preferences::setOpLedColor(int passed_color)       { opLedColor=passed_color; }
void Preferences::setVoiceLedColor(int passed_color)    { voiceLedColor=passed_color; }
void Preferences::setNoRightsLedColor(int passed_color) { noRightsLedColor=passed_color; }

int Preferences::getLogfileBufferSize()             { return logfileBufferSize; }
void Preferences::setLogfileBufferSize(int newSize) { logfileBufferSize=newSize; }

// Geometry functions
QSize Preferences::getNicksOnlineSize()        { return nicksOnlineSize; }
QSize Preferences::getNicknameSize()           { return nicknameSize; }
QSize Preferences::getLogfileReaderSize()      { return logfileReaderSize; }
QSize Preferences::getMultilineEditSize()      { return multilineEditSize; }

void Preferences::setNicksOnlineSize(QSize newSize)        { nicksOnlineSize=newSize; }
void Preferences::setNicknameSize(QSize newSize)           { nicknameSize=newSize; }
void Preferences::setLogfileReaderSize(QSize newSize)      { logfileReaderSize=newSize; }
void Preferences::setMultilineEditSize(QSize newSize)      { multilineEditSize=newSize; }

void Preferences::setHilightNick(bool state) { hilightNick=state; }
bool Preferences::getHilightNick() { return hilightNick; }

void Preferences::setHilightNickColor(const QString &newColor) { hilightNickColor.setNamedColor(newColor); }
QColor Preferences::getHilightNickColor() { return hilightNickColor; }

void Preferences::setHilightOwnLines(bool state) { hilightOwnLines=state; }
bool Preferences::getHilightOwnLines() { return hilightOwnLines; }

void Preferences::setHilightOwnLinesColor(const QString &newColor) { hilightOwnLinesColor.setNamedColor(newColor); }
QColor Preferences::getHilightOwnLinesColor() { return hilightOwnLinesColor; }

// On Screen Display
void Preferences::setOSDUsage(bool state) { OSDUsage=state; }
bool Preferences::getOSDUsage() { return OSDUsage; }

void Preferences::setOSDShowOwnNick(bool state) { OSDShowOwnNick=state; }
bool Preferences::getOSDShowOwnNick() { return OSDShowOwnNick; }

void Preferences::setOSDShowChannel(bool state) { OSDShowChannel=state; }
bool Preferences::getOSDShowChannel() { return OSDShowChannel; }

void Preferences::setOSDShowQuery(bool state) { OSDShowQuery=state; }
bool Preferences::getOSDShowQuery() { return OSDShowQuery; }

void Preferences::setOSDShowChannelEvent(bool state) { OSDShowChannelEvent=state; }
bool Preferences::getOSDShowChannelEvent() { return OSDShowChannelEvent; }

QFont Preferences::getOSDFont() { return osdFont; }
void Preferences::setOSDFont(QFont newFont) { osdFont=newFont; }
void Preferences::setOSDFontRaw(const QString &rawFont) { osdFont.fromString(rawFont); }

/*void Preferences::setOSDColor(const QString &newColor) { osdColor.setNamedColor(newColor); }
QColor Preferences::getOSDColor() { return osdColor; }*/

void Preferences::setOSDUseCustomColors(bool state) { useOSDCustomColors = state; }
bool Preferences::getOSDUseCustomColors() { return useOSDCustomColors; }

void Preferences::setOSDTextColor(const QString& newColor) { osdTextColor.setNamedColor(newColor); }
QColor Preferences::getOSDTextColor() { return osdTextColor; }

void Preferences::setOSDBackgroundColor(const QString& newColor) { osdBackgroundColor.setNamedColor(newColor); }
QColor Preferences::getOSDBackgroundColor() { return osdBackgroundColor; }

void Preferences::setOSDDuration(int ms) { OSDDuration = ms; }
int Preferences::getOSDDuration() { return OSDDuration; }

void Preferences::setOSDScreen(uint screen) { OSDScreen = screen; }
uint Preferences::getOSDScreen() { return OSDScreen; }

void Preferences::setOSDDrawShadow(bool state) { OSDDrawShadow = state; }
bool Preferences::getOSDDrawShadow() { return OSDDrawShadow; }

QFont Preferences::getTextFont() { return textFont; }
QFont Preferences::getListFont() { return listFont; }
void Preferences::setTextFont(QFont newFont) { textFont=newFont; }
void Preferences::setListFont(QFont newFont) { listFont=newFont; }
void Preferences::setTextFontRaw(const QString &rawFont) { textFont.fromString(rawFont); }
void Preferences::setListFontRaw(const QString &rawFont) { listFont.fromString(rawFont); }

void Preferences::setTimestamping(bool state) { timestamping=state; }
bool Preferences::getTimestamping() { return timestamping; }
void Preferences::setShowDate(bool state) { showDate=state; }
bool Preferences::getShowDate() { return showDate; }
void Preferences::setTimestampFormat(const QString& newFormat) { timestampFormat=newFormat; }
QString Preferences::getTimestampFormat() { return timestampFormat; }

void Preferences::setShowQuickButtons(bool state) { showQuickButtons=state; }
bool Preferences::getShowQuickButtons() { return showQuickButtons; }

void Preferences::setShowModeButtons(bool state) { showModeButtons=state; }
bool Preferences::getShowModeButtons() { return showModeButtons; }

void Preferences::setShowServerList(bool state) { showServerList=state; }
bool Preferences::getShowServerList() { return showServerList; }

void Preferences::setShowBackgroundImage(bool state) { showBackgroundImage=state; }
bool Preferences::getShowBackgroundImage() { return showBackgroundImage; }

void Preferences::setShowTrayIcon(bool state)
{
  showTrayIcon=state;
  emit updateTrayIcon();
}
bool Preferences::getShowTrayIcon() { return showTrayIcon; }

void Preferences::setTrayNotify(bool state)
{
  trayNotify = state;
  emit updateTrayIcon();
}

bool Preferences::getTrayNotify() { return trayNotify; }

void Preferences::setChannelSplitter(QValueList<int> sizes) { channelSplitter=sizes; }
QValueList<int> Preferences::getChannelSplitter() { return channelSplitter; }

void Preferences::setChannelDoubleClickAction(const QString &action) { channelDoubleClickAction=action; }
QString Preferences::getChannelDoubleClickAction() { return channelDoubleClickAction; }

void Preferences::setNotifyDoubleClickAction(const QString &action) { notifyDoubleClickAction=action; }
QString Preferences::getNotifyDoubleClickAction() { return notifyDoubleClickAction; }

void Preferences::setUseSpacing(bool state) { useSpacing=state; }
bool Preferences::getUseSpacing() { return useSpacing; }
void Preferences::setSpacing(int newSpacing) { spacing=newSpacing; }
int Preferences::getSpacing() { return spacing; }
void Preferences::setMargin(int newMargin) { margin=newMargin; }
int Preferences::getMargin() { return margin; }

void Preferences::setUseParagraphSpacing(bool state) { useParagraphSpacing=state; }
bool Preferences::getUseParagraphSpacing() { return useParagraphSpacing; }
void Preferences::setParagraphSpacing(int newSpacing) { paragraphSpacing=newSpacing; }
int Preferences::getParagraphSpacing() { return paragraphSpacing; }

// sorting stuff
void Preferences::setAdminValue(int value)           { adminValue=value; }
void Preferences::setOwnerValue(int value)           { ownerValue=value; }
void Preferences::setOpValue(int value)              { opValue=value; }
void Preferences::setHalfopValue(int value)          { halfopValue=value; }
void Preferences::setVoiceValue(int value)           { voiceValue=value; }
void Preferences::setNoRightsValue(int value)        { noRightsValue=value; }

int Preferences::getAdminValue()                     { return adminValue; }
int Preferences::getOwnerValue()                     { return ownerValue; }
int Preferences::getOpValue()                        { return opValue; }
int Preferences::getHalfopValue()                    { return halfopValue; }
int Preferences::getVoiceValue()                     { return voiceValue; }
int Preferences::getNoRightsValue()                  { return noRightsValue; }

void Preferences::setSortCaseInsensitive(bool state) { sortCaseInsensitive=state; }
void Preferences::setSortByStatus(bool state)        { sortByStatus=state; }

bool Preferences::getSortCaseInsensitive()           { return sortCaseInsensitive; }
bool Preferences::getSortByStatus()                  { return sortByStatus; }

bool Preferences::getAutoUserhost()                  { return autoUserhost; }
void Preferences::setAutoUserhost(bool state)
{
  autoUserhost=state;
  emit autoUserhostChanged(state);
}

bool Preferences::getDialogFlag(const QString& flagName)
{
  KConfig* config=KApplication::kApplication()->config();

  config->setGroup("Notification Messages");

  if( config->readEntry(flagName) != QString::null )
    return false;
  else
    return true;
}

void Preferences::setDialogFlag(const QString& flagName,bool state)
{
  KConfig* config=KApplication::kApplication()->config();

  config->setGroup("Notification Messages");

  if(state)
    config->deleteEntry(flagName);
  else
  {
    if ( config->readEntry(flagName) == QString::null )
      config->writeEntry(flagName,"no");
  }

  config->sync();
}

void Preferences::setMaximumLagTime(int lag) { maximumLag=lag; }
int Preferences::getMaximumLagTime()         { return maximumLag; }

void Preferences::setIRCColorList(QStringList cl) { ircColorList=cl; }
QStringList Preferences::getIRCColorList()        { return ircColorList; }

void Preferences::setAliasList(QStringList newList) { aliasList=newList; }
QStringList Preferences::getAliasList()             { return aliasList; }

int Preferences::getNickCompletionMode() { return nickCompletionMode; }
void Preferences::setNickCompletionMode(int mode) { nickCompletionMode = mode; }
QString Preferences::getPrefixCharacter() { return prefixCharacter; }
void  Preferences::setPrefixCharacter(QString prefix) { prefixCharacter = prefix; }

bool Preferences::getShowMenuBar() { return showMenuBar; }
void Preferences::setShowMenuBar(bool s) { showMenuBar = s; }

#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 0)
bool Preferences::getShowToolBar() { return showToolBar; }
void Preferences::setShowToolBar(bool s) { showToolBar = s; }
#endif
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 90)
bool Preferences::getShowStatusBar() { return showStatusBar; }
void Preferences::setShowStatusBar(bool s) { showStatusBar = s; }
#endif
#if QT_VERSION >= 0x030200
bool Preferences::getShowTabBarCloseButton() { return showTabBarCloseButton; }
void Preferences::setShowTabBarCloseButton(bool s) { showTabBarCloseButton = s; }
#endif

bool Preferences::getShowTopic() { return showTopic; }
void Preferences::setShowTopic(bool s) { showTopic = s; }

bool Preferences::getShowRememberLineInAllWindows() { return showRememberLineInAllWindows; }
void Preferences::setShowRememberLineInAllWindows(bool s) { showRememberLineInAllWindows = s; }

bool Preferences::getFocusNewQueries() { return focusNewQueries; }
void Preferences::setFocusNewQueries(bool s) { focusNewQueries = s; }

bool Preferences::getHideUnimportantEvents()           { return hideUnimportantEvents; }
void Preferences::setHideUnimportantEvents(bool state) { hideUnimportantEvents=state; }

bool Preferences::getDisableExpansion() { return disableExpansion; }
void Preferences::setDisableExpansion(bool state) { disableExpansion = state; }

// Web Browser
bool Preferences::getWebBrowserUseKdeDefault() { return webBrowserUseKdeDefault; }
void Preferences::setWebBrowserUseKdeDefault(bool state) { webBrowserUseKdeDefault = state; }
QString Preferences::getWebBrowserCmd() { return webBrowserCmd; }
void Preferences::setWebBrowserCmd(const QString &cmd) { webBrowserCmd=cmd; }

bool Preferences::getFilterColors() { return filterColors; }
void Preferences::setFilterColors(bool filter) { filterColors = filter; }

bool Preferences::getRedirectToStatusPane() { return redirectToStatusPane; }
void Preferences::setRedirectToStatusPane(bool redirect) { redirectToStatusPane = redirect; }

bool Preferences::getOpenWatchedNicksAtStartup() { return m_openWatchedNicksAtStartup; }
void Preferences::setOpenWatchedNicksAtStartup(bool open) { m_openWatchedNicksAtStartup = open; }

#include "preferences.moc"
