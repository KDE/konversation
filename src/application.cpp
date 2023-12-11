/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2005 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2005 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2005-2008 Eike Hein <hein@kde.org>
*/

#include "application.h"

#include "connectionmanager.h"
#include "scriptlauncher.h"
#include "transfermanager.h"
#include "viewcontainer.h"
#include "trayicon.h"
#include "urlcatcher.h"
#include "highlight.h"
#include "sound.h"
#include "quickconnectdialog.h"
#include "dbus.h"
#include "servergroupsettings.h"
#include "serversettings.h"
#include "channel.h"
#include "images.h"
#include "notificationhandler.h"
#include "launcherentryhandler.h"
#include "awaymanager.h"
#include "konversation_log.h"
#include "konversation_state.h"

#include <kio_version.h>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KConfig>
#include <KShell>
#include <KMacroExpander>
#include <KWallet>
#include <KTextEdit>
#include <KSharedConfig>
#include <KWindowSystem>

#if HAVE_X11
#include <KStartupInfo>
#endif

#include <QRegularExpression>
#include <QDBusConnection>
#include <QNetworkProxy>
#include <QStandardItemModel>
#include <QFileInfo>
#include <QTextCursor>
#include <QDesktopServices>
#include <QCommandLineParser>
#include <QNetworkInformation>

using namespace Konversation;

Application::Application(int &argc, char **argv)
: QApplication(argc, argv)
{
    mainWindow = nullptr;
    m_restartScheduled = false;
    m_connectionManager = nullptr;
    m_awayManager = nullptr;
    m_scriptLauncher = nullptr;
    quickConnectDialog = nullptr;
    m_osd = nullptr;
    m_wallet = nullptr;
    m_images = nullptr;
    m_sound = nullptr;
    m_dccTransferManager = nullptr;
    m_notificationHandler = nullptr;
    m_launcherEntryHandler = nullptr;
    m_urlModel = nullptr;
    dbusObject = nullptr;
    identDBus = nullptr;
}

Application::~Application()
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;

    if (!m_images)
        return; // Nothing to do, newInstance() has never been called.

    stashQueueRates();
    Preferences::self()->save(); // FIXME i can't figure out why this isn't in saveOptions --argonel
    KonversationState::self()->save();
    saveOptions(false);

    // Delete m_dccTransferManager here as its destructor depends on the main loop being in tact which it
    // won't be if if we wait till Qt starts deleting parent pointers.
    delete m_dccTransferManager;

    delete m_images;
    delete m_sound;
    //delete dbusObject;
    //delete prefsDCOP;
    //delete identDBus;
    delete m_osd;
    m_osd = nullptr;
    closeWallet();
    if (m_restartScheduled) implementRestart();
}

void Application::implementRestart()
{
    // Pop off the executable name. May not be the first argument in argv
    // everywhere, so verify first.
    if (QFileInfo(m_restartArguments.first()) == QFileInfo(QCoreApplication::applicationFilePath()))
        m_restartArguments.removeFirst();

    // Don't round-trip --restart.
    m_restartArguments.removeAll(QStringLiteral("--restart"));

    // Avoid accumulating multiple --startupdelay arguments across multiple
    // uses of restart().
    if (m_restartArguments.contains(QLatin1String("--startupdelay")))
    {
        int index = m_restartArguments.lastIndexOf(QStringLiteral("--startupdelay"));

        if (index < m_restartArguments.count() - 1 && !m_restartArguments.at(index + 1).startsWith(QLatin1Char('-')))
        {
            QString delayArgument = m_restartArguments.at(index + 1);

            bool ok;

            uint delay = delayArgument.toUInt(&ok, 10);

            // If the argument is invalid or too low, raise to at least 2000 msecs.
            if (!ok || delay < 2000)
                m_restartArguments.replace(index + 1, QStringLiteral("2000"));
        }
    }
    else
        m_restartArguments << QStringLiteral("--startupdelay") << QStringLiteral("2000");

    KProcess::startDetached(QCoreApplication::applicationFilePath(), m_restartArguments);
}

