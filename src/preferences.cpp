/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Class for application wide preferences
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
#include <qregexp.h>

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

    Konversation::ServerGroupSettingsPtr serverGroup = new Konversation::ServerGroupSettings;
    serverGroup->setName("Freenode");
    Konversation::ServerSettings server;
    server.setServer("irc.freenode.org");
    serverGroup->addServer(server);
    serverGroup->setIdentityId(identity->id());
    Konversation::ChannelSettings channel;
    channel.setName("#kde");
    serverGroup->addChannel(channel);
    m_serverGroupList.append(serverGroup);

    buttonList.append("Op,/OP %u%n");
    buttonList.append("DeOp,/DEOP %u%n");
    buttonList.append("WhoIs,/WHOIS %s,%%u%n");
    buttonList.append("Version,/CTCP %s,%%u VERSION%n");
    buttonList.append("Kick,/KICK %u%n");
    buttonList.append("Ban,/BAN %u%n");
    buttonList.append("Part,/PART %c Leaving...%n");
    buttonList.append("Quit,/QUIT Leaving...%n");

    setShowQuickButtons(false);
    setShowModeButtons(false);
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
    setReconnectCount(10);
    setAutoRejoin(true);
    setAutojoinOnInvite(false);

    setMaximumLagTime(180);

    setFixedMOTD(true);
    setBeep(false);
    setRawLog(false);

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
    setDccFastSend(true);
    setDccSendTimeout(180);
    setIPv4Fallback(false);
    setIPv4FallbackIface("eth0");

    KStandardDirs kstddir;
    setScrollbackMax(1000);

    setAutoWhoNicksLimit(200);
    setAutoWhoContinuousEnabled(true);
    setAutoWhoContinuousInterval(90);

    setShowRealNames(false);

    setLog(true);
    setLowerLog(true);
    setAddHostnameToLog(false);
    setLogFollowsNick(true);

    setLogfileBufferSize(100);
    setLogfileReaderSize(QSize(400,200));

    setTabPlacement(Bottom);
    setBlinkingTabs(false);
    setCloseButtonsOnTabs(false);
    setCloseButtonsAlignRight(false);
    setBringToFront(false);
    setFocusNewQueries(false);

    setNotifyDelay(20);
    setUseNotify(true);

    setHighlightNick(true);
    setHighlightOwnLines(false);
    setHighlightNickColor("#ff0000");
    setHighlightOwnLinesColor("#ff0000");
    setHighlightSoundEnabled(true);

    setUseClickableNicks(true);

    // On Screen Display
    setOSDUsage(false);
    setOSDShowOwnNick(false);
    setOSDShowQuery(false);
    setOSDShowChannelEvent(false);
    setOSDTextColor("#ffffff");
    setOSDDuration(3000);
    setOSDOffsetX(30);
    setOSDOffsetY(50);
    setOSDAlignment(0);                           // Left

    setColorInputFields(true);
    setBackgroundImageName(QString::null);

    setTimestamping(true);
    setShowDate(false);
    setTimestampFormat("hh:mm");

    setCommandChar("/");
    setChannelDoubleClickAction("/QUERY %u%n");
    setNotifyDoubleClickAction("/WHOIS %u%n");

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
    nickColorList.append("#000000");
    setUseColoredNicks(false);
    setUseBoldNicks(false);
    setUseLiteralModes(false);

    setNickCompletionMode(2);
    setNickCompletionCaseSensitive(false);

    setShowMenuBar(true);
    setShowTabBarCloseButton(true);

    setHideUnimportantEvents(false);
    setShowTopic(true);
    setShowNicknameBox(true);

    setShowRememberLineInAllWindows(false);

    // Web Browser
    setWebBrowserUseKdeDefault(true);
    setWebBrowserCmd("mozilla \'%u\'");

    setOpenWatchedNicksAtStartup(false);

    // Themes
    setIconTheme("default");

    setDisableNotifyWhileAway(false);

    setWikiUrl("http://en.wikipedia.org/wiki/");
    setExpandWikiUrl(false);
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

void Preferences::addServerGroup(Konversation::ServerGroupSettingsPtr serverGroup)
{
    m_serverGroupList.append(serverGroup);
}

const Konversation::ServerGroupSettingsPtr Preferences::serverGroupById(int id)
{
    if(!m_serverGroupList.count())
    {
        return 0;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = m_serverGroupList.begin(); it != m_serverGroupList.end(); ++it)
    {
        if((*it)->id() == id)
        {
            return (*it);
        }
    }

    return 0;
}

