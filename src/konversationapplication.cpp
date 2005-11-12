/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The main application
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

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

#include "konversationapplication.h"
#include "konversationmainwindow.h"
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

KonversationApplication::KonversationApplication()
: KUniqueApplication(true, true, true)
{
    mainWindow = 0;
    quickConnectDialog = 0;
    colorOffSet = 0;
    m_demoteInProgress = false;
    m_connectDelayed=false;

    demoteTimer = new QTimer(this);
    connect(demoteTimer,SIGNAL(timeout()),this,SLOT(autoDemoteAllNicks()) );
    demoteTimer->start(5*60*1000,false);

}

KonversationApplication::~KonversationApplication()
{
    saveOptions(false);

    delete m_images;
    delete dcopObject;
    //delete prefsDCOP;
    delete identDCOP;
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
        // make sure all vars are initialized properly
        quickConnectDialog = 0;
        colorOffSet = 0;

        // Sound object used to play sound...
        m_sound = new Konversation::Sound(this);

        // initialize OSD display here, so we can read the Preferences::properly
        osd = new OSDWidget( "Konversation" );

        Preferences::self();
        readOptions();

        // Images object providing LEDs, NickIcons
        m_images = new Images();

        // Auto-alias scripts
        QStringList scripts = KGlobal::dirs()->findAllResources("data","konversation/scripts/*");
        QFileInfo* fileInfo = new QFileInfo();
        QStringList aliasList(Preferences::aliasList());
        QString newAlias;

        for ( QStringList::ConstIterator it = scripts.begin(); it != scripts.end(); ++it )
        {
            fileInfo->setFile( *it );
            if ( fileInfo->isExecutable() )
            {
                newAlias = (*it).section('/',-1)+" "+"/exec "+(*it).section('/', -1 );

                if(!aliasList.contains(newAlias))
                    aliasList.append(newAlias);
            }
        }

        Preferences::setAliasList(aliasList);

        // Setup system codec
        // TODO: check if this works now as intended
        //    QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());

        // open main window
        mainWindow = new KonversationMainWindow();
        setMainWidget(mainWindow);

	connect(mainWindow,SIGNAL (showQuickConnectDialog()), this, SLOT (openQuickConnectDialog()) );
        connect(Preferences::self(), SIGNAL (updateTrayIcon()),mainWindow,SLOT (updateTrayIcon()) );
        connect(this, SIGNAL (prefsChanged()), mainWindow, SLOT (slotPrefsChanged()) );

        // apply GUI settings
        mainWindow->appearanceChanged();
        mainWindow->show();

        if(Preferences::showServerList())
        {
            mainWindow->openServerList();
        }

        // handle autoconnect on startup
        Konversation::ServerGroupList serverGroups = Preferences::serverGroupList();

        if(!m_connectDelayed)
        {
            for(Konversation::ServerGroupList::iterator it = serverGroups.begin(); it != serverGroups.end(); ++it)
            {
                if((*it)->autoConnectEnabled())
                {
                    connectToServer((*it)->id());
                }
            }
        }
        else
            quickConnectToServer(m_hostName, m_port, m_channel, m_nick, m_password, m_useSSL);

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
            connect(dcopObject,SIGNAL (dcopInsertRememberLine()),
                this,SLOT (insertRememberLine()));
            connect(dcopObject,SIGNAL (dcopSetAutoAway()),
                this,SLOT (setAutoAway()));
            connect(dcopObject,SIGNAL(dcopConnectToServer(const QString&, int,const QString&, const QString&)),
                this,SLOT(dcopConnectToServer(const QString&, int,const QString&, const QString&)));
        }

        // take care of user style changes, setting back colors and stuff
        connect(KApplication::kApplication(),SIGNAL (appearanceChanged()),mainWindow,SLOT (appearanceChanged()) );

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

void KonversationApplication::setAutoAway()
{
    Server* lookServer=serverList.first();
    while(lookServer)
    {
        if(!lookServer->isAway())
        {
            lookServer->setAutoAway();
        }
        lookServer=serverList.next();
    }

}

