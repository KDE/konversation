/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2005 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2005 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2005-2008 Eike Hein <hein@kde.org>
*/

#include "config/preferences.h"
#include "ignore.h"
#include "highlight.h"

#include <QHashIterator>
#include <QHeaderView>
#include <QTreeView>

#include <KLocalizedString>
#include <KUser>
#include <KSharedConfig>
#include <QStandardPaths>

struct PreferencesSingleton
{
    Preferences prefs;
};

Q_GLOBAL_STATIC(PreferencesSingleton, s_prefs)

Preferences *Preferences::self()
{
  if ( !s_prefs.exists() ) {
    s_prefs->prefs.load();
  }

  return &s_prefs->prefs;
}


Preferences::Preferences()
{
    // kconfigcompiler 5.91 cannot generate code with signals for ItemAccessors=true
    // manually add the item for now
    setCurrentGroup(QStringLiteral("LauncherEntry"));

    auto notifyFunction = static_cast<KConfigCompilerSignallingItem::NotifyFunction>(&Preferences::itemChanged);

    auto* innerItemShowLauncherEntryCount = new KConfigSkeleton::ItemBool(currentGroup(), QStringLiteral("ShowLauncherEntryCount"), mShowLauncherEntryCount, true);
    mShowLauncherEntryCountItem = new KConfigCompilerSignallingItem(innerItemShowLauncherEntryCount, this, notifyFunction, signalShowLauncherEntryCountChanged);
    addItem(mShowLauncherEntryCountItem);
    // end manual item addition

    // create default identity
    mIdentity=new Identity();
    mIdentity->setName(i18n("Default Identity"));
    mIdentityList.append(mIdentity);

    KUser user(KUser::UseRealUserID);
    mIdentity->setIdent(user.loginName());
    mIdentity->setRealName(user.property(KUser::FullName).toString());

    const QStringList nickList = {
        user.loginName(),
        user.loginName() + QLatin1Char('_'),
        user.loginName() + QStringLiteral("__"),
    };
    mIdentity->setNicknameList(nickList);

    Konversation::ServerGroupSettingsPtr serverGroup(new Konversation::ServerGroupSettings);
    serverGroup->setName(QStringLiteral("libera"));
    Konversation::ServerSettings server;
    server.setHost(QStringLiteral("irc.libera.chat"));
    server.setPort(6697);
    server.setSSLEnabled(true);
    serverGroup->addServer(server);
    serverGroup->setIdentityId(mIdentity->id());
    Konversation::ChannelSettings channel;
    channel.setName(QStringLiteral("#konversation"));
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
    return QStringList {
        QStringLiteral("Op,/OP %u%n"),
        QStringLiteral("DeOp,/DEOP %u%n"),
        QStringLiteral("WhoIs,/WHOIS %s,%%u%n"),
        QStringLiteral("Version,/CTCP %s,%%u VERSION%n"),
        QStringLiteral("Kick,/KICK %u%n"),
        QStringLiteral("Ban,/BAN %u%n"),
        QStringLiteral("Part,/PART %c Leaving...%n"),
        QStringLiteral("Quit,/QUIT Leaving...%n"),
    };
}

const QStringList Preferences::quickButtonList()
{
  return self()->mQuickButtonList;
}

void Preferences::setQuickButtonList(const QStringList& newList)
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
    return QList<QStringList> {
        { QStringLiteral("1"), QStringLiteral("o"), QStringLiteral("\\[\\[([^\\s]+)\\]\\]"), QStringLiteral("http://en.wikipedia.org/wiki/Special:Search?go=Go&search=%1") },
        { QStringLiteral("1"), QStringLiteral("o"), QStringLiteral("(BUG:|bug:)([0-9]+)"), QStringLiteral("https://bugs.kde.org/show_bug.cgi?id=%2") },
    };
}

const QList<QStringList> Preferences::autoreplaceList()
{
  return self()->mAutoreplaceList;
}