const Konversation::ServerGroupSettingsPtr Preferences::serverGroupByServer(const QString& server)
{
    if(!m_serverGroupList.count())
    {
        return 0;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = m_serverGroupList.begin(); it != m_serverGroupList.end(); ++it)
    {
        for (uint i = 0; i != (*it)->serverList().count(); i++)
        {
            if ((*it)->serverByIndex(i).server().lower() == server)
            {
                return (*it);
            }
        }
    }

    return 0;
}

int Preferences::serverGroupIdByName(const QString& serverGroup)
{
    Konversation::ServerGroupList::iterator it;

    for(it = m_serverGroupList.begin(); it != m_serverGroupList.end(); ++it)
    {
        if((*it)->name().lower() == serverGroup.lower())
        {
            return (*it)->id();
        }
    }

    return 0;
}

bool Preferences::isServerGroup(const QString& server)
{
    Konversation::ServerGroupList::iterator it;

    for(it = m_serverGroupList.begin(); it != m_serverGroupList.end(); ++it)
    {
        if((*it)->name().lower() == server.lower())
        {
            return true;
        }
    }

    return false;
}

void Preferences::removeServerGroup(int id)
{
    if(!m_serverGroupList.count())
    {
        return;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = m_serverGroupList.begin(); it != m_serverGroupList.end(); ++it)
    {
        if((*it)->id() == id)
        {
            m_serverGroupList.remove(it);
            return;
        }
    }
}

const QPtrList<Highlight> Preferences::highlightList()
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

void Preferences::setDccSendPortsFirst(unsigned long port)
{
    dccSendPortsFirst=port;
    if(getDccSendPortsLast() < port)
        setDccSendPortsLast(port);
}

const unsigned int Preferences::dccSendPortsFirst() { return dccSendPortsFirst; }

void Preferences::setDccSendPortsLast(unsigned long port)
{
    dccSendPortsLast=port;
    if(port < getDccSendPortsFirst())
        setDccSendPortsFirst(port);
}

const unsigned int Preferences::dccSendPortsLast() { return dccSendPortsLast; }

void Preferences::setDccSpecificChatPorts(bool state) { dccSpecificChatPorts=state; }
const bool Preferences::dccSpecificChatPorts() { return dccSpecificChatPorts; }

void Preferences::setDccChatPortsFirst(unsigned long port)
{
    dccChatPortsFirst=port;
    if(getDccChatPortsLast() < port)
        setDccChatPortsLast(port);
}

const unsigned int Preferences::dccChatPortsFirst() { return dccChatPortsFirst; }

void Preferences::setDccChatPortsLast(unsigned long port)
{
    dccChatPortsLast=port;
    if(port < getDccChatPortsFirst())
        setDccChatPortsFirst(port);
}

const unsigned int Preferences::dccChatPortsLast() { return dccChatPortsLast; }

void Preferences::setDccAutoGet(bool state) { dccAutoGet=state; }
const bool Preferences::dccAutoGet() { return dccAutoGet; }

void Preferences::setDccAutoResume(bool state) { dccAutoResume=state; }
const bool Preferences::dccAutoResume() { return dccAutoResume; }

void Preferences::setDccPath(const QString &path) { dccPath=path; }
const QString Preferences::dccPath() { return dccPath; }

void Preferences::setDccFastSend(bool state) { dccFastSend=state; }
const bool Preferences::dccFastSend() { return dccFastSend; }

void Preferences::setDccSendTimeout(int sec) { dccSendTimeout=sec; }
const int Preferences::dccSendTimeout() { return dccSendTimeout; }

void Preferences::setIPv4Fallback(bool fallback) { ipv4Fallback=fallback;}
bool Preferences::iPv4Fallback() { return ipv4Fallback; }

void Preferences::setIPv4FallbackIface(const QString& interface) { ipv4Interface=interface; }
const QString& Preferences::iPv4FallbackIface() { return ipv4Interface; }

void Preferences::setNotifyList(const QMap<QString, QStringList> &newList)
{ notifyList=newList; }
const QMap<QString, QStringList> Preferences::notifyList() { return notifyList; }
const QStringList Preferences::notifyListByGroup(const QString& groupName)
{
    if (notifyList.find(groupName) != notifyList.end())
        return notifyList[groupName];
    else
        return QStringList();
}

const QString Preferences::notifyStringByGroup(const QString& groupName)
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

const QStringList Preferences::buttonList() { return buttonList; }

