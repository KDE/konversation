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
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kapplication.h>
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
#include "commit.h"
#include "version.h"


Preferences::Preferences()
{
  // create default identity
  identity=new Identity();
  identity->setName(i18n("Default Identity"));
  addIdentity(identity);

  KUser user(KUser::UseRealUserID);
  setIdent(user.loginName());
  setRealName(user.fullName());

  QStringList nickList;
  nickList.append(user.loginName());
  nickList.append("_" + user.loginName());
  nickList.append(user.loginName() + "_");
  nickList.append("_" + user.loginName() + "_");
  identity->setNicknameList(nickList);

  setPartReason("Konversation terminated!");
  setKickReason("User terminated!");

  setShowAwayMessage(false);
  setAwayMessage("/me is away: %s");
  setUnAwayMessage("/me is back.");

  setNickCompleteSuffixStart(": ");
  setNickCompleteSuffixMiddle(" ");

  Konversation::ServerGroupSettings serverGroup;
  serverGroup.setName("Freenode");
  Konversation::ServerSettings server;
  server.setServer("irc.freenode.org");
  serverGroup.addServer(server);
  serverGroup.setIdentityId(identity->id());
  Konversation::ChannelSettings channel;
  channel.setName("#kde");
  serverGroup.addChannel(channel);
  m_serverGroupList.append(serverGroup);

  buttonList.append("Op,/OP %u%n");
  buttonList.append("DeOp,/DEOP %u%n");
  buttonList.append("WhoIs,/WHOIS %s,%%u%n");
  buttonList.append("Version,/CTCP %s,%%u VERSION%n");
  buttonList.append("Kick,/KICK %u%n");
  buttonList.append("Ban,/BAN %u%n");
  buttonList.append("Part,/PART %c KDE Rules!%n");
  buttonList.append("Quit,/QUIT KDE Rules!%n");

  setShowQuickButtons(false);
  setShowModeButtons(true);
  setShowServerList(true);
  setShowTrayIcon(false);
  setShowBackgroundImage(false);
  setTrayNotify(false);
  setSystrayOnly(false);
  setTrayNotifyOnlyOwnNick(false);

  setUseSpacing(false);
  setSpacing(2);
  setMargin(3);

  setUseParagraphSpacing(false);
  setParagraphSpacing(2);

  setAutoReconnect(true);
  setAutoRejoin(true);
  setAutojoinOnInvite(false);

  setMaximumLagTime(120);

  setFixedMOTD(true);
  setBeep(false);
  setRawLog(false);

  setVersionReply(QString::null); //XXX is this really necessary?
  setDccPath(user.homeDir()+"/dccrecv");
  setDccAddPartner(false);
  setDccCreateFolder(false);
  setDccMethodToGetOwnIp(1);
  setDccSpecificOwnIp("0.0.0.0");
  setDccSpecificSendPorts(false);
  setDccSendPortsFirst(0);
  setDccSendPortsLast(0);
  setDccSpecificChatPorts(false);
  setDccChatPortsFirst(0);
  setDccChatPortsLast(0);
  setDccAutoGet(false);
  setDccBufferSize(8192);
  setDccFastSend(true);
  setDccSendTimeout(120);

  KStandardDirs kstddir;
  setLogPath(kstddir.saveLocation("data","konversation/logs"));
  setScrollbackMax(1000);
  
  setAutoWhoNicksLimit(200);
  setAutoWhoContinuousEnabled(true);
  setAutoWhoContinuousInterval(90);

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


  setHighlightNick(true);
  setHighlightOwnLines(false);
  setHighlightNickColor("#ff0000");
  setHighlightOwnLinesColor("#ff0000");
  setHighlightSoundEnabled(true);

  // On Screen Display
  setOSDUsage(false);
  setOSDShowOwnNick(false);
  setOSDShowQuery(false);
  setOSDShowChannelEvent(false);
  setOSDTextColor("#ffffff");
  setOSDDuration(3000);
  setOSDOffsetX(30);
  setOSDOffsetY(50);
  setOSDAlignment(0);  // Left

  setColorInputFields(true);
  setBackgroundImageName(QString::null);

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
  setAwayValue(32);
  setNoRightsValue(64);

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

  nickColorList.append("#E90E7F");
  nickColorList.append("#8E55E9");
  nickColorList.append("#B30E0E");
  nickColorList.append("#18B33C");
  nickColorList.append("#58ADB3");
  nickColorList.append("#9E54B3");
  nickColorList.append("#0FB39A");
  nickColorList.append("#3176B3");
  setUseColoredNicks(false);

  setNickCompletionMode(2);
  setNickCompletionCaseSensitive(false);

  setShowMenuBar(true);
  setShowTabBarCloseButton(true);

  setHideUnimportantEvents(false);
  setShowTopic(true);
  setShowNicknameBox(true);

  setShowRememberLineInAllWindows(false);
  setFocusNewQueries(true);

  // Web Browser
  setWebBrowserUseKdeDefault(true);
  setWebBrowserCmd("mozilla \'%u\'");

  setRedirectToStatusPane(true);
  
  setOpenWatchedNicksAtStartup(false);

  // Themes
  setIconTheme("default");
}

