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
#include "linkaddressbook/addressbook.h"
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

#include <KRun>
#include <KCmdLineArgs>
#include <KConfig>
#include <KShell>
#include <KToolInvocation>
#include <KCharMacroExpander>
#include <kwallet.h>
#include <solid/networking.h>


using namespace Konversation;

Application::Application()
: KUniqueApplication(true, true)
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
}

Application::~Application()
{
    kDebug();

    if (!m_images)
        return; // Nothing to do, newInstance() has never been called.

    stashQueueRates();
    Preferences::self()->writeConfig(); // FIXME i can't figure out why this isn't in saveOptions --argonel
    saveOptions(false);

    // Delete m_dccTransferManager here as its destructor depends on the main loop being in tact which it
    // won't be if if we wait till Qt starts deleting parent pointers.
    delete m_dccTransferManager;

    delete m_images;
    //delete dbusObject;
    //delete prefsDCOP;
    //delete identDBus;
    delete osd;
    osd = 0;
    closeWallet();

    if (m_restartScheduled) implementRestart();
}

void Application::implementRestart()
{
    QStringList argumentList;

#if KDE_IS_VERSION(4,5,61)
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    argumentList = args->allArguments();

    // Pop off the executable name. May not be the first argument in argv
    // everywhere, so verify first.
    if (QCoreApplication::applicationFilePath().endsWith(argumentList.first()))
        argumentList.removeFirst();

    // Don't round-trip --restart.
    argumentList.removeAll("--restart");

    // Avoid accumulating multiple --startupdelay arguments across multiple
    // uses of restart().
    if (argumentList.contains("--startupdelay"))
    {
        int index = argumentList.lastIndexOf("--startupdelay");

        if (index < argumentList.count() - 1 && !argumentList.at(index + 1).startsWith('-'))
        {
            QString delayArgument = argumentList.at(index + 1);

            bool ok;

            uint delay = delayArgument.toUInt(&ok, 10);

            // If the argument is invalid or too low, raise to at least 2000 msecs.
            if (!ok || delay < 2000)
                argumentList.replace(index + 1, "2000");
        }
    }
    else
#endif
        argumentList << "--startupdelay" << "2000";

    KProcess::startDetached(QCoreApplication::applicationFilePath(), argumentList);
}