void Application::createMainWindow(AutoConnectMode autoConnectMode, WindowRestoreMode restoreMode)
{
    connect(this, &Application::aboutToQuit, this, &Application::prepareShutdown);

    m_connectionManager = new ConnectionManager(this);

    m_awayManager = new AwayManager(this);

    connect(m_connectionManager, &ConnectionManager::identityOnline, m_awayManager, &AwayManager::identityOnline);
    connect(m_connectionManager, &ConnectionManager::identityOffline, m_awayManager, &AwayManager::identityOffline);
    connect(m_connectionManager, &ConnectionManager::connectionChangedAwayState, m_awayManager, &AwayManager::updateGlobalAwayAction);

    QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability);
    connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, m_connectionManager, &ConnectionManager::onOnlineStateChanged);

    m_scriptLauncher = new ScriptLauncher(this);

    // an instance of DccTransferManager needs to be created before GUI class instances' creation.
    m_dccTransferManager = new DCC::TransferManager(this);

    // make sure all vars are initialized properly
    quickConnectDialog = nullptr;

    // Sound object used to play sound is created when needed.
    m_sound = nullptr;

    // initialize OSD display here, so we can read the Preferences::properly
    m_osd = new OSDWidget(QStringLiteral("Konversation"));

    Preferences::self();
    readOptions();

    // Images object providing LEDs, NickIcons
    m_images = new Images();

    m_urlModel = new QStandardItemModel(0, 3, this);

    // Auto-alias scripts.  This adds any missing aliases
    QStringList aliasList(Preferences::self()->aliasList());
    const QStringList scripts(Preferences::defaultAliasList());
    bool changed = false;
    for (const QString& script : scripts) {
        if (!aliasList.contains(script)) {
            changed = true;
            aliasList.append(script);
        }
    }
    if(changed)
        Preferences::self()->setAliasList(aliasList);

    // open main window
    mainWindow = new MainWindow();

    connect(mainWindow.data(), &MainWindow::showQuickConnectDialog, this, &Application::openQuickConnectDialog);
    connect(Preferences::self(), &Preferences::updateTrayIcon, mainWindow.data(), &MainWindow::updateTrayIcon);
    connect(mainWindow.data(), &MainWindow::endNotification, m_osd, &OSDWidget::hide);
    // take care of user style changes, setting back colors and stuff

    // apply GUI settings
    Q_EMIT appearanceChanged();

    if (restoreMode == WindowRestore)
        mainWindow->restore();
    else if (Preferences::self()->showTrayIcon() && Preferences::self()->hideToTrayOnStartup())
    {
        mainWindow->systemTrayIcon()->hideWindow();
#if HAVE_X11
        KStartupInfo::appStarted();
#endif
    }
    else
        mainWindow->show();

    bool openServerList = Preferences::self()->showServerList();

    // handle autoconnect on startup
    const Konversation::ServerGroupHash serverGroups = Preferences::serverGroupHash();

    if (autoConnectMode == AutoConnect)
    {
        QList<ServerGroupSettingsPtr> serversToAutoconnect;
        for (const auto& server : serverGroups) {
            if (server->autoConnectEnabled()) {
                openServerList = false;
                serversToAutoconnect << server;
            }
        }

        std::sort(serversToAutoconnect.begin(), serversToAutoconnect.end(), [] (const ServerGroupSettingsPtr &left, const ServerGroupSettingsPtr &right)
        {
            return left->sortIndex() < right->sortIndex();
        });

        for (const auto& server : std::as_const(serversToAutoconnect)) {
            m_connectionManager->connectTo(Konversation::CreateNewConnection, server->id());
        }
    }

    if (openServerList) mainWindow->openServerList();

    connect(this, &Application::serverGroupsChanged, this, &Application::saveOptions);

    // prepare dbus interface
    dbusObject = new Konversation::DBus(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/irc"), dbusObject, QDBusConnection::ExportNonScriptableSlots);
    identDBus = new Konversation::IdentDBus(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/identity"), identDBus, QDBusConnection::ExportNonScriptableSlots);

    if (dbusObject)
    {
        connect(dbusObject,&DBus::dbusMultiServerRaw,
            this,&Application::dbusMultiServerRaw );
        connect(dbusObject,&DBus::dbusRaw,
            this,&Application::dbusRaw );
        connect(dbusObject,&DBus::dbusSay,
            this,&Application::dbusSay );
        connect(dbusObject,&DBus::dbusInfo,
            this,&Application::dbusInfo );
        connect(dbusObject,&DBus::dbusInsertMarkerLine,
            mainWindow.data(),&MainWindow::insertMarkerLine);
        connect(dbusObject, &DBus::connectTo,
            m_connectionManager,
            QOverload<Konversation::ConnectionFlag, const QString&, const QString&, const QString&, const QString&, const QString&, bool>::of(&ConnectionManager::connectTo));
    }

    m_notificationHandler = new Konversation::NotificationHandler(this);
    m_launcherEntryHandler = new Konversation::LauncherEntryHandler(this);

    connect(this, &Application::appearanceChanged, this, &Application::updateProxySettings);
}

void Application::newInstance(QCommandLineParser *args)
{
    QString url;
    if (!args->positionalArguments().isEmpty())
        url = args->positionalArguments().at(0);

    if (!mainWindow)
    {
        const AutoConnectMode autoConnectMode = (!args->isSet(QStringLiteral("noautoconnect")) && url.isEmpty() && !args->isSet(QStringLiteral("server"))) ? AutoConnect : NoAutoConnect;
        createMainWindow(autoConnectMode, NoWindowRestore);
    }
    else if (args->isSet(QStringLiteral("restart")))
    {
        restart();

        return;
    }

    if (!url.isEmpty())
        getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, url);
    else if (args->isSet(QStringLiteral("server")))
    {
        getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection,
                                          args->value(QStringLiteral("server")),
                                          args->value(QStringLiteral("port")),
                                          args->value(QStringLiteral("password")),
                                          args->value(QStringLiteral("nick")),
                                          args->value(QStringLiteral("channel")),
                                          args->isSet(QStringLiteral("ssl")));
    }

    return;
}

void Application::restoreInstance()
{
    createMainWindow(AutoConnect, WindowRestore);
}

Application* Application::instance()
{
    return qobject_cast<Application*>(QApplication::instance());
}

void Application::restart()
{
    m_restartScheduled = true;

    mainWindow->quitProgram();
}

void Application::prepareShutdown()
{
    if (mainWindow)
        mainWindow->getViewContainer()->prepareShutdown();

    if (m_awayManager)
    {
        m_awayManager->blockSignals(true);
        delete m_awayManager;
        m_awayManager = nullptr;
    }

    if (m_connectionManager)
    {
        m_connectionManager->quitServers();
        m_connectionManager->blockSignals(true);
        delete m_connectionManager;
        m_connectionManager = nullptr;
    }
}

bool Application::event(QEvent* event)
{
    if (event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::ApplicationFontChange) {
        Q_EMIT appearanceChanged();
    }

    return QApplication::event(event);
}

void Application::showQueueTuner(bool p)
{
    getMainWindow()->getViewContainer()->showQueueTuner(p);
}

void Application::dbusMultiServerRaw(const QString &command)
{
    sendMultiServerCommand(command.section(QLatin1Char(' '), 0,0), command.section(QLatin1Char(' '), 1));
}

void Application::dbusRaw(const QString& connection, const QString &command)
{
    Server* server = getConnectionManager()->getServerByName(connection, ConnectionManager::MatchByIdThenName);

    if (server) server->dbusRaw(command);
}


void Application::dbusSay(const QString& connection, const QString& target, const QString& command)
{
    Server* server = getConnectionManager()->getServerByName(connection, ConnectionManager::MatchByIdThenName);

    if (server) server->dbusSay(target, command);
}

void Application::dbusInfo(const QString& string)
{
    mainWindow->getViewContainer()->appendToFrontmost(i18n("D-Bus"), string, nullptr);
}

