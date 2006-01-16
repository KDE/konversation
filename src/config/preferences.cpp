/*
  This program is free software; you can redistribute it and/or self()->modify
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
#include <kstaticdeleter.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kurl.h>
#include <kuser.h>

#include <qpalette.h>
#include <qregexp.h>
#include <qfileinfo.h>

#include "config/preferences.h"
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
    Konversation::ServerGroupSettingsPtr serverGroup = new Konversation::ServerGroupSettings;
    serverGroup->setName("Freenode");
    Konversation::ServerSettings server;
    server.setServer("irc.freenode.org");
    serverGroup->addServer(server);
    serverGroup->setIdentityId(mIdentity->id());
    Konversation::ChannelSettings channel;
    channel.setName("#kde");
    serverGroup->addChannel(channel);
    serverGroup->setExpanded(false);
    mServerGroupList.append(serverGroup);
    QStringList quickButtons=QStringList() << "Op,/OP %u%n"
                                           << "DeOp,/DEOP %u%n"
                                           << "WhoIs,/WHOIS %s,%%u%n"
                                           << "Version,/CTCP %s,%%u VERSION%n"
                                           << "Kick,/KICK %u%n"
                                           << "Ban,/BAN %u%n"
                                           << "Part,/PART %c Leaving...%n"
                                           << "Quit,/QUIT Leaving...%n";
    setQuickButtonList(quickButtons);

    // nicklist sorting order defaults
    setNicknameSortingOrder(defaultNicknameSortingOrder());

    /*
    setOwnerValue(32);
    setAdminValue(16);
    setOpValue(8);
    setHalfopValue(4);
    setVoiceValue(2);
    setNoRightsValue(1);
    */
}

Preferences::~Preferences()
{
    clearIdentityList();

    if ( mSelf == this )
        staticPreferencesDeleter.setObject( mSelf, 0, false );
}
const Konversation::ServerGroupList Preferences::serverGroupList()
{
    return self()->mServerGroupList;
}

const QStringList Preferences::quickButtonList()
{
  return self()->mQuickButtonList;
}

void Preferences::setQuickButtonList(const QStringList newList)
{
  self()->mQuickButtonList=newList;
}

void Preferences::setServerGroupList(const Konversation::ServerGroupList& list)
{
    self()->mServerGroupList.clear();
    self()->mServerGroupList = list;
}

void Preferences::addServerGroup(Konversation::ServerGroupSettingsPtr serverGroup)
{
    self()->mServerGroupList.append(serverGroup);
}

const Konversation::ServerGroupSettingsPtr Preferences::serverGroupById(int id)
{
    if(!self()->mServerGroupList.count())
    {
        return 0;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = self()->mServerGroupList.begin(); it != self()->mServerGroupList.end(); ++it)
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
    if(!self()->mServerGroupList.count())
    {
        return 0;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = self()->mServerGroupList.begin(); it != self()->mServerGroupList.end(); ++it)
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

    for(it = self()->mServerGroupList.begin(); it != self()->mServerGroupList.end(); ++it)
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

    for(it = self()->mServerGroupList.begin(); it != self()->mServerGroupList.end(); ++it)
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
    if(!self()->mServerGroupList.count())
    {
        return;
    }

    Konversation::ServerGroupList::iterator it;

    for(it = self()->mServerGroupList.begin(); it != self()->mServerGroupList.end(); ++it)
    {
        if((*it)->id() == id)
        {
            self()->mServerGroupList.remove(it);
            return;
        }
    }
}


const QPtrList<Highlight> Preferences::highlightList()
{
    return self()->mHighlightList;
}

void Preferences::setHighlightList(QPtrList<Highlight> newList)
{
    self()->mHighlightList.clear();
    self()->mHighlightList=newList;
}

void Preferences::addHighlight(const QString& newHighlight,
bool regExp,
const QColor &newColor,
const QString& sound,
const QString& autoText)
{
    self()->mHighlightList.append(new Highlight(newHighlight,regExp,newColor,KURL(sound),autoText));
}

void Preferences::setIgnoreList(QPtrList<Ignore> newList)
{
    self()->mIgnoreList.clear();
    self()->mIgnoreList=newList;
}

void Preferences::addIgnore(const QString &newIgnore)
{
    QStringList ignore = QStringList::split(',',newIgnore);
    removeIgnore(ignore[0]);
    self()->mIgnoreList.append(new Ignore(ignore[0],ignore[1].toInt()));
}