void Preferences::setAutoreplaceList(const QList<QStringList> &newList)
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
    const Konversation::ServerGroupHash& serverGroupHash = self()->mServerGroupHash;
    for (const auto& serverGroupSettings : serverGroupHash) {
        const auto serverList = serverGroupSettings->serverList();
        for (const auto& serverSertting : serverList) {
            if (serverSertting.host().toLower() == server)
                serverGroups.append(serverGroupSettings);
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
    const Konversation::ServerGroupHash& serverGroupHash = self()->mServerGroupHash;
    for (const auto& serverGroupSettings : serverGroupHash) {
        if (serverGroupSettings->name().toLower() == server.toLower())
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

void Preferences::setHighlightList(const QList<Highlight*> &newList)
{
    qDeleteAll(self()->mHighlightList);
    self()->mHighlightList.clear();
    self()->mHighlightList=newList;
}

void Preferences::addHighlight(const QString& highlight, bool regExp, const QColor& color,
    const QString& soundURL, const QString& autoText, const QString& chatWindows, bool notify)
{
    self()->mHighlightList.append(new Highlight(highlight, regExp, color,
        QUrl(soundURL), autoText, chatWindows, notify));
}

void Preferences::setIgnoreList(const QList<Ignore*> &newList)
{
    clearIgnoreList();
    self()->mIgnoreList=newList;
}

void Preferences::addIgnore(const QString &newIgnore)
{
    QStringList ignore = newIgnore.split(QLatin1Char(','));
    self()->mIgnoreList.append(new Ignore(ignore[0],ignore[1].toInt()));
}

bool Preferences::removeIgnore(const QString &oldIgnore)
{
    for (Ignore *ignore : std::as_const(self()->mIgnoreList)) {
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
    for (Ignore *ignore : std::as_const(self()->mIgnoreList)) {
        if (ignore->getName().section(QLatin1Char('!'),0,0).toLower()==nickname.toLower())
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
    return self()->mNotifyList.value(serverGroupId);
}

const QString Preferences::notifyStringByGroupId(int serverGroupId)
{
    return notifyListByGroupId(serverGroupId).join(QLatin1Char(' '));
}

bool Preferences::addNotify(int serverGroupId, const QString& newPattern)
{
    QStringList& list = self()->mNotifyList[serverGroupId];

    if (!list.contains(newPattern, Qt::CaseInsensitive))
    {
        list.append(newPattern);

        if (list.size() == 1)
            Q_EMIT self()->notifyListStarted(serverGroupId);

        return true;
    }

    return false;
}

bool Preferences::removeNotify(int serverGroupId, const QString& pattern)
{
    if (self()->mNotifyList.contains(serverGroupId))
    {
        QString lowered = pattern.toLower();
        const QStringList& oldList = self()->mNotifyList[serverGroupId];
        QStringList newList;

        for (const QString& nick : oldList) {
            if (nick.toLower() != lowered)
                newList << nick;
        }

        if (newList.size() < oldList.size())
        {
            if (newList.isEmpty())
                self()->mNotifyList.remove(serverGroupId);
            else
                self()->mNotifyList[serverGroupId] = newList;

            return true;
        }
    }

    return false;
}

bool Preferences::isNotify(int serverGroupId, const QString& pattern)
{
    if (self()->mNotifyList.contains(serverGroupId))
        if (self()->mNotifyList.value(serverGroupId).contains(pattern, Qt::CaseInsensitive))
            return true;

    return false;
}

// Default identity functions
void Preferences::addIdentity(const IdentityPtr &identity) { self()->mIdentityList.append(identity); }
void Preferences::removeIdentity(const IdentityPtr &identity) { self()->mIdentityList.removeOne(identity); }

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
    const QList<IdentityPtr> identities = identityList();
    for (const auto& identity : identities) {
        if (identity->getName() == name) {
            return identity;
        }
    }

    // no self()->matching identity found, return default identity
    return identities.first();
}

const IdentityPtr Preferences::identityById(int id)
{
    const QList<IdentityPtr> identities = identityList();
    for (const auto& identity : identities) {
        if (identity->id() == id) {
            return identity;
        }
    }

    return identities.first();
}

QStringList Preferences::defaultAliasList()
{
    // Auto-alias scripts
    const QStringList scriptDirs = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, QStringLiteral("scripts"), QStandardPaths::LocateDirectory);
    QSet<QString> scripts;

    for (const QString &dir : scriptDirs) {

        const QStringList scriptFiles = QDir(dir).entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Executable);

        for (const QString &script : scriptFiles) {
            scripts << script;
        }
    }

    QStringList aliasList;

    for (const QString& script : std::as_const(scripts)) {
        aliasList.append(QStringLiteral("%1 /exec %1").arg(script));

        // FIXME: Historically, defaultAliasList() is primarily used to dynamically
        // compile a list of installed scripts and generate appropriate aliases for
        // them. It's not only used when the alias preferences are reset or initia-
        // lized, but also on application start. The following crudely adds two
        // aliases when the 'media' script is found, to provide easy access to its
        // capability to differentiate  between audio and video media. This method
        // needs at the very least to be split up in two, or scripts may in the
        // future determine what aliases they want to add.
        if (script == QLatin1String("media"))
        {
            aliasList.append(QStringLiteral("audio /exec media audio"));
            aliasList.append(QStringLiteral("video /exec media video"));
        }
    }

    return aliasList;
}

void Preferences::clearIgnoreList() { qDeleteAll(self()->mIgnoreList); self()->mIgnoreList.clear(); }
const QList<Ignore*> Preferences::ignoreList() { return self()->mIgnoreList; }

void Preferences::setShowTrayIcon(bool state)
{
    self()->PreferencesBase::setShowTrayIcon(state);
    Q_EMIT self()->updateTrayIcon();
}

void Preferences::setTrayNotify(bool state)
{
    self()->PreferencesBase::setTrayNotify(state);
    Q_EMIT self()->updateTrayIcon();
}


void Preferences::setAutoUserhost(bool state)
{
    self()->PreferencesBase::setAutoUserhost(state);
}


// Channel Encodings
const QString Preferences::channelEncoding(const QString& server,const QString& channel)
{
    //check all matching server's encodings
    //TODO when we rewrite dbus, allow for multiple encodings to be returned
    //for true 'duplicity compatibility'
    const QList<int> serverIds = serverGroupIdsByName(server);
    if(serverIds.count() > 1)
    {
        for (int serverId : serverIds) {
            const QString codec = channelEncoding(serverId, channel);
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
    const QList<int> serverIds = serverGroupIdsByName(server);
    if(serverIds.count() > 1)
    {
        for (int serverId : serverIds)
            setChannelEncoding(serverId, channel, encoding);
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

const QString Preferences::spellCheckingLanguage(const Konversation::ServerGroupSettingsPtr &serverGroup, const QString& key)
{
    if (self()->mServerGroupSpellCheckingLanguages.contains(serverGroup))
        return self()->mServerGroupSpellCheckingLanguages.value(serverGroup).value(key);

    return QString();
}

const QString Preferences::spellCheckingLanguage(const QString& server, const QString& key)
{
    if (self()->mServerSpellCheckingLanguages.contains(server))
        return self()->mServerSpellCheckingLanguages.value(server).value(key);

    return QString();
}

void Preferences::setSpellCheckingLanguage(const Konversation::ServerGroupSettingsPtr &serverGroup, const QString& key, const QString& language)
{
    QHash<QString, QString> languageHash;

    if (self()->mServerGroupSpellCheckingLanguages.contains(serverGroup))
        languageHash = self()->mServerGroupSpellCheckingLanguages.value(serverGroup);

    languageHash.insert(key, language);

    self()->mServerGroupSpellCheckingLanguages.insert(serverGroup, languageHash);
}

void Preferences::setSpellCheckingLanguage(const QString& server, const QString& key, const QString& language)
{
    QHash<QString, QString> languageHash;

    if (self()->mServerSpellCheckingLanguages.contains(server))
        languageHash = self()->mServerSpellCheckingLanguages.value(server);

    languageHash.insert(key, language);

    self()->mServerSpellCheckingLanguages.insert(server, languageHash);
}

const QHash< Konversation::ServerGroupSettingsPtr, QHash< QString, QString > > Preferences::serverGroupSpellCheckingLanguages()
{
    return self()->mServerGroupSpellCheckingLanguages;
}

const QHash< QString, QHash< QString, QString > > Preferences::serverSpellCheckingLanguages()
{
    return self()->mServerSpellCheckingLanguages;
}

const QString Preferences::defaultNicknameSortingOrder()
{
  return QStringLiteral("qpohv-");
}

// override to add %u if needed
QString Preferences::webBrowserCmd()
{
  // add %u to command if it's not in there
  QString cmd=self()->mWebBrowserCmd;
  if (!cmd.contains(QLatin1String("%u")))
      cmd += QStringLiteral(" %u");
  return cmd;
}

void Preferences::saveColumnState(QTreeView *treeView, const QString &name)
{
    KConfigGroup group(KSharedConfig::openConfig(), name);

    QList<int> columnWidths;
    const int columnCount = treeView->header()->count();
    columnWidths.reserve(columnCount);
    for (int i = 0; i < columnCount; ++i)
        columnWidths.append(treeView->columnWidth(i));
    // save column widths
    group.writeEntry("ColumnWidths", columnWidths);
    group.writeEntry("ColumnSorted", treeView->header()->sortIndicatorSection());
    group.writeEntry("ColumnSortDescending", treeView->header()->sortIndicatorOrder() == Qt::DescendingOrder ? true : false );
}

void Preferences::restoreColumnState(QTreeView* treeView, const QString &name, int defaultColumn , Qt::SortOrder defaultSortOrder)
{
    KConfigGroup group(KSharedConfig::openConfig(), name);

    QList<int> columnWidths = group.readEntry("ColumnWidths", QList<int>());
    for (int i = 0; i < columnWidths.count(); ++i)
        if (columnWidths.at(i))
            treeView->setColumnWidth(i, columnWidths.at(i));

    if (group.readEntry("ColumnSortDescending", (defaultSortOrder == Qt::DescendingOrder)))
        treeView->header()->setSortIndicator(group.readEntry("ColumnSorted", defaultColumn), Qt::DescendingOrder);
    else
        treeView->header()->setSortIndicator(group.readEntry("ColumnSorted", defaultColumn), Qt::AscendingOrder);
}

void Preferences::slotSetUseOSD(bool use)
{
    self()->setUseOSD(use);
}


void Preferences::setShowLauncherEntryCount(bool value)
{
    if ((value != mShowLauncherEntryCount) && !isShowLauncherEntryCountImmutable()) {
        mShowLauncherEntryCount = value;
        mSettingsChanged |= signalShowLauncherEntryCountChanged;
    }
}

bool Preferences::showLauncherEntryCount() const
{
    return mShowLauncherEntryCount;
}

bool Preferences::isShowLauncherEntryCountImmutable() const
{
    return isImmutable(QStringLiteral("ShowLauncherEntryCount"));
}

bool Preferences::usrSave()
{
    if (!PreferencesBase::usrSave()) {
        return false;
    }

    if (mSettingsChanged & signalShowLauncherEntryCountChanged) {
        Q_EMIT showLauncherEntryCountChanged(mShowLauncherEntryCount);
    }

    mSettingsChanged = 0;

    return true;
}

void Preferences::itemChanged(quint64 flags)
{
    mSettingsChanged |= flags;
}

#include "moc_preferences.cpp"