void Application::readOptions()
{
    // read nickname sorting order for channel nick lists
    KConfigGroup cgSortNicknames(KSharedConfig::openConfig()->group(QStringLiteral("Sort Nicknames")));

    QString sortOrder=cgSortNicknames.readEntry("SortOrder");
    QStringList sortOrderList=sortOrder.split(QString());
    sortOrderList.sort();
    if (sortOrderList.join(QString())!=QStringLiteral("-hopqv"))
    {
        sortOrder=Preferences::defaultNicknameSortingOrder();
        Preferences::self()->setSortOrder(sortOrder);
    }

    // Identity list
    const QStringList identityList=KSharedConfig::openConfig()->groupList().filter(
                                            QRegularExpression(QStringLiteral("Identity [0-9]+")));
    if (!identityList.isEmpty())
    {
        Preferences::clearIdentityList();

        for (const QString& identityGroup : identityList) {
            IdentityPtr newIdentity(new Identity());
            KConfigGroup cgIdentity(KSharedConfig::openConfig()->group(identityGroup));

            newIdentity->setName(cgIdentity.readEntry("Name"));

            newIdentity->setIdent(cgIdentity.readEntry("Ident"));
            newIdentity->setRealName(cgIdentity.readEntry("Realname"));

            newIdentity->setNicknameList(cgIdentity.readEntry<QStringList>("Nicknames",QStringList()));

            newIdentity->setAuthType(cgIdentity.readEntry("AuthType", "nickserv"));
            newIdentity->setAuthPassword(cgIdentity.readEntry("Password"));
            newIdentity->setNickservNickname(cgIdentity.readEntry("Bot"));
            newIdentity->setNickservCommand(cgIdentity.readEntry("NickservCommand", "identify"));
            newIdentity->setSaslAccount(cgIdentity.readEntry("SaslAccount"));
            newIdentity->setPemClientCertFile(cgIdentity.readEntry<QUrl>("PemClientCertFile", QUrl()));

            newIdentity->setInsertRememberLineOnAway(cgIdentity.readEntry("InsertRememberLineOnAway", false));
            newIdentity->setRunAwayCommands(cgIdentity.readEntry("ShowAwayMessage", false));
            newIdentity->setAwayCommand(cgIdentity.readEntry("AwayMessage"));
            newIdentity->setReturnCommand(cgIdentity.readEntry("ReturnMessage"));
            newIdentity->setAutomaticAway(cgIdentity.readEntry("AutomaticAway", false));
            newIdentity->setAwayInactivity(cgIdentity.readEntry("AwayInactivity", 10));
            newIdentity->setAutomaticUnaway(cgIdentity.readEntry("AutomaticUnaway", false));

            newIdentity->setQuitReason(cgIdentity.readEntry("QuitReason"));
            newIdentity->setPartReason(cgIdentity.readEntry("PartReason"));
            newIdentity->setKickReason(cgIdentity.readEntry("KickReason"));

            newIdentity->setShellCommand(cgIdentity.readEntry("PreShellCommand"));

            newIdentity->setCodecName(cgIdentity.readEntry("Codec"));

            newIdentity->setAwayMessage(cgIdentity.readEntry("AwayReason"));
            newIdentity->setAwayNickname(cgIdentity.readEntry("AwayNick"));

            Preferences::addIdentity(newIdentity);

        }

    }

    m_osd->setEnabled(Preferences::self()->useOSD());

    m_osd->setFont(Preferences::self()->oSDFont());

    m_osd->setDuration(Preferences::self()->oSDDuration());
    m_osd->setScreen(Preferences::self()->oSDScreen());
    m_osd->setShadow(Preferences::self()->oSDDrawShadow());

    m_osd->setOffset(Preferences::self()->oSDOffsetX(), Preferences::self()->oSDOffsetY());
    m_osd->setAlignment(static_cast<OSDWidget::Alignment>(Preferences::self()->oSDAlignment()));

    if(Preferences::self()->oSDUseCustomColors())
    {
        m_osd->setTextColor(Preferences::self()->oSDTextColor());
        QPalette p = m_osd->palette();
        p.setColor(m_osd->backgroundRole(), Preferences::self()->oSDBackgroundColor());
        m_osd->setPalette(p);
    }

    const QStringList groups = KSharedConfig::openConfig()->groupList().filter(
                                        QRegularExpression(QStringLiteral("ServerGroup [0-9]+")));
    QMap<int,QStringList> notifyList;
    QList<int> sgKeys;
    sgKeys.reserve(groups.size());

    if(!groups.isEmpty())
    {
        Konversation::ServerGroupHash serverGroups;
        Konversation::ChannelList channelHistory;
        Konversation::ServerSettings server;
        Konversation::ChannelSettings channel;

        for (const QString& groupName : groups) {
            KConfigGroup cgServerGroup(KSharedConfig::openConfig()->group(groupName));
            Konversation::ServerGroupSettingsPtr serverGroup(new Konversation::ServerGroupSettings);
            serverGroup->setName(cgServerGroup.readEntry("Name"));
            serverGroup->setSortIndex(groupName.section(QLatin1Char(' '), -1).toInt());
            serverGroup->setIdentityId(Preferences::identityByName(cgServerGroup.readEntry("Identity"))->id());
            serverGroup->setConnectCommands(cgServerGroup.readEntry("ConnectCommands"));
            serverGroup->setAutoConnectEnabled(cgServerGroup.readEntry("AutoConnect", false));
            serverGroup->setNotificationsEnabled(cgServerGroup.readEntry("EnableNotifications", true));
            serverGroup->setExpanded(cgServerGroup.readEntry("Expanded", false));

            notifyList.insert((*serverGroup).id(), cgServerGroup.readEntry("NotifyList", QString()).split(QLatin1Char(' '), Qt::SkipEmptyParts));

            const QStringList serverNames = cgServerGroup.readEntry("ServerList", QStringList());
            for (const QString& serverName : serverNames) {
                KConfigGroup cgServer(KSharedConfig::openConfig()->group(serverName));
                server.setHost(cgServer.readEntry("Server"));
                server.setPort(cgServer.readEntry<int>("Port", 0));
                server.setPassword(cgServer.readEntry("Password"));
                server.setSSLEnabled(cgServer.readEntry("SSLEnabled", false));
                server.setBypassProxy(cgServer.readEntry("BypassProxy", false));
                serverGroup->addServer(server);
            }

            //config->setGroup(groupName);
            const QStringList autoJoinChannels = cgServerGroup.readEntry("AutoJoinChannels", QStringList());

            for (const QString& channelName : autoJoinChannels) {
                KConfigGroup cgJoin(KSharedConfig::openConfig()->group(channelName));

                if (!cgJoin.readEntry("Name").isEmpty())
                {
                    channel.setName(cgJoin.readEntry("Name"));
                    channel.setPassword(cgJoin.readEntry("Password"));
                    serverGroup->addChannel(channel);
                }
            }

            //config->setGroup(groupName);
            const QStringList channelHistoryList = cgServerGroup.readEntry("ChannelHistory", QStringList());
            channelHistory.clear();

            for (const QString& channelName : channelHistoryList) {
                KConfigGroup cgChanHistory(KSharedConfig::openConfig()->group(channelName));

                if (!cgChanHistory.readEntry("Name").isEmpty())
                {
                    channel.setName(cgChanHistory.readEntry("Name"));
                    channel.setPassword(cgChanHistory.readEntry("Password"));
                    channel.setNotificationsEnabled(cgChanHistory.readEntry("EnableNotifications", true));
                    channelHistory.append(channel);
                }
            }

            serverGroup->setChannelHistory(channelHistory);

            serverGroups.insert(serverGroup->id(), serverGroup);
            sgKeys.append(serverGroup->id());
        }

        Preferences::setServerGroupHash(serverGroups);
    }

    // Notify Settings and lists.  Must follow Server List.
    Preferences::setNotifyList(notifyList);
    Preferences::self()->setNotifyDelay(Preferences::self()->notifyDelay());
    Preferences::self()->setUseNotify(Preferences::self()->useNotify());

    // Quick Buttons List

    // if there are button definitions in the config file, remove default buttons
    if (KSharedConfig::openConfig()->hasGroup(QStringLiteral("Button List"))) {
        Preferences::clearQuickButtonList();
    }

    KConfigGroup cgQuickButtons(KSharedConfig::openConfig()->group(QStringLiteral("Button List")));
    // Read all default buttons
    QStringList buttonList(Preferences::quickButtonList());
    // Read all quick buttons
    int index=0;
    while (cgQuickButtons.hasKey(QStringLiteral("Button%1").arg(index)))
    {
        buttonList.append(cgQuickButtons.readEntry(QStringLiteral("Button%1").arg(index++)));
    } // while
    // Put back the changed button list
    Preferences::setQuickButtonList(buttonList);

    // Autoreplace List

    // if there are autoreplace definitions in the config file, remove default entries
    if (KSharedConfig::openConfig()->hasGroup(QStringLiteral("Autoreplace List"))) {
        Preferences::clearAutoreplaceList();
    }

    KConfigGroup cgAutoreplace(KSharedConfig::openConfig()->group(QStringLiteral("Autoreplace List")));
    // Read all default entries
    QList<QStringList> autoreplaceList(Preferences::autoreplaceList());
    // Read all entries
    index=0;
    // legacy code for old autoreplace format 4/6/09
    QString autoReplaceString(QStringLiteral("Autoreplace"));
    while (cgAutoreplace.hasKey(autoReplaceString + QString::number(index)))
    {
  // read entry and get length of the string
        QString entry=cgAutoreplace.readEntry(autoReplaceString + QString::number(index++));
        int length=entry.length()-1;
        // if there's a "#" in the end, strip it (used to preserve blanks at the end of the replacement text)
        // there should always be one, but older versions did not do it, so we check first
        if (entry.at(length)==QLatin1Char('#'))
            entry.truncate(length);
        QString regex = entry.section(QLatin1Char(','),0,0);
        QString direction = entry.section(QLatin1Char(','),1,1);
        QString pattern = entry.section(QLatin1Char(','),2,2);
        QString replace = entry.section(QLatin1Char(','),3);
        // add entry to internal list
        autoreplaceList.append(QStringList { regex, direction, pattern, replace });
    } // while
    //end legacy code for old autoreplace format
    index=0; //new code for autoreplace config
    QString indexString(QString::number(index));
    QString regexString(QStringLiteral("Regex"));
    QString directString(QStringLiteral("Direction"));
    QString patternString(QStringLiteral("Pattern"));
    QString replaceString(QStringLiteral("Replace"));
    while (cgAutoreplace.hasKey(patternString + indexString))
    {
        QString pattern = cgAutoreplace.readEntry(patternString + indexString);
        QString regex = cgAutoreplace.readEntry(regexString + indexString, QStringLiteral("0"));
        QString direction = cgAutoreplace.readEntry(directString + indexString, QStringLiteral("o"));
        QString replace = cgAutoreplace.readEntry(replaceString + indexString, QString());
        if (!replace.isEmpty()) {
            int repLen=replace.length()-1;
            if (replace.at(repLen)==QLatin1Char('#')) {
                replace.truncate(repLen);
            }
        }
        if (!pattern.isEmpty()) {
            int patLen=pattern.length()-1;
            if (pattern.at(patLen)==QLatin1Char('#')) {
                pattern.truncate(patLen);
                }
        }
        index++;
        indexString = QString::number(index);
        autoreplaceList.append(QStringList { regex, direction, pattern, replace });
    }
    // Put back the changed autoreplace list
    Preferences::setAutoreplaceList(autoreplaceList);

    // Highlight List
    index = 0;
    while (KSharedConfig::openConfig()->hasGroup(QStringLiteral("Highlight%1").arg(index)))
    {
        KConfigGroup cgHighlight (KSharedConfig::openConfig()->group(QStringLiteral("Highlight%1").arg(index)));
        Preferences::addHighlight(
            cgHighlight.readEntry("Pattern"),
            cgHighlight.readEntry("RegExp", false),
            cgHighlight.readEntry("Color", QColor(Qt::black)),
            cgHighlight.readPathEntry("Sound", QString()),
            cgHighlight.readEntry("AutoText"),
            cgHighlight.readEntry("ChatWindows"),
            cgHighlight.readEntry("Notify", true)
            );
        index++;
    }

    // Ignore List
    KConfigGroup cgIgnoreList(KSharedConfig::openConfig()->group(QStringLiteral("Ignore List")));
    // Remove all default entries if there is at least one Ignore in the Preferences::file
    if (cgIgnoreList.hasKey("Ignore0"))
        Preferences::clearIgnoreList();
    // Read all ignores
    index=0;
    while (cgIgnoreList.hasKey(QStringLiteral("Ignore%1").arg(index)))
    {
        Preferences::addIgnore(cgIgnoreList.readEntry(QStringLiteral("Ignore%1").arg(index++)));
    }

    // Aliases
    KConfigGroup cgAliases(KSharedConfig::openConfig()->group(QStringLiteral("Aliases")));
    QStringList newList=cgAliases.readEntry("AliasList", QStringList());
    if (!newList.isEmpty())
        Preferences::self()->setAliasList(newList);

    // Channel Encodings
    KConfigGroup cgEncodings(KSharedConfig::openConfig()->group(QStringLiteral("Encodings")));
    const QMap<QString,QString> encodingEntries = cgEncodings.entryMap();

    const QRegularExpression reg(QStringLiteral("^([^\\s]+) ([^\\s]+)\\s?([^\\s]*)$"));
    for (auto it = encodingEntries.begin(), end = encodingEntries.end(); it != end; ++it) {
        const QRegularExpressionMatch match = reg.match(it.key());
        if(match.hasMatch())
        {
            if(match.captured(1) == QLatin1String("ServerGroup") && !match.captured(3).isEmpty())
                Preferences::setChannelEncoding(sgKeys.at(match.captured(2).toInt()), match.captured(3), it.value());
            else
                Preferences::setChannelEncoding(match.captured(1), match.captured(2), it.value());
        }
    }

    // Spell Checking Languages
    KConfigGroup cgSpellCheckingLanguages(KSharedConfig::openConfig()->group(QStringLiteral("Spell Checking Languages")));
    const QMap<QString, QString> spellCheckingLanguageEntries = cgSpellCheckingLanguages.entryMap();

    for (auto it = spellCheckingLanguageEntries.begin(), end = spellCheckingLanguageEntries.end(); it != end; ++it) {
        const QRegularExpressionMatch match = reg.match(it.key());
        if (!match.hasMatch()) {
            continue;
        }
        if (match.captured(1) == QLatin1String("ServerGroup") && !match.captured(3).isEmpty())
        {
            ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(sgKeys.at(match.captured(2).toInt()));

            if (serverGroup) {
                Preferences::setSpellCheckingLanguage(serverGroup, match.captured(3), it.value());
            }
        }
        else {
            Preferences::setSpellCheckingLanguage(match.captured(1), match.captured(2), it.value());
        }
    }

    fetchQueueRates();

    updateProxySettings();
}

