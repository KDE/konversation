/*
  This program is free software; you can redistribute it and/or self()->modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
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
    nickList.append(user.loginName() + '_');
    nickList.append(user.loginName() + "__");
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
    setQuickButtonList(defaultQuickButtonList());
    setAutoreplaceList(defaultAutoreplaceList());
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

const QStringList Preferences::defaultQuickButtonList()
{
    return QStringList() << "Op,/OP %u%n"
                         << "DeOp,/DEOP %u%n"
                         << "WhoIs,/WHOIS %s,%%u%n"
                         << "Version,/CTCP %s,%%u VERSION%n"
                         << "Kick,/KICK %u%n"
                         << "Ban,/BAN %u%n"
                         << "Part,/PART %c Leaving...%n"
                         << "Quit,/QUIT Leaving...%n";
}

const QStringList Preferences::quickButtonList()
{
  return self()->mQuickButtonList;
}

void Preferences::setQuickButtonList(const QStringList newList)
{
  self()->mQuickButtonList=newList;
}

void Preferences::clearQuickButtonList()
{
  self()->mQuickButtonList.clear();
}

// --------------------------- AutoReplace ---------------------------

const QStringList Preferences::defaultAutoreplaceList()
{
    return QStringList() << "1,o,\\[\\[([^\\s]+)\\]\\],http://en.wikipedia.org/wiki/Special:Search?go=Go&search=%1"
                         << "1,o,(BUG:|bug:)([0-9]+),http://bugs.kde.org/show_bug.cgi?id=%2";
}

const QStringList Preferences::autoreplaceList()
{
  return self()->mAutoreplaceList;
}

void Preferences::setAutoreplaceList(const QStringList newList)
{
  self()->mAutoreplaceList=newList;
}

void Preferences::clearAutoreplaceList()
{
  self()->mAutoreplaceList.clear();
}

// --------------------------- AutoReplace ---------------------------

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
        if (ignoreList.current()->getName().lower()==oldIgnore.lower())
        {
            self()->mIgnoreList.remove(ignoreList.current());
            return true;
        }
        ++ignoreList;
    }

    return false;
}

bool Preferences::isIgnored(const QString &nickname)
{
    QPtrListIterator<Ignore> ignoreList( self()->mIgnoreList );

    while (ignoreList.current())
    {
        if (ignoreList.current()->getName().section('!',0,0).lower()==nickname.lower())
        {
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

const bool Preferences::addNotify(int serverGroupId, const QString& newPattern)
{
    if (!self()->mNotifyList[serverGroupId].contains(newPattern))
    {
        QStringList nicknameList = self()->mNotifyList[serverGroupId];
        nicknameList.append(newPattern);
        self()->mNotifyList[serverGroupId] = nicknameList;
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

const bool Preferences::isNotify(int serverGroupId, const QString& pattern)
{
    if (self()->mNotifyList.find(serverGroupId) != self()->mNotifyList.end())
    {
        QStringList nicknameList = self()->mNotifyList[serverGroupId];

        if (nicknameList.contains(pattern)) return true;
    }
    return false;
}

const bool Preferences::hasNotifyList(int serverGroupId)
{
    if (self()->mNotifyList.find(serverGroupId) != self()->mNotifyList.end())
        return true;
    else
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

    for (QStringList::ConstIterator it = scripts.begin(); it != scripts.end(); ++it)
    {
        fileInfo->setFile(*it);
        if (fileInfo->isExecutable())
        {
            newAlias = (*it).section('/',-1)+' '+"/exec "+(*it).section('/', -1 );
            aliasList.append(newAlias);

            // FIXME: Historically, defaultAliasList() is primarily used to dynamically
            // compile a list of installed scripts and generate appropriate aliases for
            // them. It's not only used when the alias preferences are reset or initia-
            // lized, but also on application start. The following crudely adds two
            // aliases when the 'media' script is found, to provide easy access to its
            // capability to differentiate  between audio and video media. This method
            // needs at the very least to be split up in two, or scripts may in the
            // future determine what aliases they want to add.
            if ((*it).section('/',-1) == "media")
            {
                aliasList.append("audio /exec media audio");
                aliasList.append("video /exec media video");
            }
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
    return QString();
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

// override to add %u if needed
QString Preferences::webBrowserCmd()
{
  // add %u to command if it's not in there
  QString cmd=self()->mWebBrowserCmd;
  if(cmd.find("%u")==-1) cmd+=" %u";
  return cmd;
}

#include "preferences.moc"
