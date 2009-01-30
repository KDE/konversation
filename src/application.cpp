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

#include "application.h" ////// header renamed
#include "mainwindow.h" ////// header renamed
#include "connectionmanager.h"
#include "awaymanager.h"
#include "transfermanager.h" ////// header renamed
#include "viewcontainer.h"
#include "highlight.h"
#include "server.h"
#include "sound.h" ////// header renamed
#include "quickconnectdialog.h"
//Added by qt3to4:
#include <Q3CString>
#include <Q3PtrList>
#include "servergroupsettings.h"
#include "serversettings.h"
#include "channel.h"
#include "images.h"
#include "notificationhandler.h"
#include "commit.h"
#include "version.h"

#include <qtextcodec.h>
#include <qregexp.h>
#include <qfileinfo.h>

#include <kdebug.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
//#include <dcopclient.h>
#include <kdeversion.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kglobal.h>


KonversationApplication::KonversationApplication()
: KUniqueApplication(true, true)
{
    mainWindow = 0;
    m_connectionManager = 0;
    m_awayManager = 0;
    quickConnectDialog = 0;
    osd = 0;
}

KonversationApplication::~KonversationApplication()
{
    kDebug() << k_funcinfo << endl;
    Server::_stashRates();
    Preferences::self()->writeConfig();
    saveOptions(false);

    delete m_images;
    //delete dcopObject;
    //delete prefsDCOP;
    //delete identDCOP;
    delete osd;
    osd = 0;
}

int KonversationApplication::newInstance()
{
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    QString url; //TODO FIXME: does this really have to be a QCString?
    if (args->count() > 0)
        url = args->arg(0);

    if (!mainWindow)
    {
        connect(this, SIGNAL(aboutToQuit()), this, SLOT(prepareShutdown()));

        m_connectionManager = new ConnectionManager(this);

        m_awayManager = new AwayManager(this);

        connect(m_connectionManager, SIGNAL(identityOnline(int)), m_awayManager, SLOT(identityOnline(int)));
        connect(m_connectionManager, SIGNAL(identityOffline(int)), m_awayManager, SLOT(identityOffline(int)));
        connect(m_connectionManager, SIGNAL(identityOffline(int)), m_awayManager, SLOT(identityOffline(int)));
        connect(m_connectionManager, SIGNAL(connectionChangedAwayState(bool)), m_awayManager, SLOT(updateGlobalAwayAction(bool)));

        // an instance of DccTransferManager needs to be created before GUI class instances' creation.
        m_dccTransferManager = 0;//new DccTransferManager(this);

        // make sure all vars are initialized properly
        quickConnectDialog = 0;

        // Sound object used to play sound...
        m_sound = new Konversation::Sound(this);

        // initialize OSD display here, so we can read the Preferences::properly
        //osd = new OSDWidget( "Konversation" );

        Preferences::self();
        readOptions();

        // Images object providing LEDs, NickIcons
        m_images = new Images();
        connect(KGlobalSettings::self(), SIGNAL(iconChanged(int)), m_images, SLOT(updateIcons()));

        // Auto-alias scripts.  This adds any missing aliases
        QStringList aliasList(Preferences::aliasList());
        QStringList scripts(Preferences::defaultAliasList());
        bool changed = false;
        for ( QStringList::ConstIterator it = scripts.begin(); it != scripts.end(); ++it )
        {
            if(!aliasList.contains(*it)) {
                changed = true;
                aliasList.append(*it);
            }
        }
        if(changed)
            Preferences::setAliasList(aliasList);

        // Setup system codec
        // TODO: check if this works now as intended
        //    QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());

        // open main window
        mainWindow = new KonversationMainWindow();
        //setMainWidget(mainWindow); //TODO FIXME do we need any of the other semantics this use to gain us?

        connect(mainWindow, SIGNAL(showQuickConnectDialog()), this, SLOT(openQuickConnectDialog()) );
        connect(Preferences::self(), SIGNAL(updateTrayIcon()), mainWindow, SLOT(updateTrayIcon()) );
        connect(osd, SIGNAL(hidden()), mainWindow, SIGNAL(endNotification()));
        // take care of user style changes, setting back colors and stuff

        // apply GUI settings
        emit appearanceChanged(); //TODO FIXME i do believe this signal is abused

        if (Preferences::showTrayIcon() && Preferences::hideToTrayOnStartup())
            mainWindow->hide();
        else
            mainWindow->show();

        bool openServerList = Preferences::showServerList();

        // handle autoconnect on startup
        Konversation::ServerGroupList serverGroups = Preferences::serverGroupList();

        if (url.isEmpty() && !args->isSet("server"))
        {
            for (Konversation::ServerGroupList::iterator it = serverGroups.begin(); it != serverGroups.end(); ++it)
            {
                if ((*it)->autoConnectEnabled())
                {
                    openServerList = false;
                    m_connectionManager->connectTo(Konversation::CreateNewConnection, (*it)->id());
                }
            }
        }

        if (openServerList) mainWindow->openServerList();

        connect(this, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettingsPtr)), this, SLOT(saveOptions()));