void Application::saveOptions(bool updateGUI)
{
    // template:    KConfigGroup  (KSharedConfig::openConfig()->group( ));

    //KConfig* config=KSharedConfig::openConfig();

//    Should be handled in NicklistBehaviorConfigController now
//    config->setGroup("Sort Nicknames");

    // Clean up identity list
    const QStringList identities=KSharedConfig::openConfig()->groupList().filter(
                                            QRegularExpression(QStringLiteral("Identity [0-9]+")));
    // remove old identity list from Preferences::file to keep numbering under control
    for (const QString& identityGroup : identities) {
        KSharedConfig::openConfig()->deleteGroup(identityGroup);
    }

    const IdentityList identityList = Preferences::identityList();
    int index = 0;

    for (const auto& identity : identityList) {
        KConfigGroup cgIdentity(KSharedConfig::openConfig()->group(QStringLiteral("Identity %1").arg(index)));

        cgIdentity.writeEntry("Name",identity->getName());
        cgIdentity.writeEntry("Ident",identity->getIdent());
        cgIdentity.writeEntry("Realname",identity->getRealName());
        cgIdentity.writeEntry("Nicknames",identity->getNicknameList());
        cgIdentity.writeEntry("AuthType",identity->getAuthType());
        cgIdentity.writeEntry("Password",identity->getAuthPassword());
        cgIdentity.writeEntry("Bot",identity->getNickservNickname());
        cgIdentity.writeEntry("NickservCommand",identity->getNickservCommand());
        cgIdentity.writeEntry("SaslAccount",identity->getSaslAccount());
        cgIdentity.writeEntry("PemClientCertFile", identity->getPemClientCertFile().toString());
        cgIdentity.writeEntry("InsertRememberLineOnAway", identity->getInsertRememberLineOnAway());
        cgIdentity.writeEntry("ShowAwayMessage",identity->getRunAwayCommands());
        cgIdentity.writeEntry("AwayMessage",identity->getAwayCommand());
        cgIdentity.writeEntry("ReturnMessage",identity->getReturnCommand());
        cgIdentity.writeEntry("AutomaticAway", identity->getAutomaticAway());
        cgIdentity.writeEntry("AwayInactivity", identity->getAwayInactivity());
        cgIdentity.writeEntry("AutomaticUnaway", identity->getAutomaticUnaway());
        cgIdentity.writeEntry("QuitReason",identity->getQuitReason());
        cgIdentity.writeEntry("PartReason",identity->getPartReason());
        cgIdentity.writeEntry("KickReason",identity->getKickReason());
        cgIdentity.writeEntry("PreShellCommand",identity->getShellCommand());
        cgIdentity.writeEntry("Codec",identity->getCodecName());
        cgIdentity.writeEntry("AwayReason",identity->getAwayMessage());
        cgIdentity.writeEntry("AwayNick", identity->getAwayNickname());
        index++;
    } // endfor

    // Remove the old servergroups from the config
    const QStringList serverGroupGroups =
        KSharedConfig::openConfig()->groupList().filter(QRegularExpression(QStringLiteral("ServerGroup [0-9]+")));

    for (const QString& serverGroupGroup : serverGroupGroups) {
        KSharedConfig::openConfig()->deleteGroup(serverGroupGroup);
    }

    // Remove the old servers from the config
    const QStringList serverGroups =
        KSharedConfig::openConfig()->groupList().filter(QRegularExpression(QStringLiteral("Server [0-9]+")));

    for (const QString& serverGroup : serverGroups) {
        KSharedConfig::openConfig()->deleteGroup(serverGroup);
    }

    // Remove the old channels from the config
    const QStringList channelGroups =
        KSharedConfig::openConfig()->groupList().filter(QRegularExpression(QStringLiteral("Channel [0-9]+")));

    for (const QString& channelGroup : channelGroups) {
        KSharedConfig::openConfig()->deleteGroup(channelGroup);
    }

    // Add the new servergroups to the config
    const Konversation::ServerGroupHash serverGroupHash = Preferences::serverGroupHash();

    QMap<int, Konversation::ServerGroupSettingsPtr> sortedServerGroupMap;

    // Make the indices in the group headers reflect the server list dialog sorting.
    for (const auto& serverGroup : serverGroupHash) {
        sortedServerGroupMap.insert(serverGroup->sortIndex(), serverGroup);
    }

    index = 0;
    int index2 = 0;
    int index3 = 0;
    QStringList servers;
    QStringList channels;
    QStringList channelHistory;
    QHash<int, int> sgKeys;

    for (const auto& serverGroup : std::as_const(sortedServerGroupMap)) {
        const Konversation::ServerList serverList = serverGroup->serverList();
        servers.clear();
        servers.reserve(serverList.size());

        sgKeys.insert(serverGroup->id(), index);

        for (const auto& server : serverList) {
            const QString groupName = QStringLiteral("Server %1").arg(index2);
            servers.append(groupName);
            KConfigGroup cgServer(KSharedConfig::openConfig()->group(groupName));
            cgServer.writeEntry("Server", server.host());
            cgServer.writeEntry("Port", server.port());
            cgServer.writeEntry("Password", server.password());
            cgServer.writeEntry("SSLEnabled", server.SSLEnabled());
            cgServer.writeEntry("BypassProxy", server.bypassProxy());
            index2++;
        }

        const Konversation::ChannelList channelList = serverGroup->channelList();
        channels.clear();
        channels.reserve(channelList.size());

        for (const auto& channel : channelList) {
            const QString groupName = QStringLiteral("Channel %1").arg(index3);
            channels.append(groupName);
            KConfigGroup cgChannel(KSharedConfig::openConfig()->group(groupName));
            cgChannel.writeEntry("Name", channel.name());
            cgChannel.writeEntry("Password", channel.password());
            index3++;
        }

        const Konversation::ChannelList channelHistoryList = serverGroup->channelHistory();
        channelHistory.clear();
        channelHistory.reserve(channelHistoryList.size());

        for (const auto& channel : channelHistoryList) {
            // TODO FIXME: is it just me or is this broken?
            const QString groupName = QStringLiteral("Channel %1").arg(index3);
            channelHistory.append(groupName);
            KConfigGroup cgChannelHistory(KSharedConfig::openConfig()->group(groupName));
            cgChannelHistory.writeEntry("Name", channel.name());
            cgChannelHistory.writeEntry("Password", channel.password());
            cgChannelHistory.writeEntry("EnableNotifications", channel.enableNotifications());
            index3++;
        }

        QString sgn = QStringLiteral("ServerGroup %1").arg(index);
        KConfigGroup cgServerGroup(KSharedConfig::openConfig()->group(sgn));
        cgServerGroup.writeEntry("Name", serverGroup->name());
        cgServerGroup.writeEntry("Identity", serverGroup->identity()->getName());
        cgServerGroup.writeEntry("ServerList", servers);
        cgServerGroup.writeEntry("AutoJoinChannels", channels);
        cgServerGroup.writeEntry("ConnectCommands", serverGroup->connectCommands());
        cgServerGroup.writeEntry("AutoConnect", serverGroup->autoConnectEnabled());
        cgServerGroup.writeEntry("ChannelHistory", channelHistory);
        cgServerGroup.writeEntry("EnableNotifications", serverGroup->enableNotifications());
        cgServerGroup.writeEntry("Expanded", serverGroup->expanded());
        cgServerGroup.writeEntry("NotifyList",Preferences::notifyStringByGroupId(serverGroup->id()));
        index++;
    }

    KSharedConfig::openConfig()->deleteGroup(QStringLiteral("Server List"));

    // Ignore List
    KSharedConfig::openConfig()->deleteGroup(QStringLiteral("Ignore List"));
    KConfigGroup cgIgnoreList(KSharedConfig::openConfig()->group(QStringLiteral("Ignore List")));
    QList<Ignore*> ignoreList=Preferences::ignoreList();
    for (int i = 0; i < ignoreList.size(); ++i) {
        cgIgnoreList.writeEntry(QStringLiteral("Ignore%1").arg(i), QStringLiteral("%1,%2").arg(ignoreList.at(i)->getName()).arg(ignoreList.at(i)->getFlags()));
    }

    // Channel Encodings
    // remove all entries once
    KSharedConfig::openConfig()->deleteGroup(QStringLiteral("Channel Encodings")); // legacy Jun 29, 2009
    KSharedConfig::openConfig()->deleteGroup(QStringLiteral("Encodings"));
    KConfigGroup cgEncoding(KSharedConfig::openConfig()->group(QStringLiteral("Encodings")));
    const QList<int> encServers = Preferences::channelEncodingsServerGroupIdList();
    //i have no idea these would need to be sorted //encServers.sort();
    for (int encServer : encServers) {
        Konversation::ServerGroupSettingsPtr sgsp = Preferences::serverGroupById(encServer);

        if ( sgsp )  // sgsp == 0 when the entry is of QuickConnect or something?
        {
            const QStringList encChannels = Preferences::channelEncodingsChannelList(encServer);
            //ditto //encChannels.sort();
            for (const QString& encChannel : encChannels) {
                QString enc = Preferences::channelEncoding(encServer, encChannel);
                QString key = QLatin1Char(' ') + encChannel;
                if (sgKeys.contains(encServer))
                    key.prepend(QStringLiteral("ServerGroup ") + QString::number(sgKeys.value(encServer)));
                else
                    key.prepend(sgsp->name());
                cgEncoding.writeEntry(key, enc);
            }
        }
    }

    // Spell Checking Languages
    KSharedConfig::openConfig()->deleteGroup(QStringLiteral("Spell Checking Languages"));
    KConfigGroup cgSpellCheckingLanguages(KSharedConfig::openConfig()->group(QStringLiteral("Spell Checking Languages")));

    QHashIterator<Konversation::ServerGroupSettingsPtr, QHash<QString, QString> > i(Preferences::serverGroupSpellCheckingLanguages());

    while (i.hasNext())
    {
        i.next();

        QHashIterator<QString, QString> i2(i.value());

        while (i2.hasNext())
        {
            i2.next();

            if (sgKeys.contains(i.key()->id()))
                cgSpellCheckingLanguages.writeEntry(QStringLiteral("ServerGroup ") + QString::number(sgKeys.value(i.key()->id())) + QLatin1Char(' ') + i2.key(), i2.value());
        }
    }

    QHashIterator<QString, QHash<QString, QString> > i3(Preferences::serverSpellCheckingLanguages());

    while (i3.hasNext())
    {
        i3.next();

        QHashIterator<QString, QString> i4(i3.value());

        while (i4.hasNext())
        {
            i4.next();

            cgSpellCheckingLanguages.writeEntry(i3.key() + QLatin1Char(' ') + i4.key(), i4.value());
        }
    }

    KSharedConfig::openConfig()->sync();

    if(updateGUI)
        Q_EMIT appearanceChanged();
}

