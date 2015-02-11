/*
  This program is free software; you can redistribute it and/or modify
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

#include "application.h"
#include "connectionmanager.h"
#include "scriptlauncher.h"
#include "transfermanager.h"
#include "viewcontainer.h"
#include "urlcatcher.h"
#include "highlight.h"
#include "server.h"
#include "sound.h"
#include "quickconnectdialog.h"
#include "dbus.h"
#include "servergroupsettings.h"
#include "serversettings.h"
#include "channel.h"
#include "images.h"
#include "notificationhandler.h"
#include "awaymanager.h"

#include <QTextCodec>
#include <QRegExp>
#include <QtDBus/QDBusConnection>
#include <QNetworkProxy>
#include <QWaitCondition>
#include <QStandardItemModel>
#include <QFileInfo>
#include <QTextCursor>
#include <QDesktopServices>
#include <QCommandLineParser>

#include <KRun>
#include <KConfig>
#include <KShell>
#include <KMacroExpander>
#include <kwallet.h>
#include <KTextEdit>
#include <KSharedConfig>


using namespace Konversation;

Application::Application(int &argc, char **argv)
: QApplication(argc, argv)
{
    mainWindow = 0;
    m_restartScheduled = false;
    m_connectionManager = 0;
    m_awayManager = 0;
    m_scriptLauncher = 0;
    quickConnectDialog = 0;
    osd = 0;
    m_wallet = NULL;
    m_images = 0;
    m_sound = 0;
    m_dccTransferManager = 0;
    m_notificationHandler = 0;
    m_urlModel = 0;
    dbusObject = 0;
    identDBus = 0;
    m_networkConfigurationManager = 0;
}

Application::~Application()
{
    qDebug();

    if (!m_images)
        return; // Nothing to do, newInstance() has never been called.

    stashQueueRates();
    Preferences::self()->save(); // FIXME i can't figure out why this isn't in saveOptions --argonel
    saveOptions(false);

    // Delete m_dccTransferManager here as its destructor depends on the main loop being in tact which it
    // won't be if if we wait till Qt starts deleting parent pointers.
    delete m_dccTransferManager;

    delete m_images;
    delete m_sound;
    //delete dbusObject;
    //delete prefsDCOP;
    //delete identDBus;
    delete osd;
    osd = 0;
    closeWallet();

    delete m_networkConfigurationManager;

    if (m_restartScheduled) implementRestart();
}

void Application::implementRestart()
{
#if 0 // FIXME KF5 Port: --restart
    QStringList argumentList;

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    argumentList = args->allArguments();

    // Pop off the executable name. May not be the first argument in argv
    // everywhere, so verify first.
    if (QFileInfo(argumentList.first()) == QFileInfo(QCoreApplication::applicationFilePath()))
        argumentList.removeFirst();

    // Don't round-trip --restart.
    argumentList.removeAll(QStringLiteral("--restart"));

    // Avoid accumulating multiple --startupdelay arguments across multiple
    // uses of restart().
    if (argumentList.contains(QStringLiteral("--startupdelay")))
    {
        int index = argumentList.lastIndexOf(QStringLiteral("--startupdelay"));

        if (index < argumentList.count() - 1 && !argumentList.at(index + 1).startsWith(QLatin1Char('-')))
        {
            QString delayArgument = argumentList.at(index + 1);

            bool ok;

            uint delay = delayArgument.toUInt(&ok, 10);

            // If the argument is invalid or too low, raise to at least 2000 msecs.
            if (!ok || delay < 2000)
                argumentList.replace(index + 1, QStringLiteral("2000"));
        }
    }
    else
        argumentList << QStringLiteral("--startupdelay") << QStringLiteral("2000");

    KProcess::startDetached(QCoreApplication::applicationFilePath(), argumentList);
#endif
}

void Application::newInstance(QCommandLineParser *args)
{
    QString url;
    if (args->positionalArguments().count() > 0)
        url = args->positionalArguments().at(0);

    if (!mainWindow)
    {
        connect(this, &Application::aboutToQuit, this, &Application::prepareShutdown);

        m_connectionManager = new ConnectionManager(this);

        m_awayManager = new AwayManager(this);

        connect(m_connectionManager, &ConnectionManager::identityOnline, m_awayManager, &AwayManager::identityOnline);
        connect(m_connectionManager, &ConnectionManager::identityOffline, m_awayManager, &AwayManager::identityOffline);
        connect(m_connectionManager, &ConnectionManager::connectionChangedAwayState, m_awayManager, &AwayManager::updateGlobalAwayAction);

        m_networkConfigurationManager = new QNetworkConfigurationManager();
        connect(m_networkConfigurationManager, SIGNAL(onlineStateChanged(bool)), m_connectionManager, SLOT(onOnlineStateChanged(bool)));

        m_scriptLauncher = new ScriptLauncher(this);

        // an instance of DccTransferManager needs to be created before GUI class instances' creation.
        m_dccTransferManager = new DCC::TransferManager(this);

        // make sure all vars are initialized properly
        quickConnectDialog = 0;

        // Sound object used to play sound is created when needed.
        m_sound = NULL;

        // initialize OSD display here, so we can read the Preferences::properly
        osd = new OSDWidget( QStringLiteral("Konversation") );

        Preferences::self();
        readOptions();

        // Images object providing LEDs, NickIcons
        m_images = new Images();

        m_urlModel = new QStandardItemModel(0, 3, this);

        // Auto-alias scripts.  This adds any missing aliases
        QStringList aliasList(Preferences::self()->aliasList());
        const QStringList scripts(Preferences::defaultAliasList());
        bool changed = false;
        for ( QStringList::ConstIterator it = scripts.constBegin(); it != scripts.constEnd(); ++it )
        {
            if(!aliasList.contains(*it)) {
                changed = true;
                aliasList.append(*it);
            }
        }
        if(changed)
            Preferences::self()->setAliasList(aliasList);

        // open main window
        mainWindow = new MainWindow();

        connect(mainWindow.data(), &MainWindow::showQuickConnectDialog, this, &Application::openQuickConnectDialog);
        connect(Preferences::self(), &Preferences::updateTrayIcon, mainWindow.data(), &MainWindow::updateTrayIcon);
        connect(mainWindow.data(), &MainWindow::endNotification, osd, &OSDWidget::hide);
        // take care of user style changes, setting back colors and stuff

        // apply GUI settings
        emit appearanceChanged();

        if (Preferences::self()->showTrayIcon() && Preferences::self()->hideToTrayOnStartup())
            mainWindow->hide();
        else
            mainWindow->show();

        bool openServerList = Preferences::self()->showServerList();

        // handle autoconnect on startup
        Konversation::ServerGroupHash serverGroups = Preferences::serverGroupHash();

        if (!args->isSet(QStringLiteral("noautoconnect")) && url.isEmpty() && !args->isSet(QStringLiteral("server")))
        {
            QList<ServerGroupSettingsPtr> serversToAutoconnect;
            QHashIterator<int, Konversation::ServerGroupSettingsPtr> it(serverGroups);
            while(it.hasNext())
            {
                it.next();
                if (it.value()->autoConnectEnabled())
                {
                    openServerList = false;
                    serversToAutoconnect << it.value();
                }
            }

            std::sort(serversToAutoconnect.begin(), serversToAutoconnect.end(), [] (const ServerGroupSettingsPtr &left, const ServerGroupSettingsPtr &right)
            {
                return left->sortIndex() < right->sortIndex();
            });

            for (QList<ServerGroupSettingsPtr>::iterator it = serversToAutoconnect.begin(); it != serversToAutoconnect.end(); ++it)
            {
                m_connectionManager->connectTo(Konversation::CreateNewConnection, (*it)->id());
            }
        }

        if (openServerList) mainWindow->openServerList();

        connect(this, SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)), this, SLOT(saveOptions()));

        // prepare dbus interface
        dbusObject = new Konversation::DBus(this);
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/irc"), dbusObject, QDBusConnection::ExportNonScriptableSlots);
        identDBus = new Konversation::IdentDBus(this);
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/identity"), identDBus, QDBusConnection::ExportNonScriptableSlots);

        if (dbusObject)
        {
            connect(dbusObject,SIGNAL (dbusMultiServerRaw(QString)),
                this,SLOT (dbusMultiServerRaw(QString)) );
            connect(dbusObject,SIGNAL (dbusRaw(QString,QString)),
                this,SLOT (dbusRaw(QString,QString)) );
            connect(dbusObject,SIGNAL (dbusSay(QString,QString,QString)),
                this,SLOT (dbusSay(QString,QString,QString)) );
            connect(dbusObject,SIGNAL (dbusInfo(QString)),
                this,SLOT (dbusInfo(QString)) );
            connect(dbusObject,SIGNAL (dbusInsertMarkerLine()),
                mainWindow,SIGNAL(insertMarkerLine()));
            connect(dbusObject, SIGNAL(connectTo(Konversation::ConnectionFlag,QString,QString,QString,QString,QString,bool)),
                m_connectionManager, SLOT(connectTo(Konversation::ConnectionFlag,QString,QString,QString,QString,QString,bool)));
        }

        m_notificationHandler = new Konversation::NotificationHandler(this);

        connect(this, &Application::appearanceChanged, this, &Application::updateProxySettings);
    }
#if 0 //FIXME KF5 Port: --restart
    else if (args->isSet(QStringLiteral("restart")))
    {
        restart();

        return;
    }
#endif

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

Application* Application::instance()
{
    return static_cast<Application*>(QApplication::instance());
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
        m_awayManager = 0;
    }

    if (m_connectionManager)
    {
        m_connectionManager->quitServers();
        m_connectionManager->blockSignals(true);
        delete m_connectionManager;
        m_connectionManager = 0;
    }
}

bool Application::event(QEvent* event)
{
    if (event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::ApplicationFontChange) {
        emit appearanceChanged();
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
    mainWindow->getViewContainer()->appendToFrontmost(i18n("D-Bus"), string, 0);
}

void Application::readOptions()
{
    // get standard config file

    // read nickname sorting order for channel nick lists
    KConfigGroup cgSortNicknames(KSharedConfig::openConfig()->group("Sort Nicknames"));

    QString sortOrder=cgSortNicknames.readEntry("SortOrder");
    QStringList sortOrderList=sortOrder.split(QString());
    sortOrderList.sort();
    if (sortOrderList.join(QString())!=QStringLiteral("-hopqv"))
    {
        sortOrder=Preferences::defaultNicknameSortingOrder();
        Preferences::self()->setSortOrder(sortOrder);
    }

    // Identity list
    QStringList identityList=KSharedConfig::openConfig()->groupList().filter(QRegExp(QStringLiteral("Identity [0-9]+")));
    if (!identityList.isEmpty())
    {
        Preferences::clearIdentityList();

        for(int index=0;index<identityList.count();index++)
        {
            IdentityPtr newIdentity(new Identity());
            KConfigGroup cgIdentity(KSharedConfig::openConfig()->group(identityList[index]));

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

    osd->setEnabled(Preferences::self()->useOSD());

    //How to load the font from the text?
    osd->setFont(Preferences::self()->oSDFont());

    osd->setDuration(Preferences::self()->oSDDuration());
    osd->setScreen(Preferences::self()->oSDScreen());
    osd->setShadow(Preferences::self()->oSDDrawShadow());

    osd->setOffset(Preferences::self()->oSDOffsetX(), Preferences::self()->oSDOffsetY());
    osd->setAlignment((OSDWidget::Alignment)Preferences::self()->oSDAlignment());

    if(Preferences::self()->oSDUseCustomColors())
    {
        osd->setTextColor(Preferences::self()->oSDTextColor());
        QPalette p = osd->palette();
        p.setColor(osd->backgroundRole(), Preferences::self()->oSDBackgroundColor());
        osd->setPalette(p);
    }

    // Check if there is old server list config //TODO FIXME why are we doing this here?
    KConfigGroup cgServerList(KSharedConfig::openConfig()->group("Server List"));

    // Read the new server settings
    QStringList groups = KSharedConfig::openConfig()->groupList().filter(QRegExp(QStringLiteral("ServerGroup [0-9]+")));
    QMap<int,QStringList> notifyList;
    QList<int> sgKeys;

    if(!groups.isEmpty())
    {
        Konversation::ServerGroupHash serverGroups;
        QStringList::iterator it;
        QStringList tmp1;
        QStringList::iterator it2;
        int index = 0;
        Konversation::ChannelList channelHistory;
        Konversation::ServerSettings server;
        Konversation::ChannelSettings channel;

        for (it = groups.begin(); it != groups.end(); ++it)
        {
            KConfigGroup cgServerGroup(KSharedConfig::openConfig()->group(*it));
            Konversation::ServerGroupSettingsPtr serverGroup(new Konversation::ServerGroupSettings);
            serverGroup->setName(cgServerGroup.readEntry("Name"));
            serverGroup->setSortIndex(groups.at(index).section(' ', -1).toInt() );
            serverGroup->setIdentityId(Preferences::identityByName(cgServerGroup.readEntry("Identity"))->id());
            serverGroup->setConnectCommands(cgServerGroup.readEntry("ConnectCommands"));
            serverGroup->setAutoConnectEnabled(cgServerGroup.readEntry("AutoConnect", false));
            serverGroup->setNotificationsEnabled(cgServerGroup.readEntry("EnableNotifications", true));
            serverGroup->setExpanded(cgServerGroup.readEntry("Expanded", false));

            notifyList.insert((*serverGroup).id(), cgServerGroup.readEntry("NotifyList", QString()).split(QLatin1Char(' '), QString::SkipEmptyParts));

            tmp1 = cgServerGroup.readEntry("ServerList", QStringList());
            for (it2 = tmp1.begin(); it2 != tmp1.end(); ++it2)
            {
                KConfigGroup cgServer(KSharedConfig::openConfig()->group(*it2));
                server.setHost(cgServer.readEntry("Server"));
                server.setPort(cgServer.readEntry<int>("Port", 0));
                server.setPassword(cgServer.readEntry("Password"));
                server.setSSLEnabled(cgServer.readEntry("SSLEnabled", false));
                serverGroup->addServer(server);
            }

            //config->setGroup((*it));
            tmp1 = cgServerGroup.readEntry("AutoJoinChannels", QStringList());

            for (it2 = tmp1.begin(); it2 != tmp1.end(); ++it2)
            {
                KConfigGroup cgJoin(KSharedConfig::openConfig()->group(*it2));

                if (!cgJoin.readEntry("Name").isEmpty())
                {
                    channel.setName(cgJoin.readEntry("Name"));
                    channel.setPassword(cgJoin.readEntry("Password"));
                    serverGroup->addChannel(channel);
                }
            }

            //config->setGroup((*it));
            tmp1 = cgServerGroup.readEntry("ChannelHistory", QStringList());
            channelHistory.clear();

            for (it2 = tmp1.begin(); it2 != tmp1.end(); ++it2)
            {
                KConfigGroup cgChanHistory(KSharedConfig::openConfig()->group(*it2));

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

            index++;
        }

        Preferences::setServerGroupHash(serverGroups);
    }

    // Notify Settings and lists.  Must follow Server List.
    Preferences::setNotifyList(notifyList);
    Preferences::self()->setNotifyDelay(Preferences::self()->notifyDelay());
    Preferences::self()->setUseNotify(Preferences::self()->useNotify());

    // Quick Buttons List

    // if there are button definitions in the config file, remove default buttons
    if (KSharedConfig::openConfig()->hasGroup("Button List"))
        Preferences::clearQuickButtonList();

    KConfigGroup cgQuickButtons(KSharedConfig::openConfig()->group("Button List"));
    // Read all default buttons
    QStringList buttonList(Preferences::quickButtonList());
    // Read all quick buttons
    int index=0;
    while (cgQuickButtons.hasKey(QString(QStringLiteral("Button%1")).arg(index)))
    {
        buttonList.append(cgQuickButtons.readEntry(QString(QStringLiteral("Button%1")).arg(index++)));
    } // while
    // Put back the changed button list
    Preferences::setQuickButtonList(buttonList);

    // Autoreplace List

    // if there are autoreplace definitions in the config file, remove default entries
    if (KSharedConfig::openConfig()->hasGroup("Autoreplace List"))
        Preferences::clearAutoreplaceList();

    KConfigGroup cgAutoreplace(KSharedConfig::openConfig()->group("Autoreplace List"));
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
            entry=entry.left(length);
        QString regex = entry.section(QLatin1Char(','),0,0);
        QString direction = entry.section(QLatin1Char(','),1,1);
        QString pattern = entry.section(QLatin1Char(','),2,2);
        QString replace = entry.section(QLatin1Char(','),3);
        // add entry to internal list
        autoreplaceList.append(QStringList() << regex << direction << pattern << replace);
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
        if (replace.length()>0)
        {
            int repLen=replace.length()-1;
            if (replace.at(repLen)==QLatin1Char('#'))
                replace=replace.left(repLen);
        }
        if (pattern.length()>0)
        {
            int patLen=pattern.length()-1;
            if (pattern.at(patLen)==QLatin1Char('#'))
                pattern=pattern.left(patLen);
        }
        index++;
        indexString = QString::number(index);
        autoreplaceList.append(QStringList() << regex << direction << pattern << replace);
    }
    // Put back the changed autoreplace list
    Preferences::setAutoreplaceList(autoreplaceList);

    //TODO FIXME I assume this is in the <default> group, but I have a hunch we just don't care about <1.0.1
    // Highlight List
    KConfigGroup cgDefault(KSharedConfig::openConfig()->group("<default>"));
    if (cgDefault.hasKey("Highlight")) // Stay compatible with versions < 0.14
    {
        QString highlight=cgDefault.readEntry("Highlight");
        QStringList hiList = highlight.split(QLatin1Char(' '), QString::SkipEmptyParts);

        for (int hiIndex=0; hiIndex < hiList.count(); hiIndex+=2)
        {
            Preferences::addHighlight(hiList[hiIndex], false, QString(QLatin1Char('#')+hiList[hiIndex+1]), QString(), QString(), QString(), true);
        }

        cgDefault.deleteEntry("Highlight");
    }
    else
    {
        int i = 0;

        while (KSharedConfig::openConfig()->hasGroup(QString(QStringLiteral("Highlight%1")).arg(i)))
        {
            KConfigGroup cgHilight(KSharedConfig::openConfig()->group(QString(QStringLiteral("Highlight%1")).arg(i)));
            Preferences::addHighlight(
                cgHilight.readEntry("Pattern"),
                cgHilight.readEntry("RegExp", false),
                cgHilight.readEntry("Color", QColor(Qt::black)),
                cgHilight.readPathEntry("Sound", QString()),
                cgHilight.readEntry("AutoText"),
                cgHilight.readEntry("ChatWindows"),
                cgHilight.readEntry("Notify", true)
                );
            i++;
        }
    }

    // Ignore List
    KConfigGroup cgIgnoreList(KSharedConfig::openConfig()->group("Ignore List"));
    // Remove all default entries if there is at least one Ignore in the Preferences::file
    if (cgIgnoreList.hasKey("Ignore0"))
        Preferences::clearIgnoreList();
    // Read all ignores
    index=0;
    while (cgIgnoreList.hasKey(QString(QStringLiteral("Ignore%1")).arg(index)))
    {
        Preferences::addIgnore(cgIgnoreList.readEntry(QString(QStringLiteral("Ignore%1")).arg(index++)));
    }

    // Aliases
    KConfigGroup cgAliases(KSharedConfig::openConfig()->group("Aliases"));
    QStringList newList=cgAliases.readEntry("AliasList", QStringList());
    if (!newList.isEmpty())
        Preferences::self()->setAliasList(newList);

    // Channel Encodings

    //Legacy channel encodings read in Jun. 29, 2009
    KConfigGroup cgChannelEncodings(KSharedConfig::openConfig()->group("Channel Encodings"));
    QMap<QString,QString> channelEncodingEntries=cgChannelEncodings.entryMap();
    QRegExp re(QStringLiteral("^(.+) ([^\\s]+)$"));
    QList<QString> channelEncodingEntryKeys=channelEncodingEntries.keys();

    for(QList<QString>::const_iterator itStr=channelEncodingEntryKeys.constBegin(); itStr != channelEncodingEntryKeys.constEnd(); ++itStr)
    {
        if(re.indexIn(*itStr) > -1)
        {
                Preferences::setChannelEncoding(re.cap(1),re.cap(2),channelEncodingEntries[*itStr]);
        }
    }
    //End legacy channel encodings read in Jun 29, 2009

    KConfigGroup cgEncodings(KSharedConfig::openConfig()->group("Encodings"));
    QMap<QString,QString> encodingEntries=cgEncodings.entryMap();
    QList<QString> encodingEntryKeys=encodingEntries.keys();

    QRegExp reg(QStringLiteral("^([^\\s]+) ([^\\s]+)\\s?([^\\s]*)$"));
    for(QList<QString>::const_iterator itStr=encodingEntryKeys.constBegin(); itStr != encodingEntryKeys.constEnd(); ++itStr)
    {
        if(reg.indexIn(*itStr) > -1)
        {
            if(reg.cap(1) == QStringLiteral("ServerGroup") && !reg.cap(3).isEmpty())
                Preferences::setChannelEncoding(sgKeys.at(reg.cap(2).toInt()), reg.cap(3), encodingEntries[*itStr]);
            else
                Preferences::setChannelEncoding(reg.cap(1), reg.cap(2), encodingEntries[*itStr]);
        }
    }

    // Spell Checking Languages
    KConfigGroup cgSpellCheckingLanguages(KSharedConfig::openConfig()->group("Spell Checking Languages"));
    QMap<QString, QString> spellCheckingLanguageEntries=cgSpellCheckingLanguages.entryMap();
    QList<QString> spellCheckingLanguageEntryKeys=spellCheckingLanguageEntries.keys();

    for (QList<QString>::const_iterator itStr=spellCheckingLanguageEntryKeys.constBegin(); itStr != spellCheckingLanguageEntryKeys.constEnd(); ++itStr)
    {
        if (reg.indexIn(*itStr) > -1)
        {
            if (reg.cap(1) == QStringLiteral("ServerGroup") && !reg.cap(3).isEmpty())
            {
                ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(sgKeys.at(reg.cap(2).toInt()));

                if (serverGroup)
                    Preferences::setSpellCheckingLanguage(serverGroup, reg.cap(3), spellCheckingLanguageEntries[*itStr]);
            }
            else
                Preferences::setSpellCheckingLanguage(reg.cap(1), reg.cap(2), spellCheckingLanguageEntries[*itStr]);
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
    QStringList identities=KSharedConfig::openConfig()->groupList().filter(QRegExp(QStringLiteral("Identity [0-9]+")));
    if (identities.count())
    {
        // remove old identity list from Preferences::file to keep numbering under control
        for (int index=0; index < identities.count(); index++)
            KSharedConfig::openConfig()->deleteGroup(identities[index]);
    }

    IdentityList identityList = Preferences::identityList();
    int index = 0;

    for (IdentityList::ConstIterator it = identityList.constBegin(); it != identityList.constEnd(); ++it)
    {
        IdentityPtr identity = (*it);
        KConfigGroup cgIdentity(KSharedConfig::openConfig()->group(QString(QStringLiteral("Identity %1")).arg(index)));

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
    QStringList groups = KSharedConfig::openConfig()->groupList().filter(QRegExp(QStringLiteral("ServerGroup [0-9]+")));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KSharedConfig::openConfig()->deleteGroup((*it));
        }
    }

    // Remove the old servers from the config
    groups = KSharedConfig::openConfig()->groupList().filter(QRegExp(QStringLiteral("Server [0-9]+")));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KSharedConfig::openConfig()->deleteGroup((*it));
        }
    }

    // Remove the old channels from the config
    groups = KSharedConfig::openConfig()->groupList().filter(QRegExp(QStringLiteral("Channel [0-9]+")));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KSharedConfig::openConfig()->deleteGroup((*it));
        }
    }

    // Add the new servergroups to the config
    Konversation::ServerGroupHash serverGroupHash = Preferences::serverGroupHash();
    QHashIterator<int, Konversation::ServerGroupSettingsPtr> hashIt(serverGroupHash);

    QMap<int, Konversation::ServerGroupSettingsPtr> sortedServerGroupMap;

    // Make the indices in the group headers reflect the server list dialog sorting.
    while (hashIt.hasNext())
    {
        hashIt.next();

        sortedServerGroupMap.insert(hashIt.value()->sortIndex(), hashIt.value());
    }

    QMapIterator<int, Konversation::ServerGroupSettingsPtr> it(sortedServerGroupMap);

    index = 0;
    int index2 = 0;
    int index3 = 0;
    int width = 0;
    QList<int> keys = serverGroupHash.keys();
    for(int i=0; i<keys.count(); i++)
        if(width < keys.at(i)) width = keys.at(i);
    width = QString(width).length();
    QString groupName;
    QStringList servers;
    Konversation::ServerList::iterator it2;
    Konversation::ServerList serverlist;
    Konversation::ChannelList channelList;
    Konversation::ChannelList::iterator it3;
    QStringList channels;
    QStringList channelHistory;
    QList<int> sgKeys;

    while(it.hasNext())
    {
        it.next();
        serverlist = (it.value())->serverList();
        servers.clear();

        sgKeys.append(it.value()->id());

        for(it2 = serverlist.begin(); it2 != serverlist.end(); ++it2)
        {
            groupName = QString(QStringLiteral("Server %1")).arg(index2);
            servers.append(groupName);
            KConfigGroup cgServer(KSharedConfig::openConfig()->group(groupName));
            cgServer.writeEntry("Server", (*it2).host());
            cgServer.writeEntry("Port", (*it2).port());
            cgServer.writeEntry("Password", (*it2).password());
            cgServer.writeEntry("SSLEnabled", (*it2).SSLEnabled());
            index2++;
        }

        channelList = it.value()->channelList();
        channels.clear();

        for(it3 = channelList.begin(); it3 != channelList.end(); ++it3)
        {
            groupName = QString(QStringLiteral("Channel %1")).arg(index3);
            channels.append(groupName);
            KConfigGroup cgChannel(KSharedConfig::openConfig()->group(groupName));
            cgChannel.writeEntry("Name", (*it3).name());
            cgChannel.writeEntry("Password", (*it3).password());
            index3++;
        }

        channelList = it.value()->channelHistory();
        channelHistory.clear();

        for(it3 = channelList.begin(); it3 != channelList.end(); ++it3)
        {   // TODO FIXME: is it just me or is this broken?
            groupName = QString(QStringLiteral("Channel %1")).arg(index3);
            channelHistory.append(groupName);
            KConfigGroup cgChannelHistory(KSharedConfig::openConfig()->group(groupName));
            cgChannelHistory.writeEntry("Name", (*it3).name());
            cgChannelHistory.writeEntry("Password", (*it3).password());
            cgChannelHistory.writeEntry("EnableNotifications", (*it3).enableNotifications());
            index3++;
        }

        QString sgn = QString(QStringLiteral("ServerGroup %1")).arg(QString::number(index).rightJustified(width,QLatin1Char('0')));
        KConfigGroup cgServerGroup(KSharedConfig::openConfig()->group(sgn));
        cgServerGroup.writeEntry("Name", it.value()->name());
        cgServerGroup.writeEntry("Identity", it.value()->identity()->getName());
        cgServerGroup.writeEntry("ServerList", servers);
        cgServerGroup.writeEntry("AutoJoinChannels", channels);
        cgServerGroup.writeEntry("ConnectCommands", it.value()->connectCommands());
        cgServerGroup.writeEntry("AutoConnect", it.value()->autoConnectEnabled());
        cgServerGroup.writeEntry("ChannelHistory", channelHistory);
        cgServerGroup.writeEntry("EnableNotifications", it.value()->enableNotifications());
        cgServerGroup.writeEntry("Expanded", it.value()->expanded());
        cgServerGroup.writeEntry("NotifyList",Preferences::notifyStringByGroupId(it.value()->id()));
        index++;
    }

    KSharedConfig::openConfig()->deleteGroup("Server List");

    // Ignore List
    KSharedConfig::openConfig()->deleteGroup("Ignore List");
    KConfigGroup cgIgnoreList(KSharedConfig::openConfig()->group("Ignore List"));
    QList<Ignore*> ignoreList=Preferences::ignoreList();
    for (int i = 0; i < ignoreList.size(); ++i) {
        cgIgnoreList.writeEntry(QString(QStringLiteral("Ignore%1")).arg(i),QString(QStringLiteral("%1,%2")).arg(ignoreList.at(i)->getName()).arg(ignoreList.at(i)->getFlags()));
    }

    // Channel Encodings
    // remove all entries once
    KSharedConfig::openConfig()->deleteGroup("Channel Encodings"); // legacy Jun 29, 2009
    KSharedConfig::openConfig()->deleteGroup("Encodings");
    KConfigGroup cgEncoding(KSharedConfig::openConfig()->group("Encodings"));
    QList<int> encServers=Preferences::channelEncodingsServerGroupIdList();
    //i have no idea these would need to be sorted //encServers.sort();
    QList<int>::iterator encServer;
    for ( encServer = encServers.begin(); encServer != encServers.end(); ++encServer )
    {
        Konversation::ServerGroupSettingsPtr sgsp = Preferences::serverGroupById(*encServer);

        if ( sgsp )  // sgsp == 0 when the entry is of QuickConnect or something?
        {
            QStringList encChannels=Preferences::channelEncodingsChannelList(*encServer);
            //ditto //encChannels.sort();
            QStringList::iterator encChannel;
            for ( encChannel = encChannels.begin(); encChannel != encChannels.end(); ++encChannel )
            {
                QString enc = Preferences::channelEncoding(*encServer, *encChannel);
                QString key = QLatin1Char(' ') + (*encChannel);
                if(sgKeys.contains(*encServer))
                    key.prepend(QStringLiteral("ServerGroup ")+QString::number(sgKeys.indexOf(*encServer)));
                else
                    key.prepend(sgsp->name());
                cgEncoding.writeEntry(key, enc);
            }
        }
    }

    // Spell Checking Languages
    KSharedConfig::openConfig()->deleteGroup("Spell Checking Languages");
    KConfigGroup cgSpellCheckingLanguages(KSharedConfig::openConfig()->group("Spell Checking Languages"));

    QHashIterator<Konversation::ServerGroupSettingsPtr, QHash<QString, QString> > i(Preferences::serverGroupSpellCheckingLanguages());

    while (i.hasNext())
    {
        i.next();

        QHashIterator<QString, QString> i2(i.value());

        while (i2.hasNext())
        {
            i2.next();

            int serverGroupIndex = sgKeys.indexOf(i.key()->id());

            if (serverGroupIndex != -1)
                cgSpellCheckingLanguages.writeEntry(QStringLiteral("ServerGroup ") + QString::number(serverGroupIndex) + QLatin1Char(' ') + i2.key(), i2.value());
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
        emit appearanceChanged();
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
        QList<int> r;
        r.append(staticrates[i].m_rate);
        r.append(staticrates[i].m_interval / 1000);
        r.append(int(staticrates[i].m_type));
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

    url = url.replace(QStringLiteral("&amp;"), QStringLiteral("&"));

    QList<QStandardItem*> existing = m_urlModel->findItems(url, Qt::MatchExactly, 1);

    foreach(QStandardItem* item, existing)
    {
        if (m_urlModel->item(item->row(), 0)->data(Qt::DisplayRole).toString() == origin)
            m_urlModel->removeRow(item->row());
    }

    m_urlModel->insertRow(0);
    m_urlModel->setData(m_urlModel->index(0, 0), origin, Qt::DisplayRole);
    m_urlModel->setData(m_urlModel->index(0, 1), url, Qt::DisplayRole);

    UrlDateItem* dateItem = new UrlDateItem(dateTime);
    m_urlModel->setItem(0, 2, dateItem);
}

void Application::openQuickConnectDialog()
{
    quickConnectDialog = new QuickConnectDialog(mainWindow);
    connect(quickConnectDialog, SIGNAL(connectClicked(Konversation::ConnectionFlag,QString,QString,QString,QString,QString,bool)),
        m_connectionManager, SLOT(connectTo(Konversation::ConnectionFlag,QString,QString,QString,QString,QString,bool)));
    quickConnectDialog->show();
}

void Application::sendMultiServerCommand(const QString& command, const QString& parameter)
{
    const QList<Server*> serverList = getConnectionManager()->getServerList();

    foreach (Server* server, serverList)
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
    foreach (Server* lookServer, serverList)
    {
        if (lserverOrGroup.isEmpty()
            || lookServer->getServerName().toLower()==lserverOrGroup
            || lookServer->getDisplayName().toLower()==lserverOrGroup)
        {
            nickInfo = lookServer->getNickInfo(ircnick);
            if (nickInfo)
                return nickInfo;         //If we found one
        }
    }
    return (NickInfoPtr)0;
}

// auto replace on input/output
QPair<QString, int> Application::doAutoreplace(const QString& text, bool output, int cursorPos)
{
    // get autoreplace list
    QList<QStringList> autoreplaceList=Preferences::autoreplaceList();
    // working copy
    QString line=text;

    // loop through the list of replacement patterns
    for (int index=0;index<autoreplaceList.count();index++)
    {
        // get autoreplace definition
        QStringList definition=autoreplaceList[index];
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
            if (regex==QStringLiteral("1"))
            {
                // create regex from pattern
                QRegExp needleReg(pattern);
                // set pattern case insensitive
                needleReg.setCaseSensitivity(Qt::CaseSensitive);
                int index = 0;
                int newIndex = index;

                do {
                    // find matches
                    index = line.indexOf(needleReg, index);

                    if (index != -1)
                    {
                        // remember captured patterns
                        QStringList captures = needleReg.capturedTexts();
                        QString replaceWith = replacement;

                        replaceWith.replace(QStringLiteral("%%"),QStringLiteral("%\x01")); // escape double %
                        // replace %0-9 in regex groups
                        for (int capture=0;capture<captures.count();capture++)
                        {
                            QString search = QString(QStringLiteral("%%1")).arg(capture);
                            replaceWith.replace(search, captures[capture]);
                        }
                        //Explanation why this is important so we don't forget:
                        //If somebody has a regex that say has a replacement of url.com/%1/%2 and the
                        //regex can either match one or two patterns, if the 2nd pattern match is left,
                        //the url is invalid (url.com/match/%2). This is expected regex behavior I'd assume.
                        replaceWith.remove(QRegExp(QStringLiteral("%[0-9]")));

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
                QRegExp needleReg(pattern);
                needleReg.setPatternSyntax(QRegExp::FixedString);
                int index=line.indexOf(needleReg);
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

                    index=line.indexOf(needleReg,nextLength);
                }
            }
        }
    }

    return QPair<QString, int>(line, cursorPos);
}

void Application::doInlineAutoreplace(KTextEdit* textEdit)
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
    if (!Preferences::self()->useCustomBrowser() || url.startsWith(QLatin1String("mailto:")) || url.startsWith(QLatin1String("amarok:")))
    {
        if (url.startsWith(QLatin1String("irc://")) || url.startsWith(QLatin1String("ircs://")))
            Application::instance()->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, url);
        else if (url.startsWith(QLatin1String("mailto:")))
            QDesktopServices::openUrl(QUrl(url));
        else
#ifndef Q_OS_WIN
            new KRun(QUrl(url), Application::instance()->getMainWindow());
#else
            QDesktopServices::openUrl(QUrl::fromUserInput(url));
#endif
    }
    else
    {
        QHash<QChar,QString> map;
        map.insert(QLatin1Char('u'), url);
        const QString cmd = KMacroExpander::expandMacrosShellQuote(Preferences::self()->webBrowserCmd(), map);
        const QStringList args = KShell::splitArgs(cmd);

        if (!args.isEmpty())
        {
            KProcess::startDetached(args);
            return;
        }
    }
}

Konversation::Sound* Application::sound()
{
    if (!m_sound)
        m_sound = new Konversation::Sound;

    return m_sound;
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
        proxy.setPort(Preferences::self()->proxyPort());
        proxy.setUser(Preferences::self()->proxyUsername());
        QString password;

        if(wallet())
        {
            int ret = wallet()->readPassword(QStringLiteral("ProxyPassword"), password);

            if(ret != 0)
            {
                qCritical() << "Failed to read the proxy password from the wallet, error code:" << ret;
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
            return NULL;

        connect(m_wallet, &KWallet::Wallet::walletClosed, this, &Application::closeWallet);

        if(!m_wallet->hasFolder(QStringLiteral("Konversation")))
        {
            if(!m_wallet->createFolder(QStringLiteral("Konversation")))
            {
                qCritical() << "Failed to create folder Konversation in the network wallet.";
                closeWallet();
                return NULL;
            }
        }

        if(!m_wallet->setFolder(QStringLiteral("Konversation")))
        {
            qCritical() << "Failed to set active folder to Konversation in the network wallet.";
            closeWallet();
            return NULL;
        }
    }

    return m_wallet;
}

void Application::closeWallet()
{
    delete m_wallet;
    m_wallet = NULL;
}



// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