bool Preferences::removeIgnore(const QString &oldIgnore)
{
    QPtrListIterator<Ignore> ignoreList( self()->mIgnoreList );
    while (ignoreList.current())
    {
        if (ignoreList.current()->getName()==oldIgnore)
        {
            self()->mIgnoreList.remove(ignoreList.current());
            return true;
        }
        ++ignoreList;
    }

    return false;
}

void Preferences::setNotifyList(const QMap<int, QStringList> &newList)
{ self()->mNotifyList=newList; }

const QMap<int, QStringList> Preferences::notifyList() { return self()->mNotifyList; }

const QStringList Preferences::notifyListByGroupName(const QString& groupName)
{
  int id=serverGroupIdByName(groupName);
  if (id && self()->mNotifyList.find(id) != self()->mNotifyList.end())
        return self()->mNotifyList[id];
    else
        return QStringList();
}

const QString Preferences::notifyStringByGroupName(const QString& groupName)
{
    return notifyListByGroupName(groupName).join(" ");
}

const bool Preferences::addNotify(const QString& groupName, const QString& newPattern)
{
    // don't add duplicates
    if (groupName.isEmpty() || newPattern.isEmpty()) return false;

    int id=serverGroupIdByName(groupName);
    if(!id) return false;

    if (!self()->mNotifyList[id].contains(newPattern))
    {
        QStringList nicknameList = self()->mNotifyList[id];
        nicknameList.append(newPattern);
        self()->mNotifyList[id] = nicknameList;
        return true;
    }
    return false;
}

const bool Preferences::removeNotify(const QString& groupName, const QString& pattern)
{
  int id=serverGroupIdByName(groupName);
  if(!id) return false;

  if (self()->mNotifyList.find(id) != self()->mNotifyList.end())
    {
        QStringList nicknameList = self()->mNotifyList[id];
        nicknameList.remove(pattern);
        if (nicknameList.isEmpty())
            self()->mNotifyList.remove(id);
        else
            self()->mNotifyList[id] = nicknameList;
        return true;
    }
    return false;
}

// Default identity functions
void Preferences::addIdentity(IdentityPtr identity) { self()->mIdentityList.append(identity); }
void Preferences::removeIdentity(IdentityPtr identity) { self()->mIdentityList.remove(identity); }

void Preferences::clearIdentityList()
{
    self()->mIdentityList.clear();
}

const QValueList<IdentityPtr> Preferences::identityList() { return self()->mIdentityList; }

void Preferences::setIdentityList(const QValueList<IdentityPtr>& list)
{
    self()->mIdentityList.clear();
    self()->mIdentityList = list;
}

const IdentityPtr Preferences::identityByName(const QString& name)
{
    QValueList<IdentityPtr> identities = identityList();
    QValueList<IdentityPtr>::iterator it = identities.begin();

    while(it != identities.end())
    {
        if((*it)->getName() == name)
        {
            return (*it);
        }

        ++it;
    }

    // no self()->matching identity found, return default identity
    return identities.first();
}

const IdentityPtr Preferences::identityById(int id)
{
    QValueList<IdentityPtr> identList = identityList();
    for(QValueList<IdentityPtr>::iterator it = identList.begin(); it != identList.end(); ++it)
    {
        if((*it)->id() == id)
        {
            return (*it);
        }
    }

    return identList.first();
}

QStringList Preferences::defaultAliasList()
{
        // Auto-alias scripts
        QStringList scripts = KGlobal::dirs()->findAllResources("data","konversation/scripts/*");
        QFileInfo* fileInfo = new QFileInfo();
        QStringList aliasList;
	QString newAlias;

        for ( QStringList::ConstIterator it = scripts.begin(); it != scripts.end(); ++it )
        {
            fileInfo->setFile( *it );
            if ( fileInfo->isExecutable() )
            {
                newAlias = (*it).section('/',-1)+" "+"/exec "+(*it).section('/', -1 );
                aliasList.append(newAlias);
            }
        }
	return aliasList;
}


const QString Preferences::realName() { return self()->mIdentityList[0]->getRealName(); }
void Preferences::setRealName(const QString &name) { self()->mIdentityList[0]->setRealName(name); }

const QString Preferences::ident() { return self()->mIdentityList[0]->getIdent(); }
void Preferences::setIdent(const QString &ident) { self()->mIdentityList[0]->setIdent(ident); }

const QString Preferences::partReason() { return self()->mIdentityList[0]->getPartReason(); }
void Preferences::setPartReason(const QString &newReason) { self()->mIdentityList[0]->setPartReason(newReason); }