void Application::fetchQueueRates()
{
    //The following rate was found in the rc for all queues, which were deliberately bad numbers chosen for debugging.
    //Its possible that the static array was constructed or deconstructed at the wrong time, and so those values saved
    //in the rc. When reading the values out of the rc, we must check to see if they're this specific value,
    //and if so, reset to defaults. --argonel
    IRCQueue::EmptyingRate shit(6, 50000, IRCQueue::EmptyingRate::Lines);
    int bad = 0;
    for (int i=0; i <= countOfQueues(); i++)
    {
        QList<int> r = Preferences::self()->queueRate(i);
        staticrates[i] = IRCQueue::EmptyingRate(r[0], r[1]*1000, IRCQueue::EmptyingRate::RateType(r[2]));
        if (staticrates[i] == shit)
            bad++;
    }
    if (bad == 3)
        resetQueueRates();
}

void Application::stashQueueRates()
{
    for (int i=0; i <= countOfQueues(); i++)
    {
        const QList<int> r {
            staticrates[i].m_rate,
            staticrates[i].m_interval / 1000,
            static_cast<int>(staticrates[i].m_type),
        };
        Preferences::self()->setQueueRate(i, r);
    }
}

void Application::resetQueueRates()
{
    for (int i=0; i <= countOfQueues(); i++)
    {
        Preferences::self()->queueRateItem(i)->setDefault();
        QList<int> r=Preferences::self()->queueRate(i);
        staticrates[i]=IRCQueue::EmptyingRate(r[0], r[1]*1000, IRCQueue::EmptyingRate::RateType(r[2]));
    }
}