#ifdef SOMEHOW_MAGICALLY_DCOP_WORKS_IN_KDE4_NOW
        // prepare dcop interface
        dcopObject = new KonvDCOP;
        kapp->dcopClient()->setDefaultObject(dcopObject->objId());
        identDCOP = new KonvIdentDCOP;

        if (dcopObject)
        {
            connect(dcopObject,SIGNAL (dcopMultiServerRaw(const QString&)),
                this,SLOT (dcopMultiServerRaw(const QString&)) );
            connect(dcopObject,SIGNAL (dcopRaw(const QString&,const QString&)),
                this,SLOT (dcopRaw(const QString&,const QString&)) );
            connect(dcopObject,SIGNAL (dcopSay(const QString&,const QString&,const QString&)),
                this,SLOT (dcopSay(const QString&,const QString&,const QString&)) );
            connect(dcopObject,SIGNAL (dcopInfo(const QString&)),
                this,SLOT (dcopInfo(const QString&)) );
            connect(dcopObject,SIGNAL (dcopInsertMarkerLine()),
                mainWindow,SIGNAL(insertMarkerLine()));
            connect(dcopObject, SIGNAL(connectTo(Konversation::ConnectionFlag, const QString&, const QString&, const QString&, const QString&, const QString&, bool)),
                m_connectionManager, SLOT(connectTo(Konversation::ConnectionFlag, const QString&, const QString&, const QString&, const QString&, const QString&, bool)));
        }
#endif //SOMEHOW_MAGICALLY_DCOP_WORKS_IN_KDE4_NOW

        m_notificationHandler = new Konversation::NotificationHandler(this);
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

KonversationApplication* KonversationApplication::instance()
{
    return static_cast<KonversationApplication*>(KApplication::kApplication());
}

void KonversationApplication::prepareShutdown()
{
    mainWindow->getViewContainer()->prepareShutdown();

    m_awayManager->blockSignals(true);
    delete m_awayManager;

    m_connectionManager->quitServers();
    m_connectionManager->blockSignals(true);
    delete m_connectionManager;
}

void KonversationApplication::showQueueTuner(bool p)
{
    getMainWindow()->getViewContainer()->showQueueTuner(p);
}

void KonversationApplication::dcopMultiServerRaw(const QString &command)
{
    sendMultiServerCommand(command.section(' ', 0,0), command.section(' ', 1));
}

void KonversationApplication::dcopRaw(const QString& connection, const QString &command)
{
    Server* server = 0;

    bool conversion = false;
    int connectionId = connection.toInt(&conversion);

    if (conversion) server = getConnectionManager()->getServerByConnectionId(connectionId);

    if (!server) server = getConnectionManager()->getServerByName(connection);

    if (server) server->dcopRaw(command);
}


void KonversationApplication::dcopSay(const QString& connection, const QString& target, const QString& command)
{
    Server* server = 0;

    bool conversion = false;
    int connectionId = connection.toInt(&conversion);

    if (conversion) server = getConnectionManager()->getServerByConnectionId(connectionId);

    if (!server) server = getConnectionManager()->getServerByName(connection);

    if (server) server->dcopSay(target, command);
}

void KonversationApplication::dcopInfo(const QString& string)
{
    mainWindow->getViewContainer()->appendToFrontmost(i18n("DCOP"), string, 0);
}

void KonversationApplication::readOptions()
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
        Preferences::setSortOrder(sortOrder);
    }

    // Identity list
    QStringList identityList=KGlobal::config()->groupList().grep(QRegExp("Identity [0-9]+"));
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
            newIdentity->setShowAwayMessage(cgIdentity.readEntry("ShowAwayMessage", false));
            newIdentity->setAwayMessage(cgIdentity.readEntry("AwayMessage"));
            newIdentity->setReturnMessage(cgIdentity.readEntry("ReturnMessage"));
            newIdentity->setAutomaticAway(cgIdentity.readEntry("AutomaticAway", false));
            newIdentity->setAwayInactivity(cgIdentity.readEntry<int>("AwayInactivity", 10));
            newIdentity->setAutomaticUnaway(cgIdentity.readEntry("AutomaticUnaway", false));

            newIdentity->setQuitReason(cgIdentity.readEntry("QuitReason"));
            newIdentity->setPartReason(cgIdentity.readEntry("PartReason"));
            newIdentity->setKickReason(cgIdentity.readEntry("KickReason"));

            newIdentity->setShellCommand(cgIdentity.readEntry("PreShellCommand"));

            newIdentity->setCodecName(cgIdentity.readEntry("Codec"));

            newIdentity->setAwayNick(cgIdentity.readEntry("AwayNick"));

            Preferences::addIdentity(newIdentity);

        }

    }