Preferences::~Preferences()
{
  clearIdentityList();
}

const Konversation::ServerGroupList Preferences::serverGroupList()
{
  return m_serverGroupList;
}

void Preferences::setServerGroupList(const Konversation::ServerGroupList& list)
{
  m_serverGroupList.clear();
  m_serverGroupList = list;
}

void Preferences::addServerGroup(const Konversation::ServerGroupSettings& serverGroup)
{
  m_serverGroupList.append(serverGroup);
}

const Konversation::ServerGroupSettings Preferences::serverGroupById(int id)
{
  if(!m_serverGroupList.count()) {
    return Konversation::ServerGroupSettings();
  }
  
  Konversation::ServerGroupList::iterator it;
  
  for(it = m_serverGroupList.begin(); it != m_serverGroupList.end(); ++it) {
    if((*it).id() == id) {
      return (*it);
    }
  }

  return Konversation::ServerGroupSettings();
}

void Preferences::removeServerGroup(int id)
{
  if(!m_serverGroupList.count()) {
    return;
  }
  
  Konversation::ServerGroupList::iterator it;
  
  for(it = m_serverGroupList.begin(); it != m_serverGroupList.end(); ++it) {
    if((*it).id() == id) {
      m_serverGroupList.remove(it);
      return;
    }
  }
}

const QPtrList<Highlight> Preferences::getHighlightList()
{
  return highlightList;
}

void Preferences::setHighlightList(QPtrList<Highlight> newList)
{
  highlightList.clear();
  highlightList=newList;
}

void Preferences::addHighlight(const QString& newHighlight,
                             bool regExp,
                             const QColor &newColor,
                             const QString& sound,
                             const QString& autoText)
{
  highlightList.append(new Highlight(newHighlight,regExp,newColor,KURL(sound),autoText));
}

void Preferences::setHighlightSoundEnabled(bool enabled)
{
  highlightSoundEnabled = enabled;
}