void Application::storeUrl(const QString& origin, const QString& newUrl, const QDateTime& dateTime)
{
    QString url(newUrl);

    url.replace(QStringLiteral("&amp;"), QStringLiteral("&"));

    const QList<QStandardItem*> existing = m_urlModel->findItems(url, Qt::MatchExactly, 1);

    for (QStandardItem* item : existing) {
        if (m_urlModel->item(item->row(), 0)->data(Qt::DisplayRole).toString() == origin)
            m_urlModel->removeRow(item->row());
    }

    m_urlModel->insertRow(0);
    m_urlModel->setData(m_urlModel->index(0, 0), origin, Qt::DisplayRole);
    m_urlModel->setData(m_urlModel->index(0, 1), url, Qt::DisplayRole);

    auto* dateItem = new UrlDateItem(dateTime);
    m_urlModel->setItem(0, 2, dateItem);
}

void Application::openQuickConnectDialog()
{
    quickConnectDialog = new QuickConnectDialog(mainWindow);
    connect(quickConnectDialog, &QuickConnectDialog::connectClicked,
        m_connectionManager, QOverload<Konversation::ConnectionFlag, const QString&, const QString&, const QString&, const QString&, const QString&, bool>::of(&ConnectionManager::connectTo));
    quickConnectDialog->show();
}