// Default identity functions
void Preferences::addIdentity(IdentityPtr identity) { identityList.append(identity); }
void Preferences::removeIdentity(IdentityPtr identity) { identityList.remove(identity); }

void Preferences::clearIdentityList()
{
    identityList.clear();
}

const QValueList<IdentityPtr> Preferences::identityList() { return identityList; }

void Preferences::setIdentityList(const QValueList<IdentityPtr>& list)
{
    identityList.clear();
    identityList = list;
}

const IdentityPtr Preferences::identityByName(const QString& name)
{
    QValueList<IdentityPtr> identities = getIdentityList();
    QValueList<IdentityPtr>::iterator it = identities.begin();

    while(it != identities.end())
    {
        if((*it)->getName() == name)
        {
            return (*it);
        }

        ++it;
    }

    // no matching identity found, return default identity
    return identities.first();
}

const IdentityPtr Preferences::identityById(int id)
{
    QValueList<IdentityPtr> identityList = getIdentityList();
    for(QValueList<IdentityPtr>::iterator it = identityList.begin(); it != identityList.end(); ++it)
    {
        if((*it)->id() == id)
        {
            return (*it);
        }
    }

    return identityList.first();
}

const QString Preferences::realName() { return identityList[0]->getRealName(); }
void Preferences::setRealName(const QString &name) { identityList[0]->setRealName(name); }

const QString Preferences::ident() { return identityList[0]->getIdent(); }
void Preferences::setIdent(const QString &ident) { identityList[0]->setIdent(ident); }

const QString Preferences::partReason() { return identityList[0]->getPartReason(); }
void Preferences::setPartReason(const QString &newReason) { identityList[0]->setPartReason(newReason); }

const QString Preferences::kickReason() { return identityList[0]->getKickReason(); }
void Preferences::setKickReason(const QString &newReason) { identityList[0]->setKickReason(newReason); }

const bool Preferences::showAwayMessage() { return identityList[0]->getShowAwayMessage(); }
void Preferences::setShowAwayMessage(bool state) { identityList[0]->setShowAwayMessage(state); }

const QString Preferences::awayMessage() { return identityList[0]->getAwayMessage(); }
void Preferences::setAwayMessage(const QString &newMessage) { identityList[0]->setAwayMessage(newMessage); }
const QString Preferences::unAwayMessage() { return identityList[0]->getReturnMessage(); }
void Preferences::setUnAwayMessage(const QString &newMessage) { identityList[0]->setReturnMessage(newMessage); }

void Preferences::clearIgnoreList() { ignoreList.clear(); }
const QPtrList<Ignore> Preferences::ignoreList() { return ignoreList; }

const QString Preferences::nickname(int index) { return identityList[0]->getNickname(index); }
const QStringList Preferences::nicknameList() { return identityList[0]->getNicknameList(); }
void Preferences::setNickname(int index,const QString &newName) { identityList[0]->setNickname(index,newName); }
void Preferences::setNicknameList(const QStringList &newList) { identityList[0]->setNicknameList(newList); }

void Preferences::setTabPlacement(TabPlacement where) { tabPlacement=where; }
const Preferences::TabPlacement Preferences::tabPlacement() { return tabPlacement; }


// TODO: Make this a little simpler (use an array and enum)
//       get/set message font colors

const QString Preferences::color(const QString& name)
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

const QString Preferences::defaultColor(const QString& name)
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

const QString Preferences::nickCompleteSuffixStart() {return nickCompleteSuffixStart; }
const QString Preferences::nickCompleteSuffixMiddle() {return nickCompleteSuffixMiddle; }

void Preferences::setNickCompleteSuffixStart(const QString &suffix) { nickCompleteSuffixStart=suffix; }
void Preferences::setNickCompleteSuffixMiddle(const QString &suffix) { nickCompleteSuffixMiddle=suffix; }

const int Preferences::logfileBufferSize()             { return logfileBufferSize; }
void Preferences::setLogfileBufferSize(int newSize) { logfileBufferSize=newSize; }

// Geometry functions
const QSize Preferences::nicksOnlineSize()        { return nicksOnlineSize; }
const QSize Preferences::nicknameSize()           { return nicknameSize; }
const QSize Preferences::logfileReaderSize()      { return logfileReaderSize; }
const QSize Preferences::multilineEditSize()      { return multilineEditSize; }