const bool Preferences::getHighlightSoundEnabled()
{
  return highlightSoundEnabled;
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

// Accessor methods

void Preferences::setLog(bool state) { log=state; }
const bool Preferences::getLog() { return log; }
void Preferences::setLowerLog(bool state) { lowerLog=state; }
const bool Preferences::getLowerLog() { return lowerLog; }
void Preferences::setLogFollowsNick(bool state) { logFollowsNick=state; }
const bool Preferences::getLogFollowsNick() { return logFollowsNick; }
void Preferences::setLogPath(const QString &path) { logPath=path; }
const QString Preferences::getLogPath() { return logPath; }

void Preferences::setScrollbackMax(int max) { scrollbackMax=max; }
const int Preferences::getScrollbackMax() { return scrollbackMax; }

void Preferences::setAutoWhoNicksLimit(int limit) { autoWhoNicksLimit=limit; }
const int Preferences::getAutoWhoNicksLimit() { return autoWhoNicksLimit; }
void Preferences::setAutoWhoContinuousEnabled(bool state)
{ 
  autoWhoContinuousEnabled=state;
  emit autoContinuousWhoChanged();
}
const bool Preferences::getAutoWhoContinuousEnabled() { return autoWhoContinuousEnabled; }
void Preferences::setAutoWhoContinuousInterval(int interval) { autoWhoContinuousInterval=interval; }
const int Preferences::getAutoWhoContinuousInterval() { return autoWhoContinuousInterval; }

void Preferences::setDccAddPartner(bool state) { dccAddPartner=state; }
const bool Preferences::getDccAddPartner() { return dccAddPartner; }

void Preferences::setDccCreateFolder(bool state) { dccCreateFolder=state; }
const bool Preferences::getDccCreateFolder() { return dccCreateFolder; }

void Preferences::setDccBufferSize(unsigned long size) { dccBufferSize=size; }
const unsigned long Preferences::getDccBufferSize() { return dccBufferSize; }

void Preferences::setDccMethodToGetOwnIp(int methodId) { dccMethodToGetOwnIp=methodId; }
const int Preferences::getDccMethodToGetOwnIp() { return dccMethodToGetOwnIp; }

void Preferences::setDccSpecificOwnIp(const QString& ip) { dccSpecificOwnIp = ip; }
const QString Preferences::getDccSpecificOwnIp() { return dccSpecificOwnIp; }

void Preferences::setDccSpecificSendPorts(bool state) { dccSpecificSendPorts=state; }
const bool Preferences::getDccSpecificSendPorts() { return dccSpecificSendPorts; }

void Preferences::setDccSendPortsFirst(unsigned long port)
{
  dccSendPortsFirst=port;
  if(getDccSendPortsLast() < port)
    setDccSendPortsLast(port);
}
const unsigned int Preferences::getDccSendPortsFirst() { return dccSendPortsFirst; }

void Preferences::setDccSendPortsLast(unsigned long port)
{
  dccSendPortsLast=port;
  if(port < getDccSendPortsFirst())
    setDccSendPortsFirst(port);
}
const unsigned int Preferences::getDccSendPortsLast() { return dccSendPortsLast; }

void Preferences::setDccSpecificChatPorts(bool state) { dccSpecificChatPorts=state; }
const bool Preferences::getDccSpecificChatPorts() { return dccSpecificChatPorts; }

void Preferences::setDccChatPortsFirst(unsigned long port)
{
  dccChatPortsFirst=port;
  if(getDccChatPortsLast() < port)
    setDccChatPortsLast(port);
}
const unsigned int Preferences::getDccChatPortsFirst() { return dccChatPortsFirst; }

void Preferences::setDccChatPortsLast(unsigned long port)
{
  dccChatPortsLast=port;
  if(port < getDccChatPortsFirst())
    setDccChatPortsFirst(port);
}
const unsigned int Preferences::getDccChatPortsLast() { return dccChatPortsLast; }

void Preferences::setDccAutoGet(bool state) { dccAutoGet=state; }
const bool Preferences::getDccAutoGet() { return dccAutoGet; }

void Preferences::setDccAutoResume(bool state) { dccAutoResume=state; }
const bool Preferences::getDccAutoResume() { return dccAutoResume; }

void Preferences::setDccPath(const QString &path) { dccPath=path; }
const QString Preferences::getDccPath() { return dccPath; }

void Preferences::setDccFastSend(bool state) { dccFastSend=state; }
const bool Preferences::getDccFastSend() { return dccFastSend; }

void Preferences::setDccSendTimeout(int sec) { dccSendTimeout=sec; }
const int Preferences::getDccSendTimeout() { return dccSendTimeout; }

void Preferences::setFixedMOTD(bool state) { fixedMOTD=state; }
const bool Preferences::getFixedMOTD() { return fixedMOTD; }

void Preferences::setAutoReconnect(bool state) { autoReconnect=state; }
const bool Preferences::getAutoReconnect() { return autoReconnect; }

void Preferences::setAutoRejoin(bool state) { autoRejoin=state; }
const bool Preferences::getAutoRejoin() { return autoRejoin; }

void Preferences::setAutojoinOnInvite(bool state) { autojoinOnInvite=state; }
const bool Preferences::getAutojoinOnInvite()           { return autojoinOnInvite; }

void Preferences::setBeep(bool state) { beep=state; }
const bool Preferences::getBeep() { return beep; }

void Preferences::setRawLog(bool state) { rawLog=state; }
const bool Preferences::getRawLog() { return rawLog; }

void Preferences::setVersionReply(const QString &reply) { versionReply = reply; }

/**
  Get the version custom version response, or the default if none has been configured.
  @param forRc If true, this request is going to be stored in the RC file so do not return the default reply.
*/
const QString Preferences::getVersionReply(bool forRC) const
{
  if (!forRC && versionReply.isEmpty())
    return QString("Konversation %1 Build %2 (C)2002-2004 by the Konversation team.").arg(VERSION).arg(COMMIT);
  return versionReply;
}

const int Preferences::getNotifyDelay() { return notifyDelay; }
void Preferences::setNotifyDelay(int delay) { notifyDelay=delay; }
const bool Preferences::getUseNotify() { return useNotify; }
void Preferences::setUseNotify(bool use) { useNotify=use; }
void Preferences::setNotifyList(const QMap<QString, QStringList> &newList)
  { notifyList=newList; }
const QMap<QString, QStringList> Preferences::getNotifyList() { return notifyList; }
const QStringList Preferences::getNotifyListByGroup(const QString& groupName)
{
  if (notifyList.find(groupName) != notifyList.end())
    return notifyList[groupName];
  else
    return QStringList();
}
const QString Preferences::getNotifyStringByGroup(const QString& groupName)
{
  return getNotifyListByGroup(groupName).join(" ");
}
const bool Preferences::addNotify(const QString& groupName, const QString& newPattern)
{
  // don't add duplicates
  if (groupName.isEmpty() || newPattern.isEmpty()) return false;
  if (!notifyList[groupName].contains(newPattern))
  {
    QStringList nicknameList = notifyList[groupName];
    nicknameList.append(newPattern);
    notifyList[groupName] = nicknameList;
    return true;
  }
  return false;
}
const bool Preferences::removeNotify(const QString& groupName, const QString& pattern)
{
  if (notifyList.find(groupName) != notifyList.end())
  {
    QStringList nicknameList = notifyList[groupName];
    nicknameList.remove(pattern);
    if (nicknameList.isEmpty())
        notifyList.remove(groupName);
    else
        notifyList[groupName] = nicknameList;
    return true;
  } else
    return false;
}

const QStringList Preferences::getButtonList() { return buttonList; }

// Default identity functions
void Preferences::addIdentity(IdentityPtr identity) { identityList.append(identity); }
void Preferences::removeIdentity(IdentityPtr identity) { identityList.remove(identity); }

void Preferences::clearIdentityList()
{
  identityList.clear();
}

const QValueList<IdentityPtr> Preferences::getIdentityList() { return identityList; }

void Preferences::setIdentityList(const QValueList<IdentityPtr>& list)
{
  identityList.clear();
  identityList = list;
}

const IdentityPtr Preferences::getIdentityByName(const QString& name)
{
  QValueList<IdentityPtr> identities = getIdentityList();
  QValueList<IdentityPtr>::iterator it = identities.begin();

  while(it != identities.end())
  {
    if((*it)->getName() == name) {
      return (*it);
    }

    ++it;
  }

  // no matching identity found, return default identity
  return identities.first();
}

const IdentityPtr Preferences::getIdentityById(int id)
{
  QValueList<IdentityPtr> identityList = getIdentityList();
  for(QValueList<IdentityPtr>::iterator it = identityList.begin(); it != identityList.end(); ++it) {
    if((*it)->id() == id) {
      return (*it);
    }
  }

  return identityList.first();
}

const QString Preferences::getRealName() { return identityList[0]->getRealName(); }
void Preferences::setRealName(const QString &name) { identityList[0]->setRealName(name); }

const QString Preferences::getIdent() { return identityList[0]->getIdent(); }
void Preferences::setIdent(const QString &ident) { identityList[0]->setIdent(ident); }

const QString Preferences::getPartReason() { return identityList[0]->getPartReason(); }
void Preferences::setPartReason(const QString &newReason) { identityList[0]->setPartReason(newReason); }

const QString Preferences::getKickReason() { return identityList[0]->getKickReason(); }
void Preferences::setKickReason(const QString &newReason) { identityList[0]->setKickReason(newReason); }

const bool Preferences::getShowAwayMessage() { return identityList[0]->getShowAwayMessage(); }
void Preferences::setShowAwayMessage(bool state) { identityList[0]->setShowAwayMessage(state); }

const QString Preferences::getAwayMessage() { return identityList[0]->getAwayMessage(); }
void Preferences::setAwayMessage(const QString &newMessage) { identityList[0]->setAwayMessage(newMessage); }
const QString Preferences::getUnAwayMessage() { return identityList[0]->getReturnMessage(); }
void Preferences::setUnAwayMessage(const QString &newMessage) { identityList[0]->setReturnMessage(newMessage); }

void Preferences::clearIgnoreList() { ignoreList.clear(); }
const QPtrList<Ignore> Preferences::getIgnoreList() { return ignoreList; }

const QString Preferences::getNickname(int index) { return identityList[0]->getNickname(index); }
const QStringList Preferences::getNicknameList() { return identityList[0]->getNicknameList(); }
void Preferences::setNickname(int index,const QString &newName) { identityList[0]->setNickname(index,newName); }
void Preferences::setNicknameList(const QStringList &newList) { identityList[0]->setNicknameList(newList); }

void Preferences::setTabPlacement(TabPlacement where) { tabPlacement=where; }
const Preferences::TabPlacement Preferences::getTabPlacement() { return tabPlacement; }

void Preferences::setBlinkingTabs(bool blink) { blinkingTabs=blink; }
const bool Preferences::getBlinkingTabs() { return blinkingTabs; }

void Preferences::setCloseButtonsOnTabs(bool state) { closeButtonsOnTabs=state; }
const bool Preferences::getCloseButtonsOnTabs() { return closeButtonsOnTabs; }

void Preferences::setCloseButtonsAlignRight(bool state) { closeButtonsAlignRight=state; }
const bool Preferences::getCloseButtonsAlignRight() { return closeButtonsAlignRight; }

void Preferences::setBringToFront(bool state) { bringToFront=state; }
const bool Preferences::getBringToFront() { return bringToFront; }

void Preferences::setCommandChar(const QString &newCommandChar) { commandChar=newCommandChar; }
const QString Preferences::getCommandChar() { return commandChar; }
void Preferences::setPreShellCommand(const QString& command) { preShellCommandStr=command; }
const QString Preferences::getPreShellCommand() { return preShellCommandStr; }

// TODO: Make this a little simpler (use an array and enum)
//       get/set message font colors

const QString Preferences::getColor(const QString& name)
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

const QString Preferences::getDefaultColor(const QString& name)
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
const bool Preferences::getColorInputFields()           { return colorInputFields; }

void Preferences::setBackgroundImageName(const QString& name) { backgroundImage=name; }
const QString& Preferences::getBackgroundImageName() { return backgroundImage; }

const QString Preferences::getNickCompleteSuffixStart() {return nickCompleteSuffixStart; }
const QString Preferences::getNickCompleteSuffixMiddle() {return nickCompleteSuffixMiddle; }

void Preferences::setNickCompleteSuffixStart(const QString &suffix) { nickCompleteSuffixStart=suffix; }
void Preferences::setNickCompleteSuffixMiddle(const QString &suffix) { nickCompleteSuffixMiddle=suffix; }

const int Preferences::getLogfileBufferSize()             { return logfileBufferSize; }
void Preferences::setLogfileBufferSize(int newSize) { logfileBufferSize=newSize; }

// Geometry functions
const QSize Preferences::getNicksOnlineSize()        { return nicksOnlineSize; }
const QSize Preferences::getNicknameSize()           { return nicknameSize; }
const QSize Preferences::getLogfileReaderSize()      { return logfileReaderSize; }
const QSize Preferences::getMultilineEditSize()      { return multilineEditSize; }

void Preferences::setNicksOnlineSize(const QSize &newSize)     { nicksOnlineSize=newSize; }
void Preferences::setNicknameSize(const QSize &newSize)        { nicknameSize=newSize; }
void Preferences::setLogfileReaderSize(const QSize &newSize)   { logfileReaderSize=newSize; }
void Preferences::setMultilineEditSize(const QSize &newSize)   { multilineEditSize=newSize; }

void Preferences::setHighlightNick(bool state) { highlightNick=state; }
const bool Preferences::getHighlightNick() { return highlightNick; }

void Preferences::setHighlightNickColor(const QString &newColor) { highlightNickColor.setNamedColor(newColor); }
const QColor Preferences::getHighlightNickColor() { return highlightNickColor; }

void Preferences::setHighlightOwnLines(bool state) { highlightOwnLines=state; }
const bool Preferences::getHighlightOwnLines() { return highlightOwnLines; }

void Preferences::setHighlightOwnLinesColor(const QString &newColor) { highlightOwnLinesColor.setNamedColor(newColor); }
const QColor Preferences::getHighlightOwnLinesColor() { return highlightOwnLinesColor; }

// On Screen Display
void Preferences::setOSDUsage(bool state) { OSDUsage=state; }
const bool Preferences::getOSDUsage() { return OSDUsage; }

void Preferences::setOSDShowOwnNick(bool state) { OSDShowOwnNick=state; }
const bool Preferences::getOSDShowOwnNick() { return OSDShowOwnNick; }

void Preferences::setOSDShowChannel(bool state) { OSDShowChannel=state; }
const bool Preferences::getOSDShowChannel() { return OSDShowChannel; }

void Preferences::setOSDShowQuery(bool state) { OSDShowQuery=state; }
const bool Preferences::getOSDShowQuery() { return OSDShowQuery; }

void Preferences::setOSDShowChannelEvent(bool state) { OSDShowChannelEvent=state; }
const bool Preferences::getOSDShowChannelEvent() { return OSDShowChannelEvent; }

const QFont Preferences::getOSDFont() { return osdFont; }
void Preferences::setOSDFont(const QFont &newFont) { osdFont=newFont; }
void Preferences::setOSDFontRaw(const QString &rawFont) { osdFont.fromString(rawFont); }

/*void Preferences::setOSDColor(const QString &newColor) { osdColor.setNamedColor(newColor); }
QColor Preferences::getOSDColor() { return osdColor; }*/

void Preferences::setOSDUseCustomColors(bool state) { useOSDCustomColors = state; }
const bool Preferences::getOSDUseCustomColors() { return useOSDCustomColors; }

void Preferences::setOSDTextColor(const QString& newColor) { osdTextColor.setNamedColor(newColor); }
const QColor Preferences::getOSDTextColor() { return osdTextColor; }

void Preferences::setOSDBackgroundColor(const QString& newColor) { osdBackgroundColor.setNamedColor(newColor); }
const QColor Preferences::getOSDBackgroundColor() { return osdBackgroundColor; }

void Preferences::setOSDDuration(int ms) { OSDDuration = ms; }
const int Preferences::getOSDDuration() { return OSDDuration; }

void Preferences::setOSDScreen(uint screen) { OSDScreen = screen; }
const uint Preferences::getOSDScreen() { return OSDScreen; }

void Preferences::setOSDDrawShadow(bool state) { OSDDrawShadow = state; }
const bool Preferences::getOSDDrawShadow() { return OSDDrawShadow; }

void Preferences::setOSDOffsetX(int offset) { OSDOffsetX = offset; }
const int Preferences::getOSDOffsetX() { return OSDOffsetX; }

void Preferences::setOSDOffsetY(int offset) { OSDOffsetY = offset; }
const int Preferences::getOSDOffsetY() { return OSDOffsetY; }

void Preferences::setOSDAlignment(int alignment) { OSDAlignment = alignment; }
const int Preferences::getOSDAlignment() { return OSDAlignment; }

const QFont Preferences::getTextFont() { return textFont; }
const QFont Preferences::getListFont() { return listFont; }
void Preferences::setTextFont(const QFont &newFont) { textFont=newFont; }
void Preferences::setListFont(const QFont &newFont) { listFont=newFont; }
void Preferences::setTextFontRaw(const QString &rawFont) { textFont.fromString(rawFont); }
void Preferences::setListFontRaw(const QString &rawFont) { listFont.fromString(rawFont); }

void Preferences::setTimestamping(bool state) { timestamping=state; }
const bool Preferences::getTimestamping() { return timestamping; }
void Preferences::setShowDate(bool state) { showDate=state; }
const bool Preferences::getShowDate() { return showDate; }
void Preferences::setTimestampFormat(const QString& newFormat) { timestampFormat=newFormat; }
const QString Preferences::getTimestampFormat() { return timestampFormat; }

void Preferences::setShowQuickButtons(bool state) { showQuickButtons=state; }
const bool Preferences::getShowQuickButtons() { return showQuickButtons; }

void Preferences::setShowModeButtons(bool state) { showModeButtons=state; }
const bool Preferences::getShowModeButtons() { return showModeButtons; }

void Preferences::setShowServerList(bool state) { showServerList=state; }
const bool Preferences::getShowServerList() { return showServerList; }

void Preferences::setShowBackgroundImage(bool state) { showBackgroundImage=state; }
const bool Preferences::getShowBackgroundImage() { return showBackgroundImage; }

void Preferences::setShowTrayIcon(bool state)
{
  showTrayIcon=state;
  emit updateTrayIcon();
}
const bool Preferences::getShowTrayIcon() { return showTrayIcon; }

void Preferences::setSystrayOnly(bool state) 
{ 
  systrayOnly=state; 
  emit updateTrayIcon();
}
const bool Preferences::getSystrayOnly() { return systrayOnly; }

void Preferences::setTrayNotify(bool state)
{
  trayNotify = state;
  emit updateTrayIcon();
}

const bool Preferences::getTrayNotify() const { return trayNotify; }

void Preferences::setTrayNotifyOnlyOwnNick(bool onlyOwnNick)
{
  m_trayNotifyOnlyOwnNick = onlyOwnNick;
}

bool Preferences::trayNotifyOnlyOwnNick() const
{
  return m_trayNotifyOnlyOwnNick;
}

void Preferences::setChannelSplitter(QValueList<int> sizes) { channelSplitter=sizes; }
const QValueList<int> Preferences::getChannelSplitter() { return channelSplitter; }

void Preferences::setTopicSplitterSizes(QValueList<int> sizes)
{
  m_topicSplitterSizes = sizes;
}

const QValueList<int> Preferences::topicSplitterSizes() const
{
  return m_topicSplitterSizes;
}

void Preferences::setChannelDoubleClickAction(const QString &action) { channelDoubleClickAction=action; }
const QString Preferences::getChannelDoubleClickAction() { return channelDoubleClickAction; }

void Preferences::setNotifyDoubleClickAction(const QString &action) { notifyDoubleClickAction=action; }
const QString Preferences::getNotifyDoubleClickAction() { return notifyDoubleClickAction; }

void Preferences::setUseSpacing(bool state) { useSpacing=state; }
const bool Preferences::getUseSpacing() { return useSpacing; }
void Preferences::setSpacing(int newSpacing) { spacing=newSpacing; }
const int Preferences::getSpacing() { return spacing; }
void Preferences::setMargin(int newMargin) { margin=newMargin; }
const int Preferences::getMargin() { return margin; }

void Preferences::setUseParagraphSpacing(bool state) { useParagraphSpacing=state; }
const bool Preferences::getUseParagraphSpacing() { return useParagraphSpacing; }
void Preferences::setParagraphSpacing(int newSpacing) { paragraphSpacing=newSpacing; }
const int Preferences::getParagraphSpacing() { return paragraphSpacing; }

// sorting stuff
void Preferences::setAdminValue(int value)           { adminValue=value; }
void Preferences::setOwnerValue(int value)           { ownerValue=value; }
void Preferences::setOpValue(int value)              { opValue=value; }
void Preferences::setHalfopValue(int value)          { halfopValue=value; }
void Preferences::setVoiceValue(int value)           { voiceValue=value; }
void Preferences::setAwayValue(int value)            { awayValue=value; }
void Preferences::setNoRightsValue(int value)        { noRightsValue=value; }

const int Preferences::getAdminValue()                     { return adminValue; }
const int Preferences::getOwnerValue()                     { return ownerValue; }
const int Preferences::getOpValue()                        { return opValue; }
const int Preferences::getHalfopValue()                    { return halfopValue; }
const int Preferences::getVoiceValue()                     { return voiceValue; }
const int Preferences::getAwayValue()                      { return awayValue; }
const int Preferences::getNoRightsValue()                  { return noRightsValue; }

void Preferences::setSortCaseInsensitive(bool state) { sortCaseInsensitive=state; }
void Preferences::setSortByStatus(bool state)        { sortByStatus=state; }

const bool Preferences::getSortCaseInsensitive()           { return sortCaseInsensitive; }
const bool Preferences::getSortByStatus()                  { return sortByStatus; }

const bool Preferences::getAutoUserhost()                  { return autoUserhost; }
void Preferences::setAutoUserhost(bool state)
{
  autoUserhost=state;
  emit autoUserhostChanged(state);
}

const bool Preferences::getDialogFlag(const QString& flagName)
{
  KConfig* config=KApplication::kApplication()->config();

  config->setGroup("Notification Messages");

  if( !config->readEntry(flagName).isEmpty() )
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
    if ( config->readEntry(flagName).isEmpty() )
      config->writeEntry(flagName,"no");
  }

  config->sync();
}