void Application::sendMultiServerCommand(const QString& command, const QString& parameter)
{
    const QList<Server*> serverList = getConnectionManager()->getServerList();

    for (Server* server : serverList)
        server->executeMultiServerCommand(command, parameter);
}

void Application::splitNick_Server(const QString& nick_server, QString &ircnick, QString &serverOrGroup)
{
    //kaddresbook uses the utf separator 0xE120, so treat that as a separator as well
    QString nickServer = nick_server;
    nickServer.replace(QChar(0xE120), QStringLiteral("@"));
    ircnick = nickServer.section(QLatin1Char('@'),0,0);
    serverOrGroup = nickServer.section(QLatin1Char('@'),1);
}

NickInfoPtr Application::getNickInfo(const QString &ircnick, const QString &serverOrGroup)
{
    const QList<Server*> serverList = getConnectionManager()->getServerList();
    NickInfoPtr nickInfo;
    QString lserverOrGroup = serverOrGroup.toLower();
    for (Server* lookServer : serverList) {
        if (lserverOrGroup.isEmpty()
            || lookServer->getServerName().toLower()==lserverOrGroup
            || lookServer->getDisplayName().toLower()==lserverOrGroup)
        {
            nickInfo = lookServer->getNickInfo(ircnick);
            if (nickInfo)
                return nickInfo;         //If we found one
        }
    }
    return static_cast<NickInfoPtr>(nullptr);
}

// auto replace on input/output
QPair<QString, int> Application::doAutoreplace(const QString& text, bool output, int cursorPos) const
{
    // get autoreplace list
    const QList<QStringList> autoreplaceList = Preferences::autoreplaceList();
    // working copy
    QString line=text;

    // loop through the list of replacement patterns
    for (const QStringList& definition : autoreplaceList) {
        // split definition in parts
        QString regex=definition.at(0);
        QString direction=definition.at(1);
        QString pattern=definition.at(2);
        QString replacement=definition.at(3);

        QString isDirection=output ? QStringLiteral("o") : QStringLiteral("i");

        // only replace if this pattern is for the specific direction or both directions
        if (direction==isDirection || direction==QStringLiteral("io"))
        {
            // regular expression pattern?
            if (regex== QLatin1Char('1'))
            {
                // create regex from pattern
                const QRegularExpression needleReg(pattern);
                int index = 0;
                int newIndex = index;

                do {
                    QRegularExpressionMatch rmatch;
                    // find matches
                    index = line.indexOf(needleReg, index, &rmatch);

                    if (index != -1)
                    {
                        // remember captured patterns
                        const QStringList captures = rmatch.capturedTexts();
                        QString replaceWith = replacement;

                        replaceWith.replace(QStringLiteral("%%"),QStringLiteral("%\x01")); // escape double %
                        // replace %0-9 in regex groups
                        for (int capture=0;capture<captures.count();capture++)
                        {
                            QString search = QStringLiteral("%%1").arg(capture);
                            replaceWith.replace(search, captures[capture]);
                        }
                        //Explanation why this is important so we don't forget:
                        //If somebody has a regex that say has a replacement of url.com/%1/%2 and the
                        //regex can either match one or two patterns, if the 2nd pattern match is left,
                        //the url is invalid (url.com/match/%2). This is expected regex behavior I'd assume.
                        replaceWith.remove(QRegularExpression(QStringLiteral("%[0-9]")));

                        replaceWith.replace(QStringLiteral("%\x01"),QStringLiteral("%")); // return escaped % to normal
                        // allow for var expansion in autoreplace
                        replaceWith = Konversation::doVarExpansion(replaceWith);
                        // replace input with replacement
                        line.replace(index, captures[0].length(), replaceWith);

                        newIndex = index + replaceWith.length();

                        if (cursorPos > -1 && cursorPos >= index)
                        {
                            if (cursorPos < index + captures[0].length())
                                cursorPos = newIndex;
                            else
                            {
                                if (captures[0].length() > replaceWith.length())
                                    cursorPos -= captures[0].length() - replaceWith.length();
                                else
                                    cursorPos += replaceWith.length() - captures[0].length();
                            }
                        }

                        index = newIndex;
                    }
                } while (index >= 0 && index < line.length());
            }
            else
            {
                int index = line.indexOf(pattern);
                while (index>=0)
                {
                    int length,nextLength,patLen,repLen;
                    patLen=pattern.length();

                    repLen=replacement.length();
                    length=index;
                    length+=patLen;
                    nextLength=length;
                    //nextlength is used to account for the replacement taking up less space
                    QChar before,after;
                    if (index!=0) before = line.at(index-1);
                    if (line.length() > length) after = line.at(length);

                    if (index==0 || before.isSpace() || before.isPunct())
                    {
                        if (line.length() == length || after.isSpace() || after.isPunct())
                        {
                            // allow for var expansion in autoreplace
                            replacement = Konversation::doVarExpansion(replacement);
                            line.replace(index,patLen,replacement);
                            nextLength = index+repLen;
                        }
                    }

                    if (cursorPos > -1 && cursorPos >= index)
                    {
                        if (cursorPos < length)
                            cursorPos = nextLength;
                        else
                        {
                            if (patLen > repLen)
                                cursorPos -= patLen - repLen;
                            else
                                cursorPos += repLen - patLen;
                        }
                    }

                    index = line.indexOf(pattern, nextLength);
                }
            }
        }
    }

    return QPair<QString, int>(line, cursorPos);
}