/*
    osd->setEnabled(Preferences::useOSD());

    //How to load the font from the text?
    osd->setFont(Preferences::oSDFont());

    osd->setDuration(Preferences::oSDDuration());
    osd->setScreen(Preferences::oSDScreen());
    osd->setShadow(Preferences::oSDDrawShadow());

    osd->setOffset(Preferences::oSDOffsetX(), Preferences::oSDOffsetY());
    osd->setAlignment((OSDWidget::Alignment)Preferences::oSDAlignment());

    if(Preferences::oSDUseCustomColors())
    {
        osd->setTextColor(Preferences::oSDTextColor());
        osd->setBackgroundColor(Preferences::oSDBackgroundColor());
    }
*/
    // Check if there is old server list config //TODO FIXME why are we doing this here?
    KConfigGroup cgServerList(KGlobal::config()->group("Server List"));

    // Read the new server settings
    QStringList groups = KGlobal::config()->groupList().grep(QRegExp("ServerGroup [0-9]+"));
    QMap<int,QStringList> notifyList;

    if(!groups.isEmpty())
    {
        Konversation::ServerGroupList serverGroups;
        QStringList::iterator it;
        QStringList tmp1;
        QStringList::iterator it2;
        Konversation::ChannelList channelHistory;
        Konversation::ServerSettings server;
        Konversation::ChannelSettings channel;

        for (it = groups.begin(); it != groups.end(); ++it)
        {
            KConfigGroup cgServerGroup(KGlobal::config()->group(*it));
            Konversation::ServerGroupSettingsPtr serverGroup(new Konversation::ServerGroupSettings);
            serverGroup->setName(cgServerGroup.readEntry("Name"));
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

            serverGroups.append(serverGroup);
        }

        Preferences::setServerGroupList(serverGroups);
    }

    // Notify Settings and lists.  Must follow Server List.
    Preferences::setNotifyList(notifyList);
    Preferences::setNotifyDelay(Preferences::notifyDelay());
    Preferences::setUseNotify(Preferences::useNotify());

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
    QStringList autoreplaceList(Preferences::autoreplaceList());
    // Read all entries
    index=0;
    while (cgAutoreplace.hasKey(QString("Autoreplace%1").arg(index)))
    {
  // read entry and get length of the string
        QString entry=cgAutoreplace.readEntry(QString("Autoreplace%1").arg(index++));
        unsigned int length=entry.length()-1;
        // if there's a "#" in the end, strip it (used to preserve blanks at the end of the replacement text)
        // there should always be one, but older versions did not do it, so we check first
        if (entry.at(length)=='#')
            entry=entry.left(length);
        // add entry to internal list
        autoreplaceList.append(entry);
    } // while
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
            Preferences::addHighlight(hiList[hiIndex],false,'#'+hiList[hiIndex+1],QString(),QString());
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
                cgHilight.readEntry("AutoText")
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
        Preferences::setAliasList(newList);

    // Channel Encodings
    KConfigGroup cgChannelEncodings(KGlobal::config()->group("Channel Encodings"));
    QMap<QString,QString> channelEncodingEntries=cgChannelEncodings.entryMap();
    QRegExp re("^(.+) ([^\\s]+)$");
    QList<QString> channelEncodingEntryKeys=channelEncodingEntries.keys();

    for(QList<QString>::iterator itStr=channelEncodingEntryKeys.begin(); itStr != channelEncodingEntryKeys.end(); ++itStr)
    {
        if(re.indexIn(*itStr) > -1)
        {
            int serverGroupId = Preferences::serverGroupIdByName(re.cap(1));
            if(serverGroupId != -1)
                Preferences::setChannelEncoding(serverGroupId,re.cap(2),channelEncodingEntries[*itStr]);
        }
    }

    // O, what a tangled web
    Server::_fetchRates();
}