void Preferences::setMaximumLagTime(int lag) { maximumLag=lag; }
const int Preferences::getMaximumLagTime()         { return maximumLag; }

void Preferences::setIRCColorList(const QStringList &cl) { ircColorList=cl; }
const QStringList Preferences::getIRCColorList()        { return ircColorList; }

void Preferences::setAliasList(const QStringList &newList) { aliasList=newList; }
const QStringList Preferences::getAliasList()             { return aliasList; }

const int Preferences::getNickCompletionMode() { return nickCompletionMode; }
void Preferences::setNickCompletionMode(int mode) { nickCompletionMode = mode; }
const QString Preferences::getPrefixCharacter() { return prefixCharacter; }
void  Preferences::setPrefixCharacter(const QString &prefix) { prefixCharacter = prefix; }
const bool Preferences::nickCompletionCaseSensitive() const { return m_nickCompletionCaseSensitive; }
void Preferences::setNickCompletionCaseSensitive(bool caseSensitive) { m_nickCompletionCaseSensitive = caseSensitive; }

const bool Preferences::getShowMenuBar() { return showMenuBar; }
void Preferences::setShowMenuBar(bool s) { showMenuBar = s; }

const bool Preferences::getShowTabBarCloseButton() { return showTabBarCloseButton; }
void Preferences::setShowTabBarCloseButton(bool s) { showTabBarCloseButton = s; }