int Application::newInstance()
{
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    QString url;
    if (args->count() > 0)
        url = args->arg(0);

    if (!mainWindow)
    {
        connect(this, SIGNAL(aboutToQuit()), this, SLOT(prepareShutdown()));

        m_connectionManager = new ConnectionManager(this);

        m_awayManager = new AwayManager(this);

        connect(m_connectionManager, SIGNAL(identityOnline(int)), m_awayManager, SLOT(identityOnline(int)));
        connect(m_connectionManager, SIGNAL(identityOffline(int)), m_awayManager, SLOT(identityOffline(int)));
        connect(m_connectionManager, SIGNAL(connectionChangedAwayState(bool)), m_awayManager, SLOT(updateGlobalAwayAction(bool)));

        connect(Solid::Networking::notifier(), SIGNAL(shouldDisconnect()), m_connectionManager, SLOT(involuntaryQuitServers()));
        connect(Solid::Networking::notifier(), SIGNAL(shouldConnect()), m_connectionManager, SLOT(reconnectInvoluntary()));

        m_scriptLauncher = new ScriptLauncher(this);

        // an instance of DccTransferManager needs to be created before GUI class instances' creation.
        m_dccTransferManager = new DCC::TransferManager(this);

        // make sure all vars are initialized properly
        quickConnectDialog = 0;

        // Sound object used to play sound is created when needed.
        m_sound = NULL;

        // initialize OSD display here, so we can read the Preferences::properly
        osd = new OSDWidget( "Konversation" );

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

        // Setup system codec
        // TODO: check if this works now as intended
        //    QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());

        connect(KGlobalSettings::self(), SIGNAL(appearanceChanged()), this, SIGNAL(appearanceChanged()));

        // open main window
        mainWindow = new MainWindow();
        //setMainWidget(mainWindow); //TODO FIXME do we need any of the other semantics this use to gain us?

        connect(mainWindow, SIGNAL(showQuickConnectDialog()), this, SLOT(openQuickConnectDialog()) );
        connect(Preferences::self(), SIGNAL(updateTrayIcon()), mainWindow, SLOT(updateTrayIcon()) );
        connect(mainWindow, SIGNAL(endNotification()), osd, SLOT(hide()) );
        // take care of user style changes, setting back colors and stuff

        // apply GUI settings
        emit appearanceChanged(); //TODO FIXME i do believe this signal is abused

        if (Preferences::self()->showTrayIcon() && Preferences::self()->hideToTrayOnStartup())
            mainWindow->hide();
        else
            mainWindow->show();

        bool openServerList = Preferences::self()->showServerList();

        // handle autoconnect on startup
        Konversation::ServerGroupHash serverGroups = Preferences::serverGroupHash();

        if (args->isSet("autoconnect") && url.isEmpty() && !args->isSet("server"))
        {
            QHashIterator<int, Konversation::ServerGroupSettingsPtr> it(serverGroups);
            while(it.hasNext())
            {
                it.next();
                if (it.value()->autoConnectEnabled())
                {
                    openServerList = false;
                    m_connectionManager->connectTo(Konversation::CreateNewConnection, it.key());
                }
            }
        }

        if (openServerList) mainWindow->openServerList();

        connect(this, SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)), this, SLOT(saveOptions()));

        // prepare dbus interface
        dbusObject = new Konversation::DBus(this);
        QDBusConnection::sessionBus().registerObject("/irc", dbusObject, QDBusConnection::ExportNonScriptableSlots);
        identDBus = new Konversation::IdentDBus(this);
        QDBusConnection::sessionBus().registerObject("/identity", identDBus, QDBusConnection::ExportNonScriptableSlots);
        QDBusConnection::sessionBus().registerObject("/KIMIface", Konversation::Addressbook::self(), QDBusConnection::ExportNonScriptableSlots | QDBusConnection::ExportNonScriptableSignals);

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

        connect(this, SIGNAL(appearanceChanged()), this, SLOT(updateProxySettings()));
    }
    else if (args->isSet("restart"))
    {
        restart();

        return KUniqueApplication::newInstance();
    }

    if (!url.isEmpty())
        getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, url);
    else if (args->isSet("server"))
    {
        getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection,
                                          args->getOption("server"),
                                          args->getOption("port"),
                                          args->getOption("password"),
                                          args->getOption("nick"),
                                          args->getOption("channel"),
                                          args->isSet("ssl"));
    }

    return KUniqueApplication::newInstance();
}