void Application::doInlineAutoreplace(KTextEdit* textEdit) const
{
    QTextCursor cursor(textEdit->document());

    cursor.beginEditBlock();
    const QPair<QString, int>& replace = Application::instance()->doAutoreplace(textEdit->toPlainText(), true, textEdit->textCursor().position());
    cursor.select(QTextCursor::Document);
    cursor.insertText(replace.first);
    cursor.setPosition(replace.second);
    cursor.endEditBlock();

    textEdit->setTextCursor(cursor);
}


void Application::openUrl(const QString& url)
{
    if (!Preferences::self()->useCustomBrowser()
        || url.startsWith(QLatin1String("mailto:"))
        || url.startsWith(QLatin1String("amarok:"))) {
        if (url.startsWith(QLatin1String("irc://")) || url.startsWith(QLatin1String("ircs://"))) {
            Application::instance()->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, url);
            return;
        }

#ifdef Q_OS_WIN
        QDesktopServices::openUrl(QUrl::fromUserInput(url));
#else
        auto *job = new KIO::OpenUrlJob(QUrl(url));
        job->setFollowRedirections(false);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, Application::instance()->getMainWindow()));
        job->start();
        return;
#endif
    }

    // Use custom browser
    QHash<QChar,QString> map;
    map.insert(QLatin1Char('u'), url);
    const QString cmd = KMacroExpander::expandMacrosShellQuote(Preferences::webBrowserCmd(), map);
    const QStringList args = KShell::splitArgs(cmd);

    if (!args.isEmpty()) {
        KProcess::startDetached(args);
    }
}

Konversation::Sound* Application::sound() const
{
    if (!m_sound)
        m_sound = new Konversation::Sound;

    return m_sound;
}
 
OSDWidget* Application::osd() const
{
    return m_osd;
}

void Application::updateProxySettings()
{
    if (Preferences::self()->proxyEnabled())
    {
        QNetworkProxy proxy;

        if (Preferences::self()->proxyType() == Preferences::Socksv5Proxy)
        {
            proxy.setType(QNetworkProxy::Socks5Proxy);
        }
        else
        {
            proxy.setType(QNetworkProxy::HttpProxy);
        }

        proxy.setHostName(Preferences::self()->proxyAddress());
        proxy.setPort(static_cast<quint16>(Preferences::self()->proxyPort()));
        proxy.setUser(Preferences::self()->proxyUsername());
        QString password;

        if(wallet())
        {
            int ret = wallet()->readPassword(QStringLiteral("ProxyPassword"), password);

            if(ret != 0)
            {
                qCCritical(KONVERSATION_LOG) << "Failed to read the proxy password from the wallet, error code:" << ret;
            }
        }

        proxy.setPassword(password);
        QNetworkProxy::setApplicationProxy(proxy);
    }
    else
    {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
    }
}

KWallet::Wallet* Application::wallet()
{
    if(!m_wallet)
    {
        WId winid = 0;

        if(mainWindow)
            winid = mainWindow->winId();

        m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), winid);

        if(!m_wallet)
            return nullptr;

        connect(m_wallet, &KWallet::Wallet::walletClosed, this, &Application::closeWallet);

        if(!m_wallet->hasFolder(QStringLiteral("Konversation")))
        {
            if(!m_wallet->createFolder(QStringLiteral("Konversation")))
            {
                qCCritical(KONVERSATION_LOG) << "Failed to create folder Konversation in the network wallet.";
                closeWallet();
                return nullptr;
            }
        }

        if(!m_wallet->setFolder(QStringLiteral("Konversation")))
        {
            qCCritical(KONVERSATION_LOG) << "Failed to set active folder to Konversation in the network wallet.";
            closeWallet();
            return nullptr;
        }
    }

    return m_wallet;
}

void Application::closeWallet()
{
    delete m_wallet;
    m_wallet = nullptr;
}

void Application::handleActivate(const QStringList& arguments)
{
    m_commandLineParser->parse(arguments.isEmpty()? QStringList(applicationFilePath()) : arguments);

    if(m_commandLineParser->isSet(QStringLiteral("restart")))
    {
        m_restartArguments = arguments;
    }

    newInstance(m_commandLineParser);

    activateForStartLikeCall();
}

void Application::handleOpen(const QList<QUrl>& urls)
{
    for (const QUrl& url : urls) {
        getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, url.toString());
    }
    activateForStartLikeCall();
}

void Application::activateForStartLikeCall()
{
    mainWindow->show();
    KWindowSystem::updateStartupId(mainWindow->windowHandle());
    mainWindow->activateAndRaiseWindow();
}

#include "moc_application.cpp"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