const bool Preferences::getShowTopic() { return showTopic; }
void Preferences::setShowTopic(bool s) { showTopic = s; }

const bool Preferences::getShowRememberLineInAllWindows() { return showRememberLineInAllWindows; }
void Preferences::setShowRememberLineInAllWindows(bool s) { showRememberLineInAllWindows = s; }

const bool Preferences::getFocusNewQueries() { return focusNewQueries; }
void Preferences::setFocusNewQueries(bool s) { focusNewQueries = s; }

const bool Preferences::getHideUnimportantEvents()           { return hideUnimportantEvents; }
void Preferences::setHideUnimportantEvents(bool state) { hideUnimportantEvents=state; }

const bool Preferences::getDisableExpansion() { return disableExpansion; }
void Preferences::setDisableExpansion(bool state) { disableExpansion = state; }

// Web Browser
const bool Preferences::getWebBrowserUseKdeDefault() { return webBrowserUseKdeDefault; }
void Preferences::setWebBrowserUseKdeDefault(bool state) { webBrowserUseKdeDefault = state; }
const QString Preferences::getWebBrowserCmd() { return webBrowserCmd; }
void Preferences::setWebBrowserCmd(const QString &cmd) { webBrowserCmd=cmd; }

const bool Preferences::getFilterColors() { return filterColors; }
void Preferences::setFilterColors(bool filter) { filterColors = filter; }