void Preferences::setNicksOnlineSize(const QSize &newSize)     { nicksOnlineSize=newSize; }
void Preferences::setNicknameSize(const QSize &newSize)        { nicknameSize=newSize; }
void Preferences::setLogfileReaderSize(const QSize &newSize)   { logfileReaderSize=newSize; }
void Preferences::setMultilineEditSize(const QSize &newSize)   { multilineEditSize=newSize; }

void Preferences::setHighlightNick(bool state) { highlightNick=state; }
const bool Preferences::highlightNick() { return highlightNick; }

void Preferences::setHighlightOwnLines(bool state) { highlightOwnLines=state; }
const bool Preferences::highlightOwnLines() { return highlightOwnLines; }

void Preferences::setHighlightOwnLinesColor(const QString &newColor) { highlightOwnLinesColor.setNamedColor(newColor); }
const QColor Preferences::highlightOwnLinesColor() { return highlightOwnLinesColor; }

void Preferences::setUseClickableNicks(bool state) { clickableNicks=state; }
const bool Preferences::useClickableNicks() { return clickableNicks;}

// On Screen Display
void Preferences::setOSDUsage(bool state) { OSDUsage=state; }
const bool Preferences::OSDUsage() { return OSDUsage; }

void Preferences::setOSDShowOwnNick(bool state) { OSDShowOwnNick=state; }
const bool Preferences::OSDShowOwnNick() { return OSDShowOwnNick; }

void Preferences::setOSDShowChannel(bool state) { OSDShowChannel=state; }
const bool Preferences::OSDShowChannel() { return OSDShowChannel; }

void Preferences::setOSDShowQuery(bool state) { OSDShowQuery=state; }
const bool Preferences::OSDShowQuery() { return OSDShowQuery; }

void Preferences::setOSDShowChannelEvent(bool state) { OSDShowChannelEvent=state; }
const bool Preferences::OSDShowChannelEvent() { return OSDShowChannelEvent; }

const QFont Preferences::OSDFont() { return osdFont; }
void Preferences::setOSDFont(const QFont &newFont) { osdFont=newFont; }
void Preferences::setOSDFontRaw(const QString &rawFont) { osdFont.fromString(rawFont); }

/*void Preferences::setOSDColor(const QString &newColor) { osdColor.setNamedColor(newColor); }
QColor Preferences::OSDColor() { return osdColor; }*/

void Preferences::setOSDUseCustomColors(bool state) { useOSDCustomColors = state; }
const bool Preferences::OSDUseCustomColors() { return useOSDCustomColors; }

void Preferences::setOSDTextColor(const QString& newColor) { osdTextColor.setNamedColor(newColor); }
const QColor Preferences::OSDTextColor() { return osdTextColor; }

void Preferences::setOSDBackgroundColor(const QString& newColor) { osdBackgroundColor.setNamedColor(newColor); }
const QColor Preferences::OSDBackgroundColor() { return osdBackgroundColor; }

void Preferences::setOSDDuration(int ms) { OSDDuration = ms; }
const int Preferences::OSDDuration() { return OSDDuration; }

void Preferences::setOSDScreen(uint screen) { OSDScreen = screen; }
const uint Preferences::OSDScreen() { return OSDScreen; }

void Preferences::setOSDDrawShadow(bool state) { OSDDrawShadow = state; }
const bool Preferences::OSDDrawShadow() { return OSDDrawShadow; }

void Preferences::setOSDOffsetX(int offset) { OSDOffsetX = offset; }
const int Preferences::OSDOffsetX() { return OSDOffsetX; }

void Preferences::setOSDOffsetY(int offset) { OSDOffsetY = offset; }
const int Preferences::OSDOffsetY() { return OSDOffsetY; }

void Preferences::setOSDAlignment(int alignment) { OSDAlignment = alignment; }
const int Preferences::OSDAlignment() { return OSDAlignment; }

const QFont Preferences::textFont() { return textFont; }
const QFont Preferences::listFont() { return listFont; }
void Preferences::setTextFont(const QFont &newFont) { textFont=newFont; }
void Preferences::setListFont(const QFont &newFont) { listFont=newFont; }
void Preferences::setTextFontRaw(const QString &rawFont) { textFont.fromString(rawFont); }
void Preferences::setListFontRaw(const QString &rawFont) { listFont.fromString(rawFont); }

void Preferences::setTimestamping(bool state) { timestamping=state; }
const bool Preferences::timestamping() { return timestamping; }
void Preferences::setShowDate(bool state) { showDate=state; }
const bool Preferences::showDate() { return showDate; }
void Preferences::setTimestampFormat(const QString& newFormat) { timestampFormat=newFormat; }
const QString Preferences::timestampFormat() { return timestampFormat; }

