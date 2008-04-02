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
  Copyright (C) 2005-2007 Eike Hein <hein@kde.org>
*/

#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "dcctransfermanager.h"
#include "viewcontainer.h"
#include "highlight.h"
#include "server.h"
#include "konversationsound.h"
#include "quickconnectdialog.h"
#include "servergroupsettings.h"
#include "serversettings.h"
#include "channel.h"
#include "nicklistview.h"
#include "images.h"
#include "notificationhandler.h"
#include "commit.h"
#include "version.h"

#include <qtextcodec.h>
#include <qregexp.h>
#include <qfileinfo.h>

#include <kdebug.h>
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
    quickConnectDialog = 0;
    m_connectDelayed=false;
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

void KonversationApplication::delayedConnectToServer(const QString& hostname, const QString& port, const QString& channel,
const QString& nick, const QString& password,
const bool& useSSL)
{
    m_hostName=hostname;
    m_port=port;
    m_channel=channel;
    m_nick=nick;
    m_password=password;
    m_useSSL=useSSL;
    m_connectDelayed=true;
}

int KonversationApplication::newInstance()
{
    if(!mainWindow)
    {
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

        if(Preferences::showTrayIcon() && (Preferences::hiddenToTray() || Preferences::hideToTrayOnStartup()))
        {
            mainWindow->hide();
        }
        else
        {
            mainWindow->show();
        }

        bool openServerList = Preferences::showServerList();

        // handle autoconnect on startup
        Konversation::ServerGroupList serverGroups = Preferences::serverGroupList();

        if (!m_connectDelayed)
        {
            for (Konversation::ServerGroupList::iterator it = serverGroups.begin(); it != serverGroups.end(); ++it)
            {
                if ((*it)->autoConnectEnabled())
                {
                    openServerList = false;
                    connectToServer((*it)->id());
                }
            }
        }
        else
            quickConnectToServer(m_hostName, m_port, m_channel, m_nick, m_password, m_useSSL);

        if (openServerList) mainWindow->openServerList();

        connect(this, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettings*)), this, SLOT(saveOptions()));

        // prepare dcop interface
        dcopObject = new KonvDCOP;
        kapp->dcopClient()->setDefaultObject(dcopObject->objId());
        identDCOP = new KonvIdentDCOP;

        if(dcopObject)
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
            connect(dcopObject,SIGNAL(dcopConnectToServer(const QString&, int,const QString&, const QString&)),
                this,SLOT(dcopConnectToServer(const QString&, int,const QString&, const QString&)));
        }

        m_notificationHandler = new Konversation::NotificationHandler(this);
    }

    return KUniqueApplication::newInstance();
}

                                                  // static
KonversationApplication* KonversationApplication::instance()
{
    return static_cast<KonversationApplication*>( KApplication::kApplication() );
}

KonversationMainWindow *KonversationApplication::getMainWindow()
{
    return mainWindow;
}

void KonversationApplication::showQueueTuner(bool p)
{
    getMainWindow()->getViewContainer()->showQueueTuner(p);
}

void KonversationApplication::toggleAway()
{
    bool anyservers = false;
    bool alreadyaway = false;

    Server* lookServer=serverList.first();

    while(lookServer)
    {
        if(lookServer->isConnected())
        {
            anyservers = true;
            if(lookServer->isAway())
            {
                alreadyaway= true;
                break;
            }
        }
        lookServer=serverList.next();
    }

    //alreadyaway is true if _any_ servers are away
    if (alreadyaway)
        sendMultiServerCommand("back", QString());
    else
        sendMultiServerCommand("away", QString());
}

void KonversationApplication::dcopMultiServerRaw(const QString &command)
{
    sendMultiServerCommand(command.section(' ', 0,0), command.section(' ', 1));
}

void KonversationApplication::dcopRaw(const QString& server, const QString &command)
{
    Server* lookServer=serverList.first();
    while(lookServer)
    {
        if(lookServer->getServerName()==server)
        {
            lookServer->dcopRaw(command);
            //      break; // leave while loop
            //FIXME:   <muesli> there's a reason for not breaking this loop...  [see comment for dcopSay]
        }
        lookServer=serverList.next();
    }

}

