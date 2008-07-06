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

#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "connectionmanager.h"
#include "awaymanager.h"
#include "dcctransfermanager.h"
#include "viewcontainer.h"
#include "highlight.h"
#include "server.h"
#include "konversationsound.h"
#include "quickconnectdialog.h"
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
#include <dcopclient.h>
#include <kdeversion.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kiconloader.h>


KonversationApplication::KonversationApplication()
: KUniqueApplication(true, true, true)
{
    mainWindow = 0;
    m_connectionManager = 0;
    m_awayManager = 0;
    quickConnectDialog = 0;
    osd = 0;
}

KonversationApplication::~KonversationApplication()
{
    kdDebug() << k_funcinfo << endl;
    Server::_stashRates();
    Preferences::writeConfig();
    saveOptions(false);

    delete m_images;
    delete dcopObject;
    //delete prefsDCOP;
    delete identDCOP;
    delete osd;
    osd = 0;
}

int KonversationApplication::newInstance()
{
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    QCString url;
    if (args->count() > 0) url = args->arg(0);

    if (!mainWindow)
    {
        m_connectionManager = new ConnectionManager(this);

        m_awayManager = new AwayManager(this);

        connect(m_connectionManager, SIGNAL(identityOnline(int)), m_awayManager, SLOT(identityOnline(int)));
        connect(m_connectionManager, SIGNAL(identityOffline(int)), m_awayManager, SLOT(identityOffline(int)));
        connect(m_connectionManager, SIGNAL(identityOffline(int)), m_awayManager, SLOT(identityOffline(int)));
        connect(m_connectionManager, SIGNAL(connectionChangedAwayState(bool)), m_awayManager, SLOT(updateGlobalAwayAction(bool)));

        // an instance of DccTransferManager needs to be created before GUI class instances' creation.
        m_dccTransferManager = new DccTransferManager(this);

        // make sure all vars are initialized properly
        quickConnectDialog = 0;

        // Sound object used to play sound...
        m_sound = new Konversation::Sound(this);

        // initialize OSD display here, so we can read the Preferences::properly
        osd = new OSDWidget( "Konversation" );

        Preferences::self();
        readOptions();

        // Images object providing LEDs, NickIcons
        m_images = new Images();
        connect(this, SIGNAL(iconChanged(int)), m_images, SLOT(updateIcons()));

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
        setMainWidget(mainWindow);

        connect(mainWindow, SIGNAL(showQuickConnectDialog()), this, SLOT(openQuickConnectDialog()) );
        connect(Preferences::self(), SIGNAL(updateTrayIcon()), mainWindow, SLOT(updateTrayIcon()) );
        connect(osd, SIGNAL(hidden()), mainWindow, SIGNAL(endNotification()));
        // take care of user style changes, setting back colors and stuff

        // apply GUI settings
        emit appearanceChanged();

        if (Preferences::showTrayIcon() && (Preferences::hiddenToTray() || Preferences::hideToTrayOnStartup()))
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

        connect(this, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettings*)), this, SLOT(saveOptions()));

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
    KConfig* config=kapp->config();

    // read nickname sorting order for channel nick lists
    config->setGroup("Sort Nicknames");
    QString sortOrder=config->readEntry("SortOrder");
    QStringList sortOrderList=QStringList::split("",sortOrder);
    sortOrderList.sort();
    if (sortOrderList.join("")!="-hopqv")
    {
      sortOrder=Preferences::defaultNicknameSortingOrder();
      Preferences::setSortOrder(sortOrder);
    }

    // Identity list
    QStringList identityList=config->groupList().grep(QRegExp("Identity [0-9]+"));
    if(!identityList.isEmpty())
    {
        Preferences::clearIdentityList();

        for(unsigned int index=0;index<identityList.count();index++)
        {
            IdentityPtr newIdentity=new Identity();

            config->setGroup(identityList[index]);

            newIdentity->setName(config->readEntry("Name"));

            newIdentity->setIdent(config->readEntry("Ident"));
            newIdentity->setRealName(config->readEntry("Realname"));

            newIdentity->setNicknameList(config->readListEntry("Nicknames"));

            newIdentity->setBot(config->readEntry("Bot"));
            newIdentity->setPassword(config->readEntry("Password"));

            newIdentity->setInsertRememberLineOnAway(config->readBoolEntry("InsertRememberLineOnAway"));
            newIdentity->setShowAwayMessage(config->readBoolEntry("ShowAwayMessage"));
            newIdentity->setAwayMessage(config->readEntry("AwayMessage"));
            newIdentity->setReturnMessage(config->readEntry("ReturnMessage"));
            newIdentity->setAutomaticAway(config->readBoolEntry("AutomaticAway", false));
            newIdentity->setAwayInactivity(config->readNumEntry("AwayInactivity", 10));
            newIdentity->setAutomaticUnaway(config->readBoolEntry("AutomaticUnaway", false));

            newIdentity->setQuitReason(config->readEntry("QuitReason"));
            newIdentity->setPartReason(config->readEntry("PartReason"));
            newIdentity->setKickReason(config->readEntry("KickReason"));

            newIdentity->setShellCommand(config->readEntry("PreShellCommand"));

            newIdentity->setCodecName(config->readEntry("Codec"));

            newIdentity->setAwayNick(config->readEntry("AwayNick"));

            Preferences::addIdentity(newIdentity);

        }

    }

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

    // Check if there is old server list config
    config->setGroup("Server List");

    // Read the new server settings
    QStringList groups = config->groupList().grep(QRegExp("ServerGroup [0-9]+"));
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

        for(it = groups.begin(); it != groups.end(); ++it)
        {
            config->setGroup((*it));
            Konversation::ServerGroupSettingsPtr serverGroup = new Konversation::ServerGroupSettings;
            serverGroup->setName(config->readEntry("Name"));
            serverGroup->setIdentityId(Preferences::identityByName(config->readEntry("Identity"))->id());
            serverGroup->setConnectCommands(config->readEntry("ConnectCommands"));
            serverGroup->setAutoConnectEnabled(config->readBoolEntry("AutoConnect"));
            serverGroup->setNotificationsEnabled(config->readBoolEntry("EnableNotifications", true));
            serverGroup->setExpanded(config->readBoolEntry("Expanded", false));

            notifyList.insert((*serverGroup).id(),QStringList::split(' ',config->readEntry("NotifyList")));

            tmp1 = config->readListEntry("ServerList");
            for(it2 = tmp1.begin(); it2 != tmp1.end(); ++it2)
            {
                config->setGroup((*it2));
                server.setHost(config->readEntry("Server"));
                server.setPort(config->readNumEntry("Port"));
                server.setPassword(config->readEntry("Password"));
                server.setSSLEnabled(config->readBoolEntry("SSLEnabled"));
                serverGroup->addServer(server);
            }

            config->setGroup((*it));
            tmp1 = config->readListEntry("AutoJoinChannels");

            for(it2 = tmp1.begin(); it2 != tmp1.end(); ++it2)
            {
                config->setGroup((*it2));

                if(!config->readEntry("Name").isEmpty())
                {
                    channel.setName(config->readEntry("Name"));
                    channel.setPassword(config->readEntry("Password"));
                    serverGroup->addChannel(channel);
                }
            }

            config->setGroup((*it));
            tmp1 = config->readListEntry("ChannelHistory");
            channelHistory.clear();

            for(it2 = tmp1.begin(); it2 != tmp1.end(); ++it2)
            {
                config->setGroup((*it2));

                if(!config->readEntry("Name").isEmpty())
                {
                    channel.setName(config->readEntry("Name"));
                    channel.setPassword(config->readEntry("Password"));
                    channel.setNotificationsEnabled(config->readBoolEntry("EnableNotifications", true));
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
    if(config->hasGroup("Button List")) Preferences::clearQuickButtonList();
    config->setGroup("Button List");
    // Read all default buttons
    QStringList buttonList(Preferences::quickButtonList());
    // Read all quick buttons
    int index=0;
    while(config->hasKey(QString("Button%1").arg(index)))
    {
      buttonList.append(config->readEntry(QString("Button%1").arg(index++)));
    } // while
    // Put back the changed button list
    Preferences::setQuickButtonList(buttonList);

    // Autoreplace List

    // if there are autoreplace definitions in the config file, remove default entries
    if(config->hasGroup("Autoreplace List")) Preferences::clearAutoreplaceList();
    config->setGroup("Autoreplace List");
    // Read all default entries
    QStringList autoreplaceList(Preferences::autoreplaceList());
    // Read all entries
    index=0;
    while(config->hasKey(QString("Autoreplace%1").arg(index)))
    {
      // read entry and get length of the string
      QString entry=config->readEntry(QString("Autoreplace%1").arg(index++));
      unsigned int length=entry.length()-1;
      // if there's a "#" in the end, strip it (used to preserve blanks at the end of the replacement text)
      // there should always be one, but older versions did not do it, so we check first
      if(entry.at(length)=='#') entry=entry.left(length);
      // add entry to internal list
      autoreplaceList.append(entry);
    } // while
    // Put back the changed autoreplace list
    Preferences::setAutoreplaceList(autoreplaceList);

    // Highlight List
    if(config->hasKey("Highlight"))               // Stay compatible with versions < 0.14
    {
        QString highlight=config->readEntry("Highlight");
        QStringList hiList=QStringList::split(' ',highlight);

        unsigned int hiIndex;
        for(hiIndex=0;hiIndex<hiList.count();hiIndex+=2)
        {
            Preferences::addHighlight(hiList[hiIndex],false,'#'+hiList[hiIndex+1],QString(),QString());
        }

        config->deleteEntry("Highlight");
    }
    else
    {
        int i = 0;

        while(config->hasGroup(QString("Highlight%1").arg(i)))
        {
            config->setGroup(QString("Highlight%1").arg(i));
            Preferences::addHighlight(config->readEntry("Pattern"),
                config->readBoolEntry("RegExp"),
                config->readColorEntry("Color"),
                config->readPathEntry("Sound"),
                config->readEntry("AutoText"));
            i++;
        }
    }

    // Ignore List
    config->setGroup("Ignore List");
    // Remove all default entries if there is at least one Ignore in the Preferences::file
    if(config->hasKey("Ignore0")) Preferences::clearIgnoreList();
    // Read all ignores
    index=0;
    while(config->hasKey(QString("Ignore%1").arg(index)))
    {
        Preferences::addIgnore(config->readEntry(QString("Ignore%1").arg(index++)));
    }

    // Aliases
    config->setGroup("Aliases");
    QStringList newList=config->readListEntry("AliasList");
    if(!newList.isEmpty()) Preferences::setAliasList(newList);

    // Channel Encodings
    QMap<QString,QString> channelEncodingsEntry=config->entryMap("Channel Encodings");
    QRegExp re("^(.+) ([^\\s]+)$");
    QStringList channelEncodingsEntryKeys=channelEncodingsEntry.keys();
    for(unsigned int i=0; i<channelEncodingsEntry.count(); ++i)
        if(re.search(channelEncodingsEntryKeys[i]) > -1)
            Preferences::setChannelEncoding(re.cap(1),re.cap(2),channelEncodingsEntry[channelEncodingsEntryKeys[i]]);

    // O, what a tangled web
    Server::_fetchRates();
}

void KonversationApplication::saveOptions(bool updateGUI)
{
    KConfig* config=kapp->config();

//    Should be handled in NicklistBehaviorConfigController now
//    config->setGroup("Sort Nicknames");

    // Clean up identity list
    QStringList identities=config->groupList().grep(QRegExp("Identity [0-9]+"));
    if(identities.count())
    {
        // remove old identity list from Preferences::file to keep numbering under control
        for(unsigned int index=0;index<identities.count();index++)
            config->deleteGroup(identities[index]);
    }

    IdentityList identityList = Preferences::identityList();
    int index = 0;

    for(IdentityList::ConstIterator it = identityList.begin(); it != identityList.end(); ++it)
    {
        IdentityPtr identity = (*it);
        config->setGroup(QString("Identity %1").arg(index));

        config->writeEntry("Name",identity->getName());
        config->writeEntry("Ident",identity->getIdent());
        config->writeEntry("Realname",identity->getRealName());
        config->writeEntry("Nicknames",identity->getNicknameList());
        config->writeEntry("Bot",identity->getBot());
        config->writeEntry("Password",identity->getPassword());
        config->writeEntry("InsertRememberLineOnAway", identity->getInsertRememberLineOnAway());
        config->writeEntry("ShowAwayMessage",identity->getShowAwayMessage());
        config->writeEntry("AwayMessage",identity->getAwayMessage());
        config->writeEntry("ReturnMessage",identity->getReturnMessage());
        config->writeEntry("AutomaticAway", identity->getAutomaticAway());
        config->writeEntry("AwayInactivity", identity->getAwayInactivity());
        config->writeEntry("AutomaticUnaway", identity->getAutomaticUnaway());
        config->writeEntry("QuitReason",identity->getQuitReason());
        config->writeEntry("PartReason",identity->getPartReason());
        config->writeEntry("KickReason",identity->getKickReason());
        config->writeEntry("PreShellCommand",identity->getShellCommand());
        config->writeEntry("Codec",identity->getCodecName());
        config->writeEntry("AwayNick", identity->getAwayNick());
        index++;
    }                                             // endfor

    // FIXME: check if this group is still needed
    config->setGroup("Notify List");

    // Remove the old servergroups from the config
    QStringList groups = config->groupList().grep(QRegExp("ServerGroup [0-9]+"));
    if(groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            config->deleteGroup((*it));
        }
    }

    // Remove the old servers from the config
    groups = config->groupList().grep(QRegExp("Server [0-9]+"));
    if(groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            config->deleteGroup((*it));
        }
    }

    // Remove the old channels from the config
    groups = config->groupList().grep(QRegExp("Channel [0-9]+"));
    if(groups.count())
    {
        QStringList::iterator it;
        for(it = groups.begin(); it != groups.end(); ++it)
        {
            config->deleteGroup((*it));
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
            config->setGroup(groupName);
            config->writeEntry("Server", (*it2).host());
            config->writeEntry("Port", (*it2).port());
            config->writeEntry("Password", (*it2).password());
            config->writeEntry("SSLEnabled", (*it2).SSLEnabled());
            index2++;
        }

        channelList = (*it)->channelList();
        channels.clear();

        for(it3 = channelList.begin(); it3 != channelList.end(); ++it3)
        {
            groupName = QString("Channel %1").arg(index3);
            channels.append(groupName);
            config->setGroup(groupName);
            config->writeEntry("Name", (*it3).name());
            config->writeEntry("Password", (*it3).password());
            index3++;
        }

        channelList = (*it)->channelHistory();
        channelHistory.clear();

        for(it3 = channelList.begin(); it3 != channelList.end(); ++it3)
        {
            groupName = QString("Channel %1").arg(index3);
            channelHistory.append(groupName);
            config->setGroup(groupName);
            config->writeEntry("Name", (*it3).name());
            config->writeEntry("Password", (*it3).password());
            config->writeEntry("EnableNotifications", (*it3).enableNotifications());
            index3++;
        }

        config->setGroup(QString("ServerGroup %1").arg(QString::number(index).rightJustify(width,'0')));
        config->writeEntry("Name", (*it)->name());
        config->writeEntry("Identity", (*it)->identity()->getName());
        config->writeEntry("ServerList", servers);
        config->writeEntry("AutoJoinChannels", channels);
        config->writeEntry("ConnectCommands", (*it)->connectCommands());
        config->writeEntry("AutoConnect", (*it)->autoConnectEnabled());
        config->writeEntry("ChannelHistory", channelHistory);
        config->writeEntry("EnableNotifications", (*it)->enableNotifications());
        config->writeEntry("Expanded", (*it)->expanded());
        config->writeEntry("NotifyList",Preferences::notifyStringByGroupName((*it)->name()));
        index++;
    }

    config->deleteGroup("Server List");

    // Ignore List
    config->deleteGroup("Ignore List");
    config->setGroup("Ignore List");
    QPtrList<Ignore> ignoreList=Preferences::ignoreList();
    Ignore* item=ignoreList.first();
    index=0;
    while(item)
    {
        config->writeEntry(QString("Ignore%1").arg(index),QString("%1,%2").arg(item->getName()).arg(item->getFlags()));
        item=ignoreList.next();
        index++;
    }

    // Channel Encodings
    config->setGroup("Channel Encodings");
    QStringList encServers=Preferences::channelEncodingsServerList();
    //i have no idea these would need to be sorted //encServers.sort();
    QStringList::iterator encServer;
    for ( encServer = encServers.begin(); encServer != encServers.end(); ++encServer )
    {
        QStringList encChannels=Preferences::channelEncodingsChannelList(*(encServer));
        //ditto //encChannels.sort();
        QStringList::iterator encChannel;
        for ( encChannel = encChannels.begin(); encChannel != encChannels.end(); ++encChannel )
        {
            QString enc = Preferences::channelEncoding(*(encServer), *(encChannel));
            QString name = *(encServer) + ' ' + *(encChannel);
            if(!enc.isEmpty())
                config->writeEntry(name, enc);
            else
                config->deleteEntry(name);
        }
    }

    config->sync();

    if(updateGUI)
        emit appearanceChanged();
}