void KonversationApplication::saveOptions(bool updateGUI)
{
    // template:    KConfigGroup  (KGlobal::config()->group( ));

    //KConfig* config=KGlobal::config();

//    Should be handled in NicklistBehaviorConfigController now
//    config->setGroup("Sort Nicknames");

    // Clean up identity list
    QStringList identities=KGlobal::config()->groupList().grep(QRegExp("Identity [0-9]+"));
    if (identities.count())
    {
        // remove old identity list from Preferences::file to keep numbering under control
        for (int index=0; index < identities.count(); index++)
            KGlobal::config()->deleteGroup(identities[index]);
    }

    IdentityList identityList = Preferences::identityList();
    int index = 0;

    for (IdentityList::ConstIterator it = identityList.begin(); it != identityList.end(); ++it)
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
        cgIdentity.writeEntry("ShowAwayMessage",identity->getShowAwayMessage());
        cgIdentity.writeEntry("AwayMessage",identity->getAwayMessage());
        cgIdentity.writeEntry("ReturnMessage",identity->getReturnMessage());
        cgIdentity.writeEntry("AutomaticAway", identity->getAutomaticAway());
        cgIdentity.writeEntry("AwayInactivity", identity->getAwayInactivity());
        cgIdentity.writeEntry("AutomaticUnaway", identity->getAutomaticUnaway());
        cgIdentity.writeEntry("QuitReason",identity->getQuitReason());
        cgIdentity.writeEntry("PartReason",identity->getPartReason());
        cgIdentity.writeEntry("KickReason",identity->getKickReason());
        cgIdentity.writeEntry("PreShellCommand",identity->getShellCommand());
        cgIdentity.writeEntry("Codec",identity->getCodecName());
        cgIdentity.writeEntry("AwayNick", identity->getAwayNick());
        index++;
    } // endfor

    // Remove the old servergroups from the config
    QStringList groups = KGlobal::config()->groupList().grep(QRegExp("ServerGroup [0-9]+"));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KGlobal::config()->deleteGroup((*it));
        }
    }

    // Remove the old servers from the config
    groups = KGlobal::config()->groupList().grep(QRegExp("Server [0-9]+"));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KGlobal::config()->deleteGroup((*it));
        }
    }

    // Remove the old channels from the config
    groups = KGlobal::config()->groupList().grep(QRegExp("Channel [0-9]+"));
    if (groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            KGlobal::config()->deleteGroup((*it));
        }
    }

    // Add the new servergroups to the config
    Konversation::ServerGroupList serverGroupList = Preferences::serverGroupList();
    Konversation::ServerGroupList::iterator it;
    index = 0;
    int index2 = 0;
    int index3 = 0;
    int width = QString::number(serverGroupList.count()).length();
    QString groupName;
    QStringList servers;
    Konversation::ServerList::iterator it2;
    Konversation::ServerList serverlist;
    Konversation::ChannelList channelList;
    Konversation::ChannelList::iterator it3;
    QStringList channels;
    QStringList channelHistory;

    for(it = serverGroupList.begin(); it != serverGroupList.end(); ++it)
    {
        serverlist = (*it)->serverList();
        servers.clear();

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

        channelList = (*it)->channelList();
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

        channelList = (*it)->channelHistory();
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

        QString sgn("ServerGroup %1");
        sgn.arg(QString::number(index).rightJustified(width,'0'));
        KConfigGroup cgServerGroup(KGlobal::config()->group(sgn));
        cgServerGroup.writeEntry("Name", (*it)->name());
        cgServerGroup.writeEntry("Identity", (*it)->identity()->getName());
        cgServerGroup.writeEntry("ServerList", servers);
        cgServerGroup.writeEntry("AutoJoinChannels", channels);
        cgServerGroup.writeEntry("ConnectCommands", (*it)->connectCommands());
        cgServerGroup.writeEntry("AutoConnect", (*it)->autoConnectEnabled());
        cgServerGroup.writeEntry("ChannelHistory", channelHistory);
        cgServerGroup.writeEntry("EnableNotifications", (*it)->enableNotifications());
        cgServerGroup.writeEntry("Expanded", (*it)->expanded());
        cgServerGroup.writeEntry("NotifyList",Preferences::notifyStringByGroupName((*it)->name()));
        index++;
    }

    KGlobal::config()->deleteGroup("Server List");

    // Ignore List
    KGlobal::config()->deleteGroup("Ignore List");
    KConfigGroup cgIgnoreList(KGlobal::config()->group("Ignore List"));
    Q3PtrList<Ignore> ignoreList=Preferences::ignoreList();
    Ignore* item=ignoreList.first();
    index=0;
    while(item)
    {
        cgIgnoreList.writeEntry(QString("Ignore%1").arg(index),QString("%1,%2").arg(item->getName()).arg(item->getFlags()));
        item=ignoreList.next();
        index++;
    }

    // Channel Encodings
    // remove all entries once
    KGlobal::config()->deleteGroup("Channel Encodings");
    KConfigGroup cgChanEncoding(KGlobal::config()->group("Channel Encodings"));
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
                QString key = sgsp->name() + ' ' + (*encChannel);
                cgChanEncoding.writeEntry(key, enc);
            }
        }
    }

    KGlobal::config()->sync();

    if(updateGUI)
        emit appearanceChanged();
}