void Preferences::setShowQuickButtons(bool state) { showQuickButtons=state; }
const bool Preferences::showQuickButtons() { return showQuickButtons; }

void Preferences::setShowModeButtons(bool state) { showModeButtons=state; }
const bool Preferences::showModeButtons() { return showModeButtons; }

void Preferences::setShowServerList(bool state) { showServerList=state; }
const bool Preferences::showServerList() { return showServerList; }

void Preferences::setShowBackgroundImage(bool state) { showBackgroundImage=state; }
const bool Preferences::showBackgroundImage() { return showBackgroundImage; }

void Preferences::setShowTrayIcon(bool state)
{
    showTrayIcon=state;
    emit updateTrayIcon();
}

const bool Preferences::showTrayIcon() { return showTrayIcon; }

void Preferences::setSystrayOnly(bool state)
{
    systrayOnly=state;
    emit updateTrayIcon();
}

const bool Preferences::systrayOnly() { return systrayOnly; }

void Preferences::setTrayNotify(bool state)
{
    trayNotify = state;
    emit updateTrayIcon();
}

const bool Preferences::trayNotify() const { return trayNotify; }

void Preferences::setTrayNotifyOnlyOwnNick(bool onlyOwnNick)
{
    m_trayNotifyOnlyOwnNick = onlyOwnNick;
}

bool Preferences::trayNotifyOnlyOwnNick() const
{
    return m_trayNotifyOnlyOwnNick;
}

void Preferences::setChannelSplitterSizes(QValueList<int> sizes) { channelSplitter=sizes; }
const QValueList<int> Preferences::channelSplitterSizes() { return channelSplitter; }

void Preferences::setTopicSplitterSizes(QValueList<int> sizes) { m_topicSplitterSizes=sizes; }
const QValueList<int> Preferences::topicSplitterSizes() { return m_topicSplitterSizes; }

void Preferences::setChannelDoubleClickAction(const QString &action) { channelDoubleClickAction=action; }
const QString Preferences::channelDoubleClickAction() { return channelDoubleClickAction; }

void Preferences::setNotifyDoubleClickAction(const QString &action) { notifyDoubleClickAction=action; }
const QString Preferences::notifyDoubleClickAction() { return notifyDoubleClickAction; }

void Preferences::setUseSpacing(bool state) { useSpacing=state; }
const bool Preferences::useSpacing() { return useSpacing; }
void Preferences::setSpacing(int newSpacing) { spacing=newSpacing; }
const int Preferences::spacing() { return spacing; }
void Preferences::setMargin(int newMargin) { margin=newMargin; }
const int Preferences::margin() { return margin; }

void Preferences::setUseParagraphSpacing(bool state) { useParagraphSpacing=state; }
const bool Preferences::useParagraphSpacing() { return useParagraphSpacing; }
void Preferences::setParagraphSpacing(int newSpacing) { paragraphSpacing=newSpacing; }
const int Preferences::paragraphSpacing() { return paragraphSpacing; }

// sorting stuff
void Preferences::setAdminValue(int value)           { adminValue=value; }
void Preferences::setOwnerValue(int value)           { ownerValue=value; }
void Preferences::setOpValue(int value)              { opValue=value; }
void Preferences::setHalfopValue(int value)          { halfopValue=value; }
void Preferences::setVoiceValue(int value)           { voiceValue=value; }
void Preferences::setNoRightsValue(int value)        { noRightsValue=value; }

const int Preferences::adminValue()                     { return adminValue; }
const int Preferences::ownerValue()                     { return ownerValue; }
const int Preferences::opValue()                        { return opValue; }
const int Preferences::halfopValue()                    { return halfopValue; }
const int Preferences::voiceValue()                     { return voiceValue; }
const int Preferences::noRightsValue()                  { return noRightsValue; }

void Preferences::setSortCaseInsensitive(bool state) { sortCaseInsensitive=state; }
void Preferences::setSortByStatus(bool state)        { sortByStatus=state; }

const bool Preferences::sortCaseInsensitive()           { return sortCaseInsensitive; }
const bool Preferences::sortByStatus()                  { return sortByStatus; }

const bool Preferences::autoUserhost()                  { return autoUserhost; }
void Preferences::setAutoUserhost(bool state)
{
    autoUserhost=state;
    emit autoUserhostChanged(state);
}

const bool Preferences::dialogFlag(const QString& flagName)
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
const int Preferences::maximumLagTime()         { return maximumLag; }

