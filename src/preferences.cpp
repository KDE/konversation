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

Preferences *Preferences::mSelf = 0;
static KStaticDeleter<Preferences> staticPreferencesDeleter;

Preferences *Preferences::self()
{
  if ( !mSelf ) {
    staticPreferencesDeleter.setObject( mSelf, new Preferences() );
    mSelf->readConfig();
  }

  return mSelf;
}


Preferences::Preferences()
{
    mSelf = this;
    // create default identity
    mIdentity=new Identity();
    mIdentity->setName(i18n("Default Identity"));
    addIdentity(mIdentity);

    KUser user(KUser::UseRealUserID);
    setIdent(user.loginName());
    setRealName(user.fullName());

    QStringList nickList;
    nickList.append(user.loginName());
    nickList.append("_" + user.loginName());
    nickList.append(user.loginName() + "_");
    nickList.append("_" + user.loginName() + "_");
    mIdentity->setNicknameList(nickList);

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
    mServerGroupList.append(serverGroup);

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

    setFilterColors(false);

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

    if ( mSelf == this )
        staticPreferencesDeleter.setObject( mSelf, 0, false );
}
const Konversation::ServerGroupList Preferences::serverGroupList()
{
    return mServerGroupList;
}

void Preferences::setServerGroupList(const Konversation::ServerGroupList& list)
{
    mServerGroupList.clear();
    mServerGroupList = list;
}

void Preferences::addServerGroup(Konversation::ServerGroupSettingsPtr serverGroup)
{
    mServerGroupList.append(serverGroup);
}

const Konversation::ServerGroupSettingsPtr Preferences::serverGroupById(int id)
{
    if(!m_serverGroupList.count())
    {
        return 0;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = mServerGroupList.begin(); it != mServerGroupList.end(); ++it)
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

    for(it = mServerGroupList.begin(); it != mServerGroupList.end(); ++it)
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

    for(it = mServerGroupList.begin(); it != mServerGroupList.end(); ++it)
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

    for(it = mServerGroupList.begin(); it != mServerGroupList.end(); ++it)
    {
        if((*it)->name().lower() == server.lower())
        {
            return mTrue;
        }
    }

    return mFalse;
}

void Preferences::removeServerGroup(int id)
{
    if(!m_serverGroupList.count())
    {
        return;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = mServerGroupList.begin(); it != mServerGroupList.end(); ++it)
    {
        if((*it)->id() == id)
        {
            mServerGroupList.remove(it);
            return;
        }
    }
}

const QPtrList<Highlight> Preferences::highlightList()
{
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

void Preferences::setDccSendPortsLast(unsigned long port)
{
    dccSendPortsLast=port;
    if(port < getDccSendPortsFirst())
        setDccSendPortsFirst(port);
}

void Preferences::setDccChatPortsFirst(unsigned long port)
{
    dccChatPortsFirst=port;
    if(getDccChatPortsLast() < port)
        setDccChatPortsLast(port);
}

void Preferences::setDccChatPortsLast(unsigned long port)
{
    dccChatPortsLast=port;
    if(port < getDccChatPortsFirst())
        setDccChatPortsFirst(port);
}

void Preferences::setIPv4Fallback(bool fallback) { ipv4Fallback=fallback;}
bool Preferences::iPv4Fallback() { return ipv4Fallback; }

void Preferences::setIPv4FallbackIface(const QString& interface) { ipv4Interface=interface; }
const QString& Preferences::iPv4FallbackIface() { return ipv4Interface; }

void Preferences::setNotifyList(const QMap<QString, QStringList> &newList)
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
    if (groupName.isEmpty() || newPattern.isEmpty()) return mFalse;
    if (!notifyList[groupName].contains(newPattern))
    {
        QStringList nicknameList = notifyList[groupName];
        nicknameList.append(newPattern);
        notifyList[groupName] = nicknameList;
        return mTrue;
    }
    return mFalse;
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
        return mTrue;
    } else
    return mFalse;
}