void KonversationApplication::dcopSay(const QString& server,const QString& target,const QString& command)
{
    Server* lookServer=serverList.first();
    while(lookServer)
    {
        if(lookServer->getServerName()==server)
        {
            lookServer->dcopSay(target,command);
            //      break; // leave while loop
            //FIXME:   <muesli> there's a reason for not breaking this loop, here (which would spent only some
            //                  cpu cycles, anyways): I am connected to two bouncers at the same time, which are
            //                  also named the same (same ip, no dns). if a dcopSay gets emerged, it will always
            //                  get the _same_ server name as its parameter (both are named the same). although
            //                  the channel it gets sent to, is on the second server, it will always try to send
            //                  this information to a channel on the first server, which i didn't even join.
            //                  this is def. a quick-fix, we should probably handle server-id's instead of -names.
        }
        lookServer=serverList.next();
    }
}

void KonversationApplication::dcopInfo(const QString& string)
{
    mainWindow->getViewContainer()->appendToFrontmost(i18n("DCOP"), string, 0);
}

bool KonversationApplication::validateIdentity(IdentityPtr identity, bool interactive)
{
    QString errors;

    if (identity->getIdent().isEmpty())
        errors+=i18n("Please fill in your <b>Ident</b>.<br>");

    if (identity->getRealName().isEmpty())
        errors+=i18n("Please fill in your <b>Real name</b>.<br>");

    if (identity->getNickname(0).isEmpty())
        errors+=i18n("Please provide at least one <b>Nickname</b>.<br>");

    if (!errors.isEmpty())
    {
        if (interactive)
        {
            int result = KMessageBox::warningContinueCancel(0,
                            i18n("<qt>Your identity \"%1\" is not set up correctly:<br>%2</qt>")
                                .arg(identity->getName()).arg(errors),
                            i18n("Identity Settings"),
                            i18n("Edit Identity..."));

            if (result==KMessageBox::Continue)
            {
                identity = mainWindow->editIdentity(identity);

                if (identity && validateIdentity(identity,false))
                    return true;
                else
                    return false;
            }
            else
            {
                return false;
            }
        }

        return false;
    }

    return true;
}

Server* KonversationApplication::connectToServerGroup(const QString& serverGroup)
{
    int serverGroupId = Preferences::serverGroupIdByName(serverGroup);

    return connectToServer(serverGroupId);
}