// FIXME: use KURL maybe?
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
    QPtrList<Server> serverList = getConnectionManager()->getServerList();

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
    QPtrList<Server> serverList = getConnectionManager()->getServerList();
    NickInfoPtr nickInfo;
    QString lserverOrGroup = serverOrGroup.lower();
    for(Server* lookServer = serverList.first(); lookServer; lookServer = serverList.next())
    {
        if(lserverOrGroup.isEmpty()
            || lookServer->getServerName().lower()==lserverOrGroup
            || lookServer->getDisplayName().lower()==lserverOrGroup)
        {
            nickInfo = lookServer->getNickInfo(ircnick);
            if(nickInfo) return nickInfo;         //If we found one
        }
    }
    return 0;
}

// auto replace on input/output
QString KonversationApplication::doAutoreplace(const QString& text,bool output)
{
    // get autoreplace list
    QStringList autoreplaceList=Preferences::autoreplaceList();
    // working copy
    QString line=text;

    // loop through the list of replacement patterns
    for(unsigned int index=0;index<autoreplaceList.count();index++)
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
        if(direction==isDirection || direction=="io")
        {
            // regular expression pattern?
            if(regex=="1")
            {
                // create regex from pattern
                QRegExp needleReg=pattern;
                // set pattern case insensitive
                needleReg.setCaseSensitive(true);
                int index = 0;

                do {
                    replacement = definition.section(',',3);
                    // find matches
                    index = line.find(needleReg, index);

                    if(index != -1)
                    {
                        // remember captured patterns
                        QStringList captures = needleReg.capturedTexts();

                        // replace %0 - %9 in regex groups
                        for(unsigned int capture=0;capture<captures.count();capture++)
                        {
                            replacement.replace(QString("%%1").arg(capture),captures[capture]);
                        }
                        replacement.replace(QRegExp("%[0-9]"),QString());
                        // replace input with replacement
                        line.replace(index, captures[0].length(), replacement);
                        index += replacement.length();
                    }
                } while(index >= 0 && index < (int)line.length());
            }
            else
            {
                QRegExp needleReg("\\b" + QRegExp::escape(pattern) + "\\b");
                needleReg.setCaseSensitive(false);
                line.replace(needleReg,replacement);
            }
        }
    }

  return line;
}

#include "konversationapplication.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