const QStringList Preferences::buttonList() { return mButtonList; }

// Default identity functions
void Preferences::addIdentity(IdentityPtr identity) { identityList.append(identity); }
void Preferences::removeIdentity(IdentityPtr identity) { identityList.remove(identity); }

void Preferences::clearIdentityList()
{
    identityList.clear();
}

const QValueList<IdentityPtr> Preferences::identityList() { return mIdentityList; }

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
const QPtrList<Ignore> Preferences::ignoreList() { return mIgnoreList; }

const QString Preferences::nickname(int index) { return identityList[0]->getNickname(index); }
const QStringList Preferences::nicknameList() { return identityList[0]->getNicknameList(); }
void Preferences::setNickname(int index,const QString &newName) { identityList[0]->setNickname(index,newName); }
void Preferences::setNicknameList(const QStringList &newList) { identityList[0]->setNicknameList(newList); }



// TODO: Make this a little simpler (use an array and enum)
//       get/set message font colors

const QString Preferences::color(const QString& name)
{
    KConfig* config=KApplication::kApplication()->config();

    config->setGroup("Message Text Colors");
    QString color=config->readEntry(name,getDefaultColor(name));

    if(color.isEmpty()) color="000000";

    return mColor;
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

// Geometry functions
const QSize Preferences::nicksOnlineSize()        { return mNicksOnlineSize; }
const QSize Preferences::nicknameSize()           { return mNicknameSize; }
const QSize Preferences::multilineEditSize()      { return mMultilineEditSize; }

void Preferences::setNicksOnlineSize(const QSize &newSize)     { mNicksOnlineSize=newSize; }
void Preferences::setNicknameSize(const QSize &newSize)        { mNicknameSize=newSize; }
void Preferences::setMultilineEditSize(const QSize &newSize)   { mMultilineEditSize=newSize; }



void Preferences::setHighlightOwnLinesColor(const QString &newColor) { highlightOwnLinesColor.setNamedColor(newColor); }

void Preferences::setUseClickableNicks(bool state) { mClickableNicks=state; }
const bool Preferences::useClickableNicks() { return mClickableNicks;}

// On Screen Display
void Preferences::setOSDUsage(bool state) { mOSDUsage=state; }
const bool Preferences::OSDUsage() { return mOSDUsage; }

void Preferences::setOSDShowOwnNick(bool state) { mOSDShowOwnNick=state; }
const bool Preferences::OSDShowOwnNick() { return mOSDShowOwnNick; }

void Preferences::setOSDShowChannel(bool state) { mOSDShowChannel=state; }
const bool Preferences::OSDShowChannel() { return mOSDShowChannel; }

void Preferences::setOSDShowQuery(bool state) { mOSDShowQuery=state; }
const bool Preferences::OSDShowQuery() { return mOSDShowQuery; }

void Preferences::setOSDShowChannelEvent(bool state) { mOSDShowChannelEvent=state; }
const bool Preferences::OSDShowChannelEvent() { return mOSDShowChannelEvent; }

const QFont Preferences::OSDFont() { return mOsdFont; }
void Preferences::setOSDFont(const QFont &newFont) { mOsdFont=newFont; }
void Preferences::setOSDFontRaw(const QString &rawFont) { osdFont.fromString(rawFont); }

/*void Preferences::setOSDColor(const QString &newColor) { osdColor.setNamedColor(newColor); }
QColor Preferences::OSDColor() { return mOsdColor; }*/

void Preferences::setOSDUseCustomColors(bool state) { useOSDCustomColors = state; }

void Preferences::setOSDTextColor(const QString& newColor) { osdTextColor.setNamedColor(newColor); }
const QColor Preferences::OSDTextColor() { return mOsdTextColor; }

void Preferences::setOSDBackgroundColor(const QString& newColor) { osdBackgroundColor.setNamedColor(newColor); }
const QColor Preferences::OSDBackgroundColor() { return mOsdBackgroundColor; }

void Preferences::setOSDDuration(int ms) { OSDDuration = ms; }

void Preferences::setOSDScreen(uint screen) { OSDScreen = screen; }

void Preferences::setOSDDrawShadow(bool state) { OSDDrawShadow = state; }

void Preferences::setOSDOffsetX(int offset) { OSDOffsetX = offset; }
const int Preferences::OSDOffsetX() { return mOSDOffsetX; }

void Preferences::setOSDOffsetY(int offset) { OSDOffsetY = offset; }
const int Preferences::OSDOffsetY() { return mOSDOffsetY; }

void Preferences::setOSDAlignment(int alignment) { OSDAlignment = alignment; }
const int Preferences::OSDAlignment() { return mOSDAlignment; }

void Preferences::setTextFontRaw(const QString &rawFont) { textFont.fromString(rawFont); }
void Preferences::setListFontRaw(const QString &rawFont) { listFont.fromString(rawFont); }






void Preferences::setShowTrayIcon(bool state)
{
    showTrayIcon=state;
    emit updateTrayIcon();
}


void Preferences::setSystrayOnly(bool state)
{
    systrayOnly=state;
    emit updateTrayIcon();
}


void Preferences::setTrayNotify(bool state)
{
    trayNotify = state;
    emit updateTrayIcon();
}


void Preferences::setTrayNotifyOnlyOwnNick(bool onlyOwnNick)
{
}

bool Preferences::trayNotifyOnlyOwnNick() const
{
}







// sorting stuff
void Preferences::setOpValue(int value)              { mOpValue=value; }

const int Preferences::opValue()                        { return mOpValue; }



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
        return mFalse;
    else
        return mTrue;
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


void Preferences::setIRCColorList(const QStringList &cl) { mIrcColorList=cl; }
const QStringList Preferences::IRCColorList()        { return mIrcColorList; }


const int Preferences::nickCompletionMode() { return mode(); }
void Preferences::setNickCompletionMode(int mode) { setMode(mode); }
const bool Preferences::nickCompletionCaseSensitive() const { return caseSensitive(); }
void Preferences::setNickCompletionCaseSensitive(bool caseSensitive) { setCaseSensitive(caseSensitive); }

const bool Preferences::showMenuBar() { return mShowMenuBar; }
void Preferences::setShowMenuBar(bool s) { showMenuBar = s; }

void Preferences::setShowTabBarCloseButton(bool s) { showTabBarCloseButton = s; }

void Preferences::setShowTopic(bool s) { showTopic = s; }

void Preferences::setShowRememberLineInAllWindows(bool s) { showRememberLineInAllWindows = s; }

void Preferences::setFocusNewQueries(bool s) { focusNewQueries = s; }


void Preferences::setDisableExpansion(bool state) { disableExpansion = state; }

// Web Browser
const bool Preferences::webBrowserUseKdeDefault() { return mWebBrowserUseKdeDefault; }
void Preferences::setWebBrowserUseKdeDefault(bool state) { webBrowserUseKdeDefault = state; }

const bool Preferences::filterColors() { return mFilterColors; }
void Preferences::setFilterColors(bool filter) { filterColors = filter; }


const bool Preferences::openWatchedNicksAtStartup() { return mOpenWatchedNicksAtStartup; }
void Preferences::setOpenWatchedNicksAtStartup(bool open) { mOpenWatchedNicksAtStartup = open; }

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



QString Preferences::wikiUrl() const { return mWikiUrl; }
void Preferences::setWikiUrl(const QString& url) { mWikiUrl=url; }

bool Preferences::expandWikiUrl() const { return mExpandWikiUrl;}
void Preferences::setExpandWikiUrl(bool expandUrl) { mExpandWikiUrl=expandUrl; }

#include "preferences.moc"