// FIXME: use KUrl maybe?
void KonversationApplication::storeUrl(const QString& who,const QString& newUrl)
{
    QString url(newUrl);
    // clean up URL to help KRun() in URL catcher interface
    if(url.startsWith("www.")) url="http://"+url;
    else if(url.startsWith("ftp.")) url="ftp://"+url;

    url=url.replace("&amp;","&");

    // check that we don't add the same URL twice
    deleteUrl(who,url);
    urlList.append(who+' '+url);
    emit catchUrl(who,url);
}

const QStringList& KonversationApplication::getUrlList()
{
    return urlList;
}

void KonversationApplication::deleteUrl(const QString& who,const QString& url)
{
    urlList.remove(who+' '+url);
}

void KonversationApplication::clearUrlList()
{
    urlList.clear();
}

void KonversationApplication::openQuickConnectDialog()
{
    quickConnectDialog = new QuickConnectDialog(mainWindow);
    connect(quickConnectDialog, SIGNAL(connectClicked(Konversation::ConnectionFlag, const QString&, const QString&,
        const QString&, const QString&, const QString&, bool)),
        m_connectionManager, SLOT(connectTo(Konversation::ConnectionFlag, const QString&, const QString&,
        const QString&, const QString&, const QString&, bool)));
    quickConnectDialog->show();
}

void KonversationApplication::sendMultiServerCommand(const QString& command, const QString& parameter)
{
    Q3PtrList<Server> serverList = getConnectionManager()->getServerList();

    for (Server* server = serverList.first(); server; server = serverList.next())
        server->executeMultiServerCommand(command, parameter);
}

void KonversationApplication::splitNick_Server(const QString& nick_server, QString &ircnick, QString &serverOrGroup)
{
    //kaddresbook uses the utf separator 0xE120, so treat that as a separator as well
    QString nickServer = nick_server;
    nickServer.replace(QChar(0xE120), "@");
    ircnick = nickServer.section("@",0,0);
    serverOrGroup = nickServer.section("@",1);
}

NickInfoPtr KonversationApplication::getNickInfo(const QString &ircnick, const QString &serverOrGroup)
{
    Q3PtrList<Server> serverList = getConnectionManager()->getServerList();
    NickInfoPtr nickInfo;
    QString lserverOrGroup = serverOrGroup.toLower();
    for (Server* lookServer = serverList.first(); lookServer; lookServer = serverList.next())
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
QString KonversationApplication::doAutoreplace(const QString& text,bool output)
{
    // get autoreplace list
    QStringList autoreplaceList=Preferences::autoreplaceList();
    // working copy
    QString line=text;

    // loop through the list of replacement patterns
    for (int index=0;index<autoreplaceList.count();index++)
    {
        // get autoreplace definition
        QString definition=autoreplaceList[index];
        // split definition in parts
        QString regex=definition.section(',',0,0);
        QString direction=definition.section(',',1,1);
        QString pattern=definition.section(',',2,2);
        QString replacement=definition.section(',',3);

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
                    replacement = definition.section(',',3);
                    // find matches
                    index = line.find(needleReg, index);

                    if (index != -1)
                    {
                        // remember captured patterns
                        QStringList captures = needleReg.capturedTexts();

                        // replace %0 - %9 in regex groups
                        for (int capture=0;capture<captures.count();capture++)
                        {
                            replacement.replace(QString("%%1").arg(capture),captures[capture]);
                        }
                        replacement.replace(QRegExp("%[0-9]"),QString());
                        // replace input with replacement
                        line.replace(index, captures[0].length(), replacement);
                        index += replacement.length();
                    }
                } while (index >= 0 && index < (int)line.length());
            }
            else
            {
                QRegExp needleReg("\\b" + QRegExp::escape(pattern) + "\\b");
                needleReg.setCaseSensitivity(Qt::CaseInsensitive);
                line.replace(needleReg,replacement);
            }
        }
    }

  return line;
}

// #include "./application.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