void Preferences::setIRCColorList(const QStringList &cl) { ircColorList=cl; }
const QStringList Preferences::iRCColorList()        { return ircColorList; }

void Preferences::setAliasList(const QStringList &newList) { aliasList=newList; }
const QStringList Preferences::aliasList()             { return aliasList; }

const int Preferences::nickCompletionMode() { return nickCompletionMode; }
void Preferences::setNickCompletionMode(int mode) { nickCompletionMode = mode; }
const QString Preferences::prefixCharacter() { return prefixCharacter; }
void  Preferences::setPrefixCharacter(const QString &prefix) { prefixCharacter = prefix; }
const bool Preferences::nickCompletionCaseSensitive() const { return m_nickCompletionCaseSensitive; }
void Preferences::setNickCompletionCaseSensitive(bool caseSensitive) { m_nickCompletionCaseSensitive = caseSensitive; }

const bool Preferences::showMenuBar() { return showMenuBar; }
void Preferences::setShowMenuBar(bool s) { showMenuBar = s; }

const bool Preferences::showTabBarCloseButton() { return showTabBarCloseButton; }
void Preferences::setShowTabBarCloseButton(bool s) { showTabBarCloseButton = s; }

const bool Preferences::showTopic() { return showTopic; }
void Preferences::setShowTopic(bool s) { showTopic = s; }

const bool Preferences::showRememberLineInAllWindows() { return showRememberLineInAllWindows; }
void Preferences::setShowRememberLineInAllWindows(bool s) { showRememberLineInAllWindows = s; }

const bool Preferences::focusNewQueries() { return focusNewQueries; }
void Preferences::setFocusNewQueries(bool s) { focusNewQueries = s; }

const bool Preferences::hideUnimportantEvents()           { return hideUnimportantEvents; }
void Preferences::setHideUnimportantEvents(bool state) { hideUnimportantEvents=state; }

const bool Preferences::disableExpansion() { return disableExpansion; }
void Preferences::setDisableExpansion(bool state) { disableExpansion = state; }

// Web Browser
const bool Preferences::webBrowserUseKdeDefault() { return webBrowserUseKdeDefault; }
void Preferences::setWebBrowserUseKdeDefault(bool state) { webBrowserUseKdeDefault = state; }
const QString Preferences::webBrowserCmd() { return webBrowserCmd; }
void Preferences::setWebBrowserCmd(const QString &cmd) { webBrowserCmd=cmd; }

const bool Preferences::filterColors() { return filterColors; }
void Preferences::setFilterColors(bool filter) { filterColors = filter; }

const bool Preferences::useColoredNicks() { return useColoredNicks; }
void Preferences::setUseColoredNicks(bool useColor) { useColoredNicks=useColor; }

void Preferences::setNickColorList(const QStringList &cl) { nickColorList = cl; }
const QStringList Preferences::nickColorList() { return nickColorList; }

void Preferences::setUseBoldNicks(bool boldNicks) { useBoldNicks=boldNicks; }
const bool Preferences::useBoldNicks() { return useBoldNicks; }

void Preferences::setUseLiteralModes(bool literalModes) { useLiteralModes=literalModes; }
const bool Preferences::useLiteralModes() { return useLiteralModes; }

const bool Preferences::openWatchedNicksAtStartup() { return m_openWatchedNicksAtStartup; }
void Preferences::setOpenWatchedNicksAtStartup(bool open) { m_openWatchedNicksAtStartup = open; }

// Channel Encodings
const QString Preferences::channelEncoding(const QString& server,const QString& channel)
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

const QStringList Preferences::channelEncodingsServerList()
{
    return channelEncodingsMap.keys();
}

const QStringList Preferences::channelEncodingsChannelList(const QString& server)
{
    return channelEncodingsMap[server].keys();
}

void Preferences::setIconTheme(const QString& name) { iconTheme=name; }
const QString Preferences::iconTheme() { return iconTheme; }

bool Preferences::showNicknameBox() const { return m_showNicknameBox; }
void Preferences::setShowNicknameBox(bool show) { m_showNicknameBox = show; }

QString Preferences::wikiUrl() const { return wikiUrl; }
void Preferences::setWikiUrl(const QString& url) { wikiUrl=url; }

bool Preferences::expandWikiUrl() const { return expandWikiUrl;}
void Preferences::setExpandWikiUrl(bool expandUrl) { expandWikiUrl=expandUrl; }
#endif

#include "preferences.moc"