Application* Application::instance()
{
    return static_cast<Application*>(KApplication::kApplication());
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

void Application::showQueueTuner(bool p)
{
    getMainWindow()->getViewContainer()->showQueueTuner(p);
}

void Application::dbusMultiServerRaw(const QString &command)
{
    sendMultiServerCommand(command.section(' ', 0,0), command.section(' ', 1));
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
    KConfigGroup cgSortNicknames(KGlobal::config()->group("Sort Nicknames"));

    QString sortOrder=cgSortNicknames.readEntry("SortOrder");
    QStringList sortOrderList=sortOrder.split("");
    sortOrderList.sort();
    if (sortOrderList.join("")!="-hopqv")
    {
        sortOrder=Preferences::defaultNicknameSortingOrder();
        Preferences::self()->setSortOrder(sortOrder);
    }

    // Identity list
    QStringList identityList=KGlobal::config()->groupList().filter(QRegExp("Identity [0-9]+"));
    if (!identityList.isEmpty())
    {
        Preferences::clearIdentityList();

        for(int index=0;index<identityList.count();index++)
        {
            IdentityPtr newIdentity(new Identity());
            KConfigGroup cgIdentity(KGlobal::config()->group(identityList[index]));

            newIdentity->setName(cgIdentity.readEntry("Name"));

            newIdentity->setIdent(cgIdentity.readEntry("Ident"));
            newIdentity->setRealName(cgIdentity.readEntry("Realname"));

            newIdentity->setNicknameList(cgIdentity.readEntry<QStringList>("Nicknames",QStringList()));

            newIdentity->setBot(cgIdentity.readEntry("Bot"));
            newIdentity->setPassword(cgIdentity.readEntry("Password"));

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
    KConfigGroup cgServerList(KGlobal::config()->group("Server List"));

    // Read the new server settings
    QStringList groups = KGlobal::config()->groupList().filter(QRegExp("ServerGroup [0-9]+"));
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
            KConfigGroup cgServerGroup(KGlobal::config()->group(*it));
            Konversation::ServerGroupSettingsPtr serverGroup(new Konversation::ServerGroupSettings);
            serverGroup->setName(cgServerGroup.readEntry("Name"));
            serverGroup->setSortIndex(index);
            serverGroup->setIdentityId(Preferences::identityByName(cgServerGroup.readEntry("Identity"))->id());
            serverGroup->setConnectCommands(cgServerGroup.readEntry("ConnectCommands"));
            serverGroup->setAutoConnectEnabled(cgServerGroup.readEntry("AutoConnect", false));
            serverGroup->setNotificationsEnabled(cgServerGroup.readEntry("EnableNotifications", true));
            serverGroup->setExpanded(cgServerGroup.readEntry("Expanded", false));

            notifyList.insert((*serverGroup).id(), cgServerGroup.readEntry("NotifyList", QString()).split(' ', QString::SkipEmptyParts));

            tmp1 = cgServerGroup.readEntry("ServerList", QStringList());
            for (it2 = tmp1.begin(); it2 != tmp1.end(); ++it2)
            {
                KConfigGroup cgServer(KGlobal::config()->group(*it2));
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
                KConfigGroup cgJoin(KGlobal::config()->group(*it2));

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
                KConfigGroup cgChanHistory(KGlobal::config()->group(*it2));

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
    if (KGlobal::config()->hasGroup("Button List"))
        Preferences::clearQuickButtonList();

    KConfigGroup cgQuickButtons(KGlobal::config()->group("Button List"));
    // Read all default buttons
    QStringList buttonList(Preferences::quickButtonList());
    // Read all quick buttons
    int index=0;
    while (cgQuickButtons.hasKey(QString("Button%1").arg(index)))
    {
        buttonList.append(cgQuickButtons.readEntry(QString("Button%1").arg(index++)));
    } // while
    // Put back the changed button list
    Preferences::setQuickButtonList(buttonList);

    // Autoreplace List

    // if there are autoreplace definitions in the config file, remove default entries
    if (KGlobal::config()->hasGroup("Autoreplace List"))
        Preferences::clearAutoreplaceList();

    KConfigGroup cgAutoreplace(KGlobal::config()->group("Autoreplace List"));
    // Read all default entries
    QList<QStringList> autoreplaceList(Preferences::autoreplaceList());
    // Read all entries
    index=0;
    // legacy code for old autoreplace format 4/6/09
    QString autoReplaceString("Autoreplace");
    while (cgAutoreplace.hasKey(autoReplaceString + QString::number(index)))
    {
  // read entry and get length of the string
        QString entry=cgAutoreplace.readEntry(autoReplaceString + QString::number(index++));
        int length=entry.length()-1;
        // if there's a "#" in the end, strip it (used to preserve blanks at the end of the replacement text)
        // there should always be one, but older versions did not do it, so we check first
        if (entry.at(length)=='#')
            entry=entry.left(length);
        QString regex = entry.section(',',0,0);
        QString direction = entry.section(',',1,1);
        QString pattern = entry.section(',',2,2);
        QString replace = entry.section(',',3);
        // add entry to internal list
        autoreplaceList.append(QStringList() << regex << direction << pattern << replace);
    } // while
    //end legacy code for old autoreplace format
    index=0; //new code for autoreplace config
    QString indexString(QString::number(index));
    QString regexString("Regex");
    QString directString("Direction");
    QString patternString("Pattern");
    QString replaceString("Replace");
    while (cgAutoreplace.hasKey(patternString + indexString))
    {
        QString pattern = cgAutoreplace.readEntry(patternString + indexString);
        QString regex = cgAutoreplace.readEntry(regexString + indexString, QString("0"));
        QString direction = cgAutoreplace.readEntry(directString + indexString, QString("o"));
        QString replace = cgAutoreplace.readEntry(replaceString + indexString, QString());
        if (replace.length()>0)
        {
            int repLen=replace.length()-1;
            if (replace.at(repLen)=='#')
                replace=replace.left(repLen);
        }
        if (pattern.length()>0)
        {
            int patLen=pattern.length()-1;
            if (pattern.at(patLen)=='#')
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
    KConfigGroup cgDefault(KGlobal::config()->group("<default>"));
    if (cgDefault.hasKey("Highlight")) // Stay compatible with versions < 0.14
    {
        QString highlight=cgDefault.readEntry("Highlight");
        QStringList hiList = highlight.split(' ', QString::SkipEmptyParts);

        for (int hiIndex=0; hiIndex < hiList.count(); hiIndex+=2)
        {
            Preferences::addHighlight(hiList[hiIndex], false, QString('#'+hiList[hiIndex+1]), QString(), QString(), QString());
        }

        cgDefault.deleteEntry("Highlight");
    }
    else
    {
        int i = 0;

        while (KGlobal::config()->hasGroup(QString("Highlight%1").arg(i)))
        {
            KConfigGroup cgHilight(KGlobal::config()->group(QString("Highlight%1").arg(i)));
            Preferences::addHighlight(
                cgHilight.readEntry("Pattern"),
                cgHilight.readEntry("RegExp", false),
                cgHilight.readEntry("Color", QColor(Qt::black)),
                cgHilight.readPathEntry("Sound", QString()),
                cgHilight.readEntry("AutoText"),
                cgHilight.readEntry("ChatWindows")
                );
            i++;
        }
    }

    // Ignore List
    KConfigGroup cgIgnoreList(KGlobal::config()->group("Ignore List"));
    // Remove all default entries if there is at least one Ignore in the Preferences::file
    if (cgIgnoreList.hasKey("Ignore0"))
        Preferences::clearIgnoreList();
    // Read all ignores
    index=0;
    while (cgIgnoreList.hasKey(QString("Ignore%1").arg(index)))
    {
        Preferences::addIgnore(cgIgnoreList.readEntry(QString("Ignore%1").arg(index++)));
    }

    // Aliases
    KConfigGroup cgAliases(KGlobal::config()->group("Aliases"));
    QStringList newList=cgAliases.readEntry("AliasList", QStringList());
    if (!newList.isEmpty())
        Preferences::self()->setAliasList(newList);

    // Channel Encodings

    //Legacy channel encodings read in Jun. 29, 2009
    KConfigGroup cgChannelEncodings(KGlobal::config()->group("Channel Encodings"));
    QMap<QString,QString> channelEncodingEntries=cgChannelEncodings.entryMap();
    QRegExp re("^(.+) ([^\\s]+)$");
    QList<QString> channelEncodingEntryKeys=channelEncodingEntries.keys();

    for(QList<QString>::const_iterator itStr=channelEncodingEntryKeys.constBegin(); itStr != channelEncodingEntryKeys.constEnd(); ++itStr)
    {
        if(re.indexIn(*itStr) > -1)
        {
                Preferences::setChannelEncoding(re.cap(1),re.cap(2),channelEncodingEntries[*itStr]);
        }
    }
    //End legacy channel encodings read in Jun 29, 2009

    KConfigGroup cgEncodings(KGlobal::config()->group("Encodings"));
    QMap<QString,QString> encodingEntries=cgEncodings.entryMap();
    QList<QString> encodingEntryKeys=encodingEntries.keys();

    QRegExp reg("^(.+) ([^\\s]+) ([^\\s]+)$");
    for(QList<QString>::const_iterator itStr=encodingEntryKeys.constBegin(); itStr != encodingEntryKeys.constEnd(); ++itStr)
    {
        if(reg.indexIn(*itStr) > -1)
        {
            if(reg.cap(1) == "ServerGroup" && reg.numCaptures() == 3)
                Preferences::setChannelEncoding(sgKeys.at(reg.cap(2).toInt()), reg.cap(3), encodingEntries[*itStr]);
            else
                Preferences::setChannelEncoding(reg.cap(1), reg.cap(2), encodingEntries[*itStr]);
        }
    }

    fetchQueueRates();

    updateProxySettings();
}

void Application::saveOptions(bool updateGUI)
{
    // template:    KConfigGroup  (KGlobal::config()->group( ));

    //KConfig* config=KGlobal::config();

//    Should be handled in NicklistBehaviorConfigController now
//    config->setGroup("Sort Nicknames");

    // Clean up identity list
    QStringList identities=KGlobal::config()->groupList().filter(QRegExp("Identity [0-9]+"));
    if (identities.count())
    {
        // remove old identity list from Preferences::file to keep numbering under control
        for (int index=0; index < identities.count(); index++)
            KGlobal::config()->deleteGroup(identities[index]);
    }

    IdentityList identityList = Preferences::identityList();
    int index = 0;

    for (IdentityList::ConstIterator it = identityList.constBegin(); it != identityList.constEnd(); ++it)
    {
        IdentityPtr identity = (*it);
        KConfigGroup cgIdentity(KGlobal::config()->group(QString("Identity %1").arg(index)));

        cgIdentity.writeEntry("Name",identity->getName());
        cgIdentity.writeEntry("Ident",identity->getIdent());
        cgIdentity.writeEntry("Realname",identity->getRealName());
        cgIdentity.writeEntry("Nicknames",identity->getNicknameList());
        cgIdentity.writeEntry("Bot",identity->getBot());
        cgIdentity.writeEntry("Password",identity->getPassword());
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
    QStringList groups = KGlobal::config()->groupList().filter(QRegExp("ServerGroup [0-9]+"));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KGlobal::config()->deleteGroup((*it));
        }
    }

    // Remove the old servers from the config
    groups = KGlobal::config()->groupList().filter(QRegExp("Server [0-9]+"));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KGlobal::config()->deleteGroup((*it));
        }
    }

    // Remove the old channels from the config
    groups = KGlobal::config()->groupList().filter(QRegExp("Channel [0-9]+"));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KGlobal::config()->deleteGroup((*it));
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

        sgKeys.append(it.key());

        for(it2 = serverlist.begin(); it2 != serverlist.end(); ++it2)
        {
            groupName = QString("Server %1").arg(index2);
            servers.append(groupName);
            KConfigGroup cgServer(KGlobal::config()->group(groupName));
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
            groupName = QString("Channel %1").arg(index3);
            channels.append(groupName);
            KConfigGroup cgChannel(KGlobal::config()->group(groupName));
            cgChannel.writeEntry("Name", (*it3).name());
            cgChannel.writeEntry("Password", (*it3).password());
            index3++;
        }

        channelList = it.value()->channelHistory();
        channelHistory.clear();

        for(it3 = channelList.begin(); it3 != channelList.end(); ++it3)
        {   // TODO FIXME: is it just me or is this broken?
            groupName = QString("Channel %1").arg(index3);
            channelHistory.append(groupName);
            KConfigGroup cgChannelHistory(KGlobal::config()->group(groupName));
            cgChannelHistory.writeEntry("Name", (*it3).name());
            cgChannelHistory.writeEntry("Password", (*it3).password());
            cgChannelHistory.writeEntry("EnableNotifications", (*it3).enableNotifications());
            index3++;
        }

        QString sgn = QString("ServerGroup %1").arg(QString::number(index).rightJustified(width,'0'));
        KConfigGroup cgServerGroup(KGlobal::config()->group(sgn));
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

    KGlobal::config()->deleteGroup("Server List");

    // Ignore List
    KGlobal::config()->deleteGroup("Ignore List");
    KConfigGroup cgIgnoreList(KGlobal::config()->group("Ignore List"));
    QList<Ignore*> ignoreList=Preferences::ignoreList();
    for (int i = 0; i < ignoreList.size(); ++i) {
        cgIgnoreList.writeEntry(QString("Ignore%1").arg(i),QString("%1,%2").arg(ignoreList.at(i)->getName()).arg(ignoreList.at(i)->getFlags()));
    }

    // Channel Encodings
    // remove all entries once
    KGlobal::config()->deleteGroup("Channel Encodings"); // legacy Jun 29, 2009
    KGlobal::config()->deleteGroup("Encodings");
    KConfigGroup cgEncoding(KGlobal::config()->group("Encodings"));
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
                QString key = ' ' + (*encChannel);
                if(sgKeys.contains(*encServer))
                    key.prepend("ServerGroup "+QString::number(sgKeys.indexOf(*encServer)));
                else
                    key.prepend(sgsp->name());
                cgEncoding.writeEntry(key, enc);
            }
        }
    }

    KGlobal::config()->sync();

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

    url = url.replace("&amp;", "&");

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
    connect(quickConnectDialog, SIGNAL(connectClicked(Konversation::ConnectionFlag, QString, QString, QString, QString, QString, bool)),
        m_connectionManager, SLOT(connectTo(Konversation::ConnectionFlag, QString, QString, QString, QString, QString, bool)));
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
    nickServer.replace(QChar(0xE120), "@");
    ircnick = nickServer.section('@',0,0);
    serverOrGroup = nickServer.section('@',1);
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
QString Application::doAutoreplace(const QString& text,bool output)
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

        QString isDirection=output ? "o" : "i";

        // only replace if this pattern is for the specific direction or both directions
        if (direction==isDirection || direction=="io")
        {
            // regular expression pattern?
            if (regex=="1")
            {
                // create regex from pattern
                QRegExp needleReg(pattern);
                // set pattern case insensitive
                needleReg.setCaseSensitivity(Qt::CaseSensitive);
                int index = 0;

                do {
                    // find matches
                    index = line.indexOf(needleReg, index);

                    if (index != -1)
                    {
                        // remember captured patterns
                        QStringList captures = needleReg.capturedTexts();
                        QString replaceWith = replacement;

                        replaceWith.replace("%%","%\x01"); // escape double %
                        // replace %0-9 in regex groups
                        for (int capture=0;capture<captures.count();capture++)
                        {
                            QString search = QString("%%1").arg(capture);
                            replaceWith.replace(search, captures[capture]);
                        }
                        //Explanation why this is important so we don't forget:
                        //If somebody has a regex that say has a replacement of url.com/%1/%2 and the
                        //regex can either match one or two patterns, if the 2nd pattern match is left,
                        //the url is invalid (url.com/match/%2). This is expected regex behavior I'd assume.
                        replaceWith.remove(QRegExp("%[0-9]"));

                        replaceWith.replace("%\x01","%"); // return escaped % to normal
                        // allow for var expansion in autoreplace
                        replaceWith = Konversation::doVarExpansion(replaceWith);
                        // replace input with replacement
                        line.replace(index, captures[0].length(), replaceWith);
                        index += replaceWith.length();
                    }
                } while (index >= 0 && index < (int)line.length());
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
                    index=line.indexOf(needleReg,nextLength);
                }
            }
        }
    }

  return line;
}