const QString Preferences::kickReason() { return self()->mIdentityList[0]->getKickReason(); }
void Preferences::setKickReason(const QString &newReason) { self()->mIdentityList[0]->setKickReason(newReason); }

const bool Preferences::showAwayMessage() { return self()->mIdentityList[0]->getShowAwayMessage(); }
void Preferences::setShowAwayMessage(bool state) { self()->mIdentityList[0]->setShowAwayMessage(state); }

const QString Preferences::awayMessage() { return self()->mIdentityList[0]->getAwayMessage(); }
void Preferences::setAwayMessage(const QString &newMessage) { self()->mIdentityList[0]->setAwayMessage(newMessage); }
const QString Preferences::unAwayMessage() { return self()->mIdentityList[0]->getReturnMessage(); }
void Preferences::setUnAwayMessage(const QString &newMessage) { self()->mIdentityList[0]->setReturnMessage(newMessage); }

void Preferences::clearIgnoreList() { self()->mIgnoreList.clear(); }
const QPtrList<Ignore> Preferences::ignoreList() { return self()->mIgnoreList; }

const QString Preferences::nickname(int index) { return self()->mIdentityList[0]->getNickname(index); }
const QStringList Preferences::nicknameList() { return self()->mIdentityList[0]->getNicknameList(); }
void Preferences::setNickname(int index,const QString &newName) { self()->mIdentityList[0]->setNickname(index,newName); }
void Preferences::setNicknameList(const QStringList &newList) { self()->mIdentityList[0]->setNicknameList(newList); }

void Preferences::setShowTrayIcon(bool state)
{
    PreferencesBase::setShowTrayIcon(state);
    emit self()->updateTrayIcon();
}


void Preferences::setSystrayOnly(bool state)
{
    PreferencesBase::setSystrayOnly(state);
    emit self()->updateTrayIcon();
}


void Preferences::setTrayNotify(bool state)
{
    PreferencesBase::setTrayNotify(state);
    emit self()->updateTrayIcon();
}


void Preferences::setAutoUserhost(bool state)
{
    PreferencesBase::setAutoUserhost(state);
}

const bool Preferences::dialogFlag(const QString& flagName)
{
    KConfig* config=KApplication::kApplication()->config();

    config->setGroup("Notification self()->Messages");

    if( !config->readEntry(flagName).isEmpty() )
        return false;
    else
        return true;
}

void Preferences::setDialogFlag(const QString& flagName,bool state)
{
    KConfig* config=KApplication::kApplication()->config();

    config->setGroup("Notification self()->Messages");

    if(state)
        config->deleteEntry(flagName);
    else
    {
        if ( config->readEntry(flagName).isEmpty() )
            config->writeEntry(flagName,"no");
    }

    config->sync();
}


// Channel Encodings
const QString Preferences::channelEncoding(const QString& server,const QString& channel)
{
    if(self()->mChannelEncodingsMap.contains(server))
        if(self()->mChannelEncodingsMap[server].contains(channel.lower()))
            return self()->mChannelEncodingsMap[server][channel.lower()];
    return QString::null;
}

void Preferences::setChannelEncoding(const QString& server,const QString& channel,const QString& encoding)
{
    if(!encoding.isEmpty())
        self()->mChannelEncodingsMap[server][channel.lower()]=encoding;
    else
    {
        self()->mChannelEncodingsMap[server].remove(channel.lower());
        if(self()->mChannelEncodingsMap[server].count()==0)
            self()->mChannelEncodingsMap.remove(server);
    }
}

const QStringList Preferences::channelEncodingsServerList()
{
    return self()->mChannelEncodingsMap.keys();
}

const QStringList Preferences::channelEncodingsChannelList(const QString& server)
{
    return self()->mChannelEncodingsMap[server].keys();
}

const QString Preferences::defaultNicknameSortingOrder()
{
  return "qpohv-";
}

const QString Preferences::nicknameSortingOrder()
{
  return self()->mSortingOrder;
}

void Preferences::setNicknameSortingOrder(const QString newSortingOrder)
{
  self()->mSortingOrder=newSortingOrder;
}

QString Preferences::translatedWikiURL() {
  QString wikiUrl = wikiURL();
  if(wikiUrl.isEmpty() /*indicates to use localised version*/ || wikiUrl == "http://en.wikipedia.org/wiki/" /* pre 0.19 default.*/) 
    return i18n("Translate to localised wikipedia url.  Search term is appended on the end", "http://en.wikipedia.org/wiki/Special:Search?go=Go&search=");
  return wikiUrl;
}

#include "preferences.moc"