void KonversationApplication::toggleAway()
{
    kdDebug() << "toggleAway()" << endl;

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
    if(alreadyaway)
    {
                                                  //toggle as not away
        sendMultiServerCommand("away", QString::null);
    }
    else
    {
        QString awaymessage ;                     //get default awaymessage
        if(awaymessage.isEmpty()) awaymessage = "Away at the moment";

        sendMultiServerCommand("away", awaymessage);
    }
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
            //                  cpu cycles, anyways): i'm connected to two bouncers at the same time, which are
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
    mainWindow->appendToFrontmost(i18n("DCOP"), string, 0);
}

void KonversationApplication::insertRememberLine()
{
    kdDebug() << "insert remember line in konversationApplication()" << endl;
    mainWindow->insertRememberLine();
}

Server* KonversationApplication::connectToServerGroup(const QString& serverGroup)
{
    int serverGroupId = Preferences::serverGroupIdByName(serverGroup);

    return connectToServer(serverGroupId);
}

Server* KonversationApplication::connectToServer(int id)
{
    // Check if a server window with same name and port is already open
    Server* lookServer = serverList.first();

    while(lookServer)
    {
      if(lookServer->serverGroupSettings()->id() == id)
        return lookServer;

      lookServer = serverList.next();
    }

    Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(id);
    IdentityPtr identity = serverGroup->identity();

    if(!identity)
    {
        return 0;
    }

    // sanity check for identity
    QString check;

    if(identity->getIdent().isEmpty())
    {
        check+=i18n("Please fill in your <b>ident</b>.<br>");
    }
    if(identity->getRealName().isEmpty())
    {
        check+=i18n("Please fill in your <b>Real name</b>.<br>");
    }
    if(identity->getNickname(0).isEmpty())
    {
        check+=i18n("Please provide at least one <b>Nickname</b>.<br>");
    }
    if(!check.isEmpty())
    {
        KMessageBox::information(0,
            i18n("<qt>Your identity \"%1\" is not set up correctly:<br>%2</qt>")
            .arg(identity->getName()).arg(check),
            i18n("Check Identity Settings")
            );

        mainWindow->openIdentitiesDialog();

        return 0;
    }

    // identity ok, carry on

    mainWindow->show();

    Server* newServer = new Server(mainWindow, id);

    connect(mainWindow,SIGNAL (startNotifyTimer(int)),newServer,SLOT (startNotifyTimer(int)) );
    connect(mainWindow,SIGNAL (quitServer()),newServer,SLOT (quitServer()) );
    connect(newServer, SIGNAL(connectionChangedState(Server*, Server::State)),
        mainWindow, SLOT(serverStateChanged(Server*, Server::State)));

    connect(newServer,SIGNAL (nicksNowOnline(Server*,const QStringList&,bool)),mainWindow,SLOT (setOnlineList(Server*,const QStringList&,bool)) );

    connect(newServer,SIGNAL (deleted(Server*)),this,SLOT (removeServer(Server*)) );

    connect(newServer, SIGNAL(multiServerCommand(const QString&, const QString&)),
        this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
    connect(newServer, SIGNAL(awayInsertRememberLine()), this, SLOT(insertRememberLine()));

    serverList.append(newServer);

    return newServer;
}

void KonversationApplication::quickConnectToServer(const QString& hostName, const QString& port, const QString& channel, const QString& nick, const QString& password, const bool& useSSL)
{
    //used for the quick connect dialog and /server command

    Server* newServer = new Server(mainWindow, hostName, port, channel, nick, password, useSSL);

    connect(mainWindow,SIGNAL (startNotifyTimer(int)),newServer,SLOT (startNotifyTimer(int)) );
    connect(mainWindow,SIGNAL (quitServer()),newServer,SLOT (quitServer()) );

    connect(newServer,SIGNAL (nicksNowOnline(Server*,const QStringList&,bool)),mainWindow,SLOT (setOnlineList(Server*,const QStringList&,bool)) );

    connect(newServer,SIGNAL (deleted(Server*)),this,SLOT (removeServer(Server*)) );

    connect(newServer, SIGNAL(multiServerCommand(const QString&, const QString&)),
        this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
    connect(newServer, SIGNAL(awayInsertRememberLine()), this, SLOT(insertRememberLine()));

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

void KonversationApplication::removeServer(Server* server)
{
    serverList.setAutoDelete(false);              // don't delete items when they are removed
    if(!serverList.remove(server))
        kdDebug() << "Could not remove " << server->getServerName() << endl;
}

void KonversationApplication::quitKonversation()
{
    qApp->quit();
}

void KonversationApplication::readOptions()
{
    // get standard config file
    KConfig* config=kapp->config();

    // Identity list
    QStringList identityList=config->groupList().grep(QRegExp("Identity [0-9]+"));
    if(identityList.count())
    {
        Preferences::clearIdentityList();

        for(unsigned int index=0;index<identityList.count();index++)
        {
            IdentityPtr newIdentity=new Identity();

            config->setGroup(identityList[index]);

            QString n=config->readEntry("Name");

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

        }                                         // endfor

    }
    else
    {
        // Default user identity for pre 0.10 Preferences::files
        config->setGroup("User Identity");
        Preferences::setIdent(config->readEntry("Ident",Preferences::ident()));
        Preferences::setRealName(config->readEntry("Realname",Preferences::realName()));

        QString nickList=config->readEntry("Nicknames",Preferences::nicknameList().join(","));
        Preferences::setNicknameList(QStringList::split(",",nickList));

        Preferences::setShowAwayMessage(config->readBoolEntry("ShowAwayMessage",Preferences::showAwayMessage()));
        Preferences::setAwayMessage(config->readEntry("AwayMessage",Preferences::awayMessage()));
        Preferences::setUnAwayMessage(config->readEntry("UnAwayMessage",Preferences::unAwayMessage()));

        config->deleteGroup("User Identity");
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

    if(config->hasKey(QString("Server0")))
    {
        int index=0;
        Konversation::ServerGroupList serverGroups;

        // Read all old server settings
        while(config->hasKey(QString("Server%1").arg(index)))
        {
            QString serverStr = config->readEntry(QString("Server%1").arg(index++));

            Konversation::ServerGroupSettingsPtr serverGroup = new Konversation::ServerGroupSettings;
            QStringList tmp = QStringList::split(',', serverStr, true);
            serverGroup->setName(tmp[1]);
            serverGroup->setGroup(tmp[0]);
            Konversation::ServerSettings server;
            server.setServer(tmp[1]);
            server.setPort(tmp[2].toInt());
            server.setPassword(tmp[3]);
            serverGroup->addServer(server);
            serverGroup->setIdentityId(Preferences::identityByName(tmp[7])->id());
            serverGroup->setAutoConnectEnabled(tmp[6].toInt());
            serverGroup->setConnectCommands(tmp[8]);

            if(!tmp[4].isEmpty())
            {
                QStringList tmp2 = QStringList::split(" ", tmp[4], false);
                QStringList tmp3 = QStringList::split(" ", tmp[5], true);
                for(uint i = 0; i < tmp2.count(); i++)
                {
                    Konversation::ChannelSettings channel;
                    channel.setName(tmp2[i]);

                    if(i < tmp3.count())
                    {
                        channel.setPassword(tmp3[i]);
                    }

                    serverGroup->addChannel(channel);
                }
            }

            serverGroups.append(serverGroup);
        }

        Preferences::setServerGroupList(serverGroups);
    }
    else
    {
        // Read the new server settings
        QStringList groups = config->groupList().grep(QRegExp("ServerGroup [0-9]+"));

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
                serverGroup->setGroup(config->readEntry("Group"));
                serverGroup->setIdentityId(Preferences::identityByName(config->readEntry("Identity"))->id());
                serverGroup->setConnectCommands(config->readEntry("ConnectCommands"));
                serverGroup->setAutoConnectEnabled(config->readBoolEntry("AutoConnect"));
                serverGroup->setNotificationsEnabled(config->readBoolEntry("EnableNotifications", true));
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
    }

    // Notify Settings and lists.  Must follow Server List.
    Preferences::setNotifyDelay(Preferences::notifyDelay());
    Preferences::setUseNotify(Preferences::useNotify());
    Preferences::setNotifyList(Preferences::notifyList());

    // Quick Buttons List
    config->setGroup("Button List");
    // Read all buttons and overwrite default entries
    QStringList buttonList(Preferences::buttonList());
    for(int index=0;index<8;index++)
    {
        QString buttonKey(QString("Button%1").arg(index));
        if(config->hasKey(buttonKey)) buttonList[index]=config->readEntry(buttonKey);
    }
    // Put back the changed button list
    Preferences::setButtonList(buttonList);

    // Highlight List
    if(config->hasKey("Highlight"))               // Stay compatible with versions < 0.14
    {
        QString highlight=config->readEntry("Highlight");
        QStringList hiList=QStringList::split(' ',highlight);

        unsigned int hiIndex;
        for(hiIndex=0;hiIndex<hiList.count();hiIndex+=2)
        {
            Preferences::addHighlight(hiList[hiIndex],false,"#"+hiList[hiIndex+1],QString::null,QString::null);
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
    int index=0;
    while(config->hasKey(QString("Ignore%1").arg(index)))
    {
        Preferences::addIgnore(config->readEntry(QString("Ignore%1").arg(index++)));
    }

    // Aliases
    config->setGroup("Aliases");
    QStringList newList=config->readListEntry("AliasList");
    if(!newList.isEmpty()) Preferences::setAliasList(newList);

    // Web Browser
    config->setGroup("Web Browser Settings");
    Preferences::setWebBrowserUseKdeDefault(config->readBoolEntry("UseKdeDefault",Preferences::webBrowserUseKdeDefault()));
    Preferences::setWebBrowserCmd(config->readEntry("WebBrowserCmd",Preferences::webBrowserCmd()));

    // Channel Encodings
    QMap<QString,QString> channelEncodingsEntry=config->entryMap("Channel Encodings");
    QRegExp re("^(.+) ([^\\s]+)$");
    QStringList channelEncodingsEntryKeys=channelEncodingsEntry.keys();
    for(unsigned int i=0; i<channelEncodingsEntry.count(); ++i)
        if(re.search(channelEncodingsEntryKeys[i]) > -1)
            Preferences::setChannelEncoding(re.cap(1),re.cap(2),channelEncodingsEntry[channelEncodingsEntryKeys[i]]);

}

void KonversationApplication::saveOptions(bool updateGUI)
{
    KConfig* config=kapp->config();

    config->setGroup("Sort Nicknames");

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

    config->setGroup("Notify List");

    config->deleteGroup("Notify Group Lists");
    config->setGroup("Notify Group Lists");
    QMap<QString, QStringList> notifyList = Preferences::notifyList();
    QMapConstIterator<QString, QStringList> groupItEnd = notifyList.constEnd();

    for (QMapConstIterator<QString, QStringList> groupIt = notifyList.constBegin();
        groupIt != groupItEnd; ++groupIt)
    {
        config->writeEntry(groupIt.key(), groupIt.data().join(" "));
    }

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

        config->setGroup(QString("ServerGroup %1").arg(index));
        config->writeEntry("Name", (*it)->name());
        config->writeEntry("Group", (*it)->group());
        config->writeEntry("Identity", (*it)->identity()->getName());
        config->writeEntry("ServerList", servers);
        config->writeEntry("AutoJoinChannels", channels);
        config->writeEntry("ConnectCommands", (*it)->connectCommands());
        config->writeEntry("AutoConnect", (*it)->autoConnectEnabled());
        config->writeEntry("ChannelHistory", channelHistory);
        config->writeEntry("EnableNotifications", (*it)->enableNotifications());
        index++;
    }

    config->deleteGroup("Server List");

    config->setGroup("Button List");

    for(index=0;index<8;index++)
    {
        QStringList buttonList(Preferences::buttonList());
        config->writeEntry(QString("Button%1").arg(index),buttonList[index]);
    }

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
                config->writeEntry(channelEncodingsServerList[i]+" "+channelEncodingsChannelList[j],Preferences::channelEncoding(channelEncodingsServerList[i],channelEncodingsChannelList[j]));
    }

    config->sync();
    emit prefsChanged();

    if(updateGUI) 
      mainWindow->appearanceChanged();
}

void KonversationApplication::updateNickIcons()
{
    Server* lookServer=serverList.first();

    while(lookServer)
    {
        QPtrList<Channel> channelList = lookServer->getChannelList();
        Channel* channel=channelList.first();
        while(channel)
        {
            channel->getNickListView()->refresh();
            channel=channelList.next();
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
    urlList.append(who+" "+url);
    emit catchUrl(who,url);
}

const QStringList& KonversationApplication::getUrlList()
{
    return urlList;
}

void KonversationApplication::deleteUrl(const QString& who,const QString& url)
{
    urlList.remove(who+" "+url);
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

bool KonversationApplication::emitDCOPSig(const QString &appId, const QString &objId, const QString &signal, QByteArray &data)
{
    kdDebug() << "emitDCOPSig (" << signal << ")" << endl;
    //dcopObject->emitDCOPSignal(signal, data);
    QByteArray replyData;
    QCString replyType;
    if (!KApplication::dcopClient()->call(appId.ascii(), objId.ascii(), signal.ascii() /*must have prototype*/,
        data, replyType, replyData))
    {
        kdDebug() << "There was some error using DCOP." << endl;
        return true;                              // Keep processing filters
    }
    else
    {
        QDataStream reply(replyData, IO_ReadOnly);
        if (replyType == "bool")
        {
            bool result;
            reply >> result;
            return result;
        }
        else
        {
            kdDebug() << "doIt returned an unexpected type of reply!" << endl;
            return true;                          // Keep processing
        }
    }
}

QPtrList<IRCEvent> KonversationApplication::retrieveHooks (EVENT_TYPE a_type)
{
    QPtrList<IRCEvent> ret_value;
    IRCEvent *e;

    for (e = dcopObject->registered_events.first(); e; e = dcopObject->registered_events.next())
    {
        if (e->type == a_type)
        {
            ret_value.append(e);
        }
    }
    return ret_value;
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

uint& KonversationApplication::getColorOffset()
{
    return colorOffSet;
}

QMap<QString,QString>& KonversationApplication::getColorMap()
{
    return colorMap;
}

uint KonversationApplication::getKarma(const QString& nick) const
{
    return karmaMap[nick];
}

void KonversationApplication::increaseKarma(const QString& nick, uint increase)
{
    if(!m_demoteInProgress)
        karmaMap[nick] += increase;

    //kdDebug() << "New karma for " << nick << " is " << karmaMap[nick] << endl;
}

void KonversationApplication::decreaseKarma(const QString& nick)
{
    if(karmaMap[nick] > 0)
        karmaMap[nick] -= 1;

    //kdDebug() << "New karma for " << nick << " is " << karmaMap[nick] << endl;
}

void KonversationApplication::autoDemoteAllNicks()
{
    m_demoteInProgress = true;

    for(QMap<QString,uint>::Iterator it = karmaMap.begin(); it != karmaMap.end(); ++it)
    {
        decreaseKarma(it.key());
    }

    m_demoteInProgress = false;
}

void KonversationApplication::splitNick_Server(QString nick_server, QString &ircnick, QString &serverOrGroup)
{
    //kaddresbook uses the utf seperator 0xE120, so treat that as a seperator as well
    nick_server = nick_server.replace(QChar(0xE120), "@");
    ircnick = nick_server.section("@",0,0);
    serverOrGroup = nick_server.section("@",1);
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

#include "konversationapplication.moc"

// vim: set et sw=4 ts=4 cino=l1,cs,U1:
