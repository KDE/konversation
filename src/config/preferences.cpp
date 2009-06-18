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
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#include "config/preferences.h"
#include "ignore.h"
#include "highlight.h"

#include <QPalette>
#include <QRegExp>
#include <QFileInfo>
#include <QHashIterator>

#include <KLocale>
#include <KUser>
#include <KStandardDirs>


struct PreferencesSingleton
{
    Preferences prefs;
};

K_GLOBAL_STATIC(PreferencesSingleton, s_prefs)

Preferences *Preferences::self()
{
  if ( !s_prefs.exists() ) {
    s_prefs->prefs.readConfig();
  }

  return &s_prefs->prefs;
}


Preferences::Preferences()
{
    // create default identity
    mIdentity=new Identity();
    mIdentity->setName(i18n("Default Identity"));
    mIdentityList.append(mIdentity);

    KUser user(KUser::UseRealUserID);
    mIdentity->setIdent(user.loginName());
    mIdentity->setRealName(user.property(KUser::FullName).toString());

    QStringList nickList;
    nickList.append(user.loginName());
    nickList.append(user.loginName() + '_');
    nickList.append(user.loginName() + "__");
    mIdentity->setNicknameList(nickList);

    Konversation::ServerGroupSettingsPtr serverGroup(new Konversation::ServerGroupSettings);
    serverGroup->setName("Freenode");
    Konversation::ServerSettings server;
    server.setHost("chat.freenode.net");
    server.setPort(8001);
    serverGroup->addServer(server);
    serverGroup->setIdentityId(mIdentity->id());
    Konversation::ChannelSettings channel;
    channel.setName("#konversation");
    serverGroup->addChannel(channel);
    serverGroup->setExpanded(false);
    mServerGroupHash.insert(0, serverGroup);
    mQuickButtonList = defaultQuickButtonList();
    mAutoreplaceList = defaultAutoreplaceList();
}

Preferences::~Preferences()
{
    mIdentityList.clear();
    qDeleteAll(mIgnoreList);
    qDeleteAll(mHighlightList);
}
const Konversation::ServerGroupHash Preferences::serverGroupHash()
{
    return self()->mServerGroupHash;
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

const QList<QStringList> Preferences::defaultAutoreplaceList()
{
    QList<QStringList> defaultList;
    defaultList.append(QStringList() << "1" << "o" << "\\[\\[([^\\s]+)\\]\\]" << "http://en.wikipedia.org/wiki/Special:Search?go=Go&search=%1");
    defaultList.append(QStringList() << "1" << "o" << "(BUG:|bug:)([0-9]+)" << "https://bugs.kde.org/show_bug.cgi?id=%2");
    return defaultList;
}

const QList<QStringList> Preferences::autoreplaceList()
{
  return self()->mAutoreplaceList;
}

void Preferences::setAutoreplaceList(const QList<QStringList> newList)
{
  self()->mAutoreplaceList=newList;
}

void Preferences::clearAutoreplaceList()
{
  self()->mAutoreplaceList.clear();
}

// --------------------------- AutoReplace ---------------------------

void Preferences::setServerGroupHash(const Konversation::ServerGroupHash& hash)
{
    self()->mServerGroupHash.clear();
    self()->mServerGroupHash = hash;
}

void Preferences::addServerGroup(Konversation::ServerGroupSettingsPtr serverGroup)
{
    Konversation::ServerGroupHash hash = self()->mServerGroupHash;
    hash.insert(serverGroup->id(), serverGroup);
    self()->mServerGroupHash = hash;
}

const Konversation::ServerGroupSettingsPtr Preferences::serverGroupById(int id)
{
    return  self()->mServerGroupHash.value(id);
}

const QList<Konversation::ServerGroupSettingsPtr> Preferences::serverGroupsByServer(const QString& server)
{
    QList<Konversation::ServerGroupSettingsPtr> serverGroups;
    QHashIterator<int, Konversation::ServerGroupSettingsPtr> it(self()->mServerGroupHash);
    while(it.hasNext())
    {
        it.next();
        for (int i=0; i != it.value()->serverList().count(); i++)
        {
            if(it.value()->serverByIndex(i).host().toLower() == server)
                serverGroups.append(it.value());
        }
    }
    return serverGroups;
}

QList<int> Preferences::serverGroupIdsByName(const QString& serverGroup)
{
    QList<int> serverIds;
    QHashIterator<int, Konversation::ServerGroupSettingsPtr> it(self()->mServerGroupHash);
    while(it.hasNext())
    {
        if(it.next().value()->name().toLower() == serverGroup.toLower())
            serverIds.append(it.key());
    }
    if (serverIds.isEmpty())
        serverIds.append(-1);

    return serverIds;
}

bool Preferences::isServerGroup(const QString& server)
{
    QHashIterator<int, Konversation::ServerGroupSettingsPtr> it(self()->mServerGroupHash);
    while(it.hasNext())
    {
        if(it.next().value()->name().toLower() == server.toLower())
            return true;
    }
    return false;
}

void Preferences::removeServerGroup(int id)
{
    self()->mServerGroupHash.remove(id);
}


const QList<Highlight*> Preferences::highlightList()
{
    return self()->mHighlightList;
}

void Preferences::setHighlightList(QList<Highlight*> newList)
{
    qDeleteAll(self()->mHighlightList);
    self()->mHighlightList.clear();
    self()->mHighlightList=newList;
}

void Preferences::addHighlight(const QString& newHighlight,
bool regExp,
const QColor &newColor,
const QString& sound,
const QString& autoText)
{
    self()->mHighlightList.append(new Highlight(newHighlight,regExp,newColor,KUrl(sound),autoText));
}

void Preferences::setIgnoreList(QList<Ignore*> newList)
{
    clearIgnoreList();
    self()->mIgnoreList=newList;
}

void Preferences::addIgnore(const QString &newIgnore)
{
    QStringList ignore = newIgnore.split(',');
    removeIgnore(ignore[0]);
    self()->mIgnoreList.append(new Ignore(ignore[0],ignore[1].toInt()));
}

bool Preferences::removeIgnore(const QString &oldIgnore)
{
    foreach (Ignore *ignore, self()->mIgnoreList)
    {
        if (ignore->getName().toLower() == oldIgnore.toLower())
        {
            self()->mIgnoreList.removeOne(ignore);
            delete ignore;
            return true;
        }
    }

    return false;
}

bool Preferences::isIgnored(const QString &nickname)
{
    foreach (Ignore *ignore, self()->mIgnoreList)
    {
        if (ignore->getName().section('!',0,0).toLower()==nickname.toLower())
        {
            return true;
        }
    }

    return false;
}

void Preferences::setNotifyList(const QMap<int, QStringList> &newList)
{ self()->mNotifyList=newList; }

const QMap<int, QStringList> Preferences::notifyList() { return self()->mNotifyList; }

const QStringList Preferences::notifyListByGroupId(int serverGroupId)
{
  if (serverGroupId && self()->mNotifyList.find(serverGroupId) != self()->mNotifyList.end())
        return self()->mNotifyList.value(serverGroupId);
  else
        return QStringList();
}

const QString Preferences::notifyStringByGroupId(int serverGroupId)
{
    return notifyListByGroupId(serverGroupId).join(" ");
}

bool Preferences::addNotify(int serverGroupId, const QString& newPattern)
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

bool Preferences::removeNotify(int serverGroupId, const QString& pattern)
{
    if (self()->mNotifyList.find(serverGroupId) != self()->mNotifyList.end())
    {
        QStringList nicknameList = self()->mNotifyList[serverGroupId];
        nicknameList.removeAll(pattern); //FIXME: what semantics do you really want here?
        if (nicknameList.isEmpty())
            self()->mNotifyList.remove(serverGroupId);
        else
            self()->mNotifyList[serverGroupId] = nicknameList;
        return true;
    }
    return false;
}

bool Preferences::isNotify(int serverGroupId, const QString& pattern)
{
    if (self()->mNotifyList.find(serverGroupId) != self()->mNotifyList.end())
    {
        QStringList nicknameList = self()->mNotifyList[serverGroupId];

        if (nicknameList.contains(pattern)) return true;
    }
    return false;
}

bool Preferences::hasNotifyList(int serverGroupId)
{
    if (self()->mNotifyList.find(serverGroupId) != self()->mNotifyList.end())
        return true;
    else
        return false;
}

// Default identity functions
void Preferences::addIdentity(IdentityPtr identity) { self()->mIdentityList.append(identity); }
void Preferences::removeIdentity(IdentityPtr identity) { self()->mIdentityList.removeOne(identity); }

void Preferences::clearIdentityList()
{
    self()->mIdentityList.clear();
}

const IdentityList Preferences::identityList() { return self()->mIdentityList; }

void Preferences::setIdentityList(const IdentityList& list)
{
    self()->mIdentityList.clear();
    self()->mIdentityList = list;
}

const IdentityPtr Preferences::identityByName(const QString& name)
{
    QList<IdentityPtr> identities = identityList();
    QList<IdentityPtr>::iterator it = identities.begin();

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
    QList<IdentityPtr> identList = identityList();
    for(QList<IdentityPtr>::iterator it = identList.begin(); it != identList.end(); ++it)
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
    const QStringList scripts = KGlobal::dirs()->findAllResources("data","konversation/scripts/*");
    QFileInfo* fileInfo = new QFileInfo();
    QStringList aliasList;
    QString newAlias;

    for (QStringList::ConstIterator it = scripts.constBegin(); it != scripts.constEnd(); ++it)
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

    delete fileInfo;

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

bool Preferences::showAwayMessage() { return self()->mIdentityList[0]->getShowAwayMessage(); }
void Preferences::setShowAwayMessage(bool state) { self()->mIdentityList[0]->setShowAwayMessage(state); }

const QString Preferences::awayMessage() { return self()->mIdentityList[0]->getAwayMessage(); }
void Preferences::setAwayMessage(const QString &newMessage) { self()->mIdentityList[0]->setAwayMessage(newMessage); }
const QString Preferences::unAwayMessage() { return self()->mIdentityList[0]->getReturnMessage(); }
void Preferences::setUnAwayMessage(const QString &newMessage) { self()->mIdentityList[0]->setReturnMessage(newMessage); }

void Preferences::clearIgnoreList() { qDeleteAll(self()->mIgnoreList); self()->mIgnoreList.clear(); }
const QList<Ignore*> Preferences::ignoreList() { return self()->mIgnoreList; }

const QString Preferences::nickname(int index) { return self()->mIdentityList[0]->getNickname(index); }
const QStringList Preferences::nicknameList() { return self()->mIdentityList[0]->getNicknameList(); }
void Preferences::setNickname(int index,const QString &newName) { self()->mIdentityList[0]->setNickname(index,newName); }
void Preferences::setNicknameList(const QStringList &newList) { self()->mIdentityList[0]->setNicknameList(newList); }

void Preferences::setShowTrayIcon(bool state)
{
    self()->PreferencesBase::setShowTrayIcon(state);
    emit self()->updateTrayIcon();
}

void Preferences::setTrayNotify(bool state)
{
    self()->PreferencesBase::setTrayNotify(state);
    emit self()->updateTrayIcon();
}


void Preferences::setAutoUserhost(bool state)
{
    self()->PreferencesBase::setAutoUserhost(state);
}

bool Preferences::dialogFlag(const QString& flagName)
{
    KConfigGroup config(KGlobal::config()->group("Notification self()->Messages"));

    if (!config.readEntry(flagName).isEmpty())
        return false;
    else
        return true;
}

void Preferences::setDialogFlag(const QString& flagName,bool state)
{
    KConfigGroup config(KGlobal::config()->group("Notification self()->Messages"));

    if (state)
        config.deleteEntry(flagName);
    else
    {
        if (config.readEntry(flagName).isEmpty())
            config.writeEntry(flagName,"no");
    }

    config.sync();
}


// Channel Encodings
const QString Preferences::channelEncoding(const QString& server,const QString& channel)
{
    //check all matching server's encodings
    //TODO when we rewrite dbus, allow for multiple encodings to be returned
    //for true 'duplicity compatibility'
    QList<int> serverIds = serverGroupIdsByName(server);
    QString codec;
    if(serverIds.count() > 1)
    {
        for (int i=0; i < serverIds.count(); i++)
        {
            codec = channelEncoding(serverIds.at(i),channel);
            if(!codec.isEmpty())
                return codec;
        }
        return QString();
    }
    else
        return channelEncoding(serverIds.first(),channel);
}

const QString Preferences::channelEncoding(int serverGroupId,const QString& channel)
{
    if(self()->mChannelEncodingsMap.contains(serverGroupId))
        if(self()->mChannelEncodingsMap[serverGroupId].contains(channel.toLower()))
            return self()->mChannelEncodingsMap[serverGroupId][channel.toLower()];
    return QString();
}

void Preferences::setChannelEncoding(const QString& server,const QString& channel,const QString& encoding)
{
    //set channel encoding for ALL matching servergroups
    QList<int> serverIds = serverGroupIdsByName(server);
    if(serverIds.count() > 1)
    {
        for(int i=0; i < serverIds.count(); i++)
            setChannelEncoding(serverIds.at(i),channel,encoding);
    }
    else
        setChannelEncoding(serverIds.first(),channel,encoding);
}

void Preferences::setChannelEncoding(int serverGroupId,const QString& channel,const QString& encoding)
{
    self()->mChannelEncodingsMap[serverGroupId][channel.toLower()]=encoding;
}

const QList<int> Preferences::channelEncodingsServerGroupIdList()
{
    return self()->mChannelEncodingsMap.keys();
}

const QStringList Preferences::channelEncodingsChannelList(int serverGroupId)
{
    return self()->mChannelEncodingsMap[serverGroupId].keys();
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
  if (!cmd.contains("%u"))
      cmd += " %u";
  return cmd;
}

#include "preferences.moc"