void Application::openUrl(const QString& url)
{
    if (!Preferences::self()->useCustomBrowser() || url.startsWith(QLatin1String("mailto:")) || url.startsWith(QLatin1String("amarok:")))
    {
        if (url.startsWith(QLatin1String("irc://")) || url.startsWith(QLatin1String("ircs://")))
            Application::instance()->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, url);
        else if (url.startsWith(QLatin1String("mailto:")))
            KToolInvocation::invokeMailer(KUrl(url));
        else if (url.startsWith(QLatin1String("amarok:")))
            new KRun(KUrl(url), Application::instance()->getMainWindow());
        else
            KToolInvocation::invokeBrowser(url);
    }
    else
    {
        QHash<QChar,QString> map;
        map.insert('u', url);
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
        m_sound = new Konversation::Sound(this);

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
            int ret = wallet()->readPassword("ProxyPassword", password);

            if(ret != 0)
            {
                kError() << "Failed to read the proxy password from the wallet, error code:" << ret;
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

        connect(m_wallet, SIGNAL(walletClosed()), this, SLOT(closeWallet()));

        if(!m_wallet->hasFolder("Konversation"))
        {
            if(!m_wallet->createFolder("Konversation"))
            {
                kError() << "Failed to create folder Konversation in the network wallet.";
                closeWallet();
                return NULL;
            }
        }

        if(!m_wallet->setFolder("Konversation"))
        {
            kError() << "Failed to set active folder to Konversation in the network wallet.";
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

#include "application.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