Server* KonversationApplication::connectToServer(int serverGroupId, const QString& channel, Konversation::ServerSettings quickServer)
{
    // Check if a server window with same name and port is already open
    Server* lookServer = serverList.first();
    Server* existingServer = 0;

    while (lookServer)
    {
      if (lookServer->serverGroupSettings()->id() == serverGroupId)
      {
          existingServer = lookServer;
          break;
      }

      lookServer = serverList.next();
    }

    // There's already a connection to this network.
    if (existingServer)
    {
        // The connection is active, and the server list dialog requested
        // to connect to a specific server.
        if (existingServer->isConnected() && !quickServer.server().isEmpty())
        {
            // This server differs from the server we're presently connected to.
            if (quickServer.server() != existingServer->getServerName())
            {
                int result = KMessageBox::warningContinueCancel(
                mainWindow,
                i18n("You are already connected to %1. Do you want to disconnect from '%2' and connect to '%3' instead?")
                    .arg(existingServer->getServerGroup())
                    .arg(existingServer->getServerName())
                    .arg(quickServer.server()),
                i18n("Already connected to %1").arg(existingServer->getServerGroup()),
                i18n("Disconnect"),
                "ReconnectDifferentServer");

                // The user has chosen to connect to this different server
                // instead of the current one.
                if (result == KMessageBox::Continue)
                {
                    existingServer->disconnect();
                    existingServer->serverGroupSettings()->clearQuickServerList();
                    existingServer->serverGroupSettings()->setQuickServerList(quickServer);
                    existingServer->resetCurrentServerIndex();
                    existingServer->reconnect();
                    return existingServer;
                }
                // The user wants to keep the current connection alive, so
                // return the existing connection.
                else
                    return existingServer;
            }
            // We've gotten a request to connect to the same server we're
            // already connected to, so return the existing connection.
            else
                return existingServer;
        }
        // We haven't been told to connect to a specific server and already have
        // a connection to this network, so return it.
        else if (existingServer->isConnected() && quickServer.server().isEmpty())
        {
            return existingServer;
        }
        // The connection is inactive.
        else if (!existingServer->isConnected())
        {
            // We've been told to connect to a specific server. Do so.
            if (!quickServer.server().isEmpty())
            {
                existingServer->serverGroupSettings()->clearQuickServerList();
                existingServer->serverGroupSettings()->setQuickServerList(quickServer);
                existingServer->resetCurrentServerIndex();
                existingServer->reconnect();
                return existingServer;
            }
            // No specific server was part of the request, so reconnect
            // the old one.
            else
            {
                existingServer->reconnect();
                return existingServer;
            }
        }
    }

    Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(serverGroupId);
    IdentityPtr identity = serverGroup->identity();

    if (!identity || !validateIdentity(identity))
        return 0;

    emit closeServerList();

    bool clearQuickServerList = true;

    // Have we been called with a quickServer? If so, append to list & prevent
    // simple Server constructor from cleaing its quickServer list like it
    // would do normally.
    if (!quickServer.server().isEmpty())
    {
        serverGroup->setQuickServerList(quickServer);
        clearQuickServerList = false;
    }

    Server* newServer = new Server(mainWindow->getViewContainer(), serverGroupId, clearQuickServerList, channel);

    connect(mainWindow,SIGNAL (startNotifyTimer(int)),newServer,SLOT (startNotifyTimer(int)) );
    connect(mainWindow,SIGNAL (quitServer()),newServer,SLOT (quitServer()) );
    connect(newServer, SIGNAL(connectionChangedState(Server*, Server::State)),
        mainWindow, SIGNAL(serverStateChanged(Server*, Server::State)));

    connect(newServer,SIGNAL (nicksNowOnline(Server*,const QStringList&,bool)),mainWindow,SLOT (setOnlineList(Server*,const QStringList&,bool)) );

    connect(newServer,SIGNAL (deleted(Server*)),this,SLOT (removeServer(Server*)) );

    connect(newServer, SIGNAL(multiServerCommand(const QString&, const QString&)),
        this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
    connect(newServer, SIGNAL(awayInsertRememberLine(Server*)), mainWindow, SIGNAL(triggerRememberLines(Server*)));

    serverList.append(newServer);

    return newServer;
}

void KonversationApplication::quickConnectToServer(const QString& hostName, const QString& port, const QString& channel, const QString& nick, const QString& password, const bool& useSSL)
{
    //used for the quick connect dialog and /server command

    IdentityPtr identity;
    Konversation::ServerGroupSettingsPtr serverGroupOfServer;

    // If server is in an existing group, use that group (first group if server is in multiple groups)
    if (serverGroupOfServer = Preferences::serverGroupByServer(hostName))
        identity = serverGroupOfServer->identity();
    else
        identity = Preferences::identityByName("Default");

    if (!identity || !validateIdentity(identity))
        return;

    Server* newServer = new Server(mainWindow->getViewContainer(), hostName, port, channel, nick, password, useSSL);

    connect(mainWindow,SIGNAL (startNotifyTimer(int)),newServer,SLOT (startNotifyTimer(int)) );
    connect(mainWindow,SIGNAL (quitServer()),newServer,SLOT (quitServer()) );
    connect(newServer, SIGNAL(connectionChangedState(Server*, Server::State)),
        mainWindow, SIGNAL(serverStateChanged(Server*, Server::State)));

    connect(newServer,SIGNAL (nicksNowOnline(Server*,const QStringList&,bool)),mainWindow,SLOT (setOnlineList(Server*,const QStringList&,bool)) );

    connect(newServer,SIGNAL (deleted(Server*)),this,SLOT (removeServer(Server*)) );

    connect(newServer, SIGNAL(multiServerCommand(const QString&, const QString&)),
        this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
    connect(newServer, SIGNAL(awayInsertRememberLine(Server*)), mainWindow, SIGNAL(triggerRememberLines(Server*)));

    serverList.append(newServer);
}

Server* KonversationApplication::getServerByName(const QString& name)
{
    Server* lookServer=serverList.first();

    while(lookServer)
    {
        if(lookServer->getServerName()==name) return lookServer;
        lookServer=serverList.next();
    }

    return 0;
}

Server* KonversationApplication::getServerByServerGroupId(int id)
{
    Server* lookServer=serverList.first();

    while(lookServer)
    {
        if (lookServer->serverGroupSettings()->id() == id)
            return lookServer;
        lookServer=serverList.next();
    }

    return 0;

}

void KonversationApplication::removeServer(Server* server)
{
    serverList.setAutoDelete(false);              // don't delete items when they are removed
    if(!serverList.remove(server))
        kdDebug() << "Could not remove " << server->getServerName() << endl;
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
                server.setServer(config->readEntry("Server"));
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

    QValueList<IdentityPtr> identityList = Preferences::identityList();
    int index = 0;

    for(QValueList<IdentityPtr>::iterator it = identityList.begin(); it != identityList.end(); ++it)
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
        serverlist = (*it)->serverList(true);
        servers.clear();

        for(it2 = serverlist.begin(); it2 != serverlist.end(); ++it2)
        {
            groupName = QString("Server %1").arg(index2);
            servers.append(groupName);
            config->setGroup(groupName);
            config->writeEntry("Server", (*it2).server());
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
/*
    Should be done in HighlightConfigController now ...
    remove this part as soon as we are certain it works

    // Write all highlight entries
    QPtrList<Highlight> hiList=Preferences::highlightList();
    int i = 0;
    for(Highlight* hl = hiList.first(); hl; hl = hiList.next())
    {
        config->setGroup(QString("Highlight%1").arg(i));
        config->writeEntry("Pattern", hl->getPattern());
        config->writeEntry("RegExp", hl->getRegExp());
        config->writeEntry("Color", hl->getColor());
        config->writePathEntry("Sound", hl->getSoundURL().prettyURL());
        config->writeEntry("AutoText", hl->getAutoText());
        i++;
    }

    // Remove unused entries...
    while(config->hasGroup(QString("Highlight%1").arg(i)))
    {
        config->deleteGroup(QString("Highlight%1").arg(i));
        i++;
    }
*/

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
    QStringList channelEncodingsServerList=Preferences::channelEncodingsServerList();
    channelEncodingsServerList.sort();
    for(unsigned int i=0; i<channelEncodingsServerList.count(); ++i)
    {
        QStringList channelEncodingsChannelList=Preferences::channelEncodingsChannelList(channelEncodingsServerList[i]);
        channelEncodingsChannelList.sort();
        for(unsigned int j=0; j<channelEncodingsChannelList.count(); ++j)
            if(!Preferences::channelEncoding(channelEncodingsServerList[i],channelEncodingsChannelList[j]).isEmpty())
                config->writeEntry(channelEncodingsServerList[i]+' '+channelEncodingsChannelList[j],Preferences::channelEncoding(channelEncodingsServerList[i],channelEncodingsChannelList[j]));
    }

    config->sync();

    if(updateGUI)
        emit appearanceChanged();
}

void KonversationApplication::updateNickIcons()
{
    Server* lookServer=serverList.first();

    while(lookServer)
    {
        const QPtrList<Channel> &channelList = lookServer->getChannelList();
        QPtrListIterator<Channel> chanIt(channelList);
        Channel* channel;
        while((channel=chanIt.current()) != 0)
        {
            ++chanIt;
            channel->getNickListView()->refresh();
        }
        lookServer=serverList.next();
    }
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
    connect(quickConnectDialog, SIGNAL(connectClicked(const QString&, const QString&, const QString&, const QString&, const QString&, const bool&)),this, SLOT(quickConnectToServer(const QString&, const QString&, const QString&, const QString&, const QString&,const bool&)));
    quickConnectDialog->show();
}

void KonversationApplication::sendMultiServerCommand(const QString& command, const QString& parameter)
{
    for(Server* server = serverList.first(); server; server = serverList.next())
    {
        server->executeMultiServerCommand(command, parameter);
    }
}

void KonversationApplication::dcopConnectToServer(const QString& url, int port, const QString& channel,
const QString& password)
{
    Server* server = getServerByName(url);

    if(server)
        server->sendJoinCommand(channel);
    else
        quickConnectToServer(url, QString::number(port), channel, password);
}

Konversation::Sound* KonversationApplication::sound()
{
    return m_sound;
}

Images* KonversationApplication::images()
{
    return m_images;
}

// Returns list of pointers to Servers.
const QPtrList<Server> KonversationApplication::getServerList() { return serverList; }

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
    NickInfoPtr nickInfo;
    QString lserverOrGroup = serverOrGroup.lower();
    for(Server* lookServer = serverList.first(); lookServer; lookServer = serverList.next())
    {
        if(lserverOrGroup.isEmpty()
            || lookServer->getServerName().lower()==lserverOrGroup
            || lookServer->getServerGroup().lower()==lserverOrGroup)
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
                needleReg.setCaseSensitive(false);
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