const bool Preferences::getUseColoredNicks() { return useColoredNicks; }
void Preferences::setUseColoredNicks(bool useColor) { useColoredNicks=useColor; }

void Preferences::setNickColorList(const QStringList &cl) { nickColorList=cl; }
const QStringList Preferences::getNickColorList() { return nickColorList; }

const bool Preferences::getRedirectToStatusPane() { return redirectToStatusPane; }
void Preferences::setRedirectToStatusPane(bool redirect) { redirectToStatusPane = redirect; }

const bool Preferences::getOpenWatchedNicksAtStartup() { return m_openWatchedNicksAtStartup; }
void Preferences::setOpenWatchedNicksAtStartup(bool open) { m_openWatchedNicksAtStartup = open; }

// Channel Encodings
const QString Preferences::getChannelEncoding(const QString& server,const QString& channel)
{
  if(channelEncodingsMap.contains(server))
    if(channelEncodingsMap[server].contains(channel.lower()))
      return channelEncodingsMap[server][channel.lower()];
  return QString::null;
}

void Preferences::setChannelEncoding(const QString& server,const QString& channel,const QString& encoding)
{
  if(!encoding.isEmpty())
    channelEncodingsMap[server][channel.lower()]=encoding;
  else
  {
    channelEncodingsMap[server].remove(channel.lower());
    if(channelEncodingsMap[server].count()==0)
      channelEncodingsMap.remove(server);
  }
}

const QStringList Preferences::getChannelEncodingsServerList()
{
  return channelEncodingsMap.keys();
}

const QStringList Preferences::getChannelEncodingsChannelList(const QString& server)
{
  return channelEncodingsMap[server].keys();
}

void Preferences::setIconTheme(const QString& name) { iconTheme=name; }
const QString Preferences::getIconTheme() { return iconTheme; }

bool Preferences::showNicknameBox() const { return m_showNicknameBox; }
void Preferences::setShowNicknameBox(bool show) { m_showNicknameBox = show; }

#include "preferences.moc"
