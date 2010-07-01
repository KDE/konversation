// -*- mode: c++; c-file-style: "bsd"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  shows a user tree of friends per server
  begin:     Sam Aug 31 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "nicksonline.h"
#include <QHelpEvent>
#include "channel.h"
#include "server.h"
#include "application.h"
#include "connectionmanager.h"
#include "editnotifydialog.h"
#include "images.h"
#include "query.h"
#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"
#include "mainwindow.h"
#include "viewcontainer.h"
#include "nicksonlineitem.h"

#include <QInputDialog>
#include <QToolTip>
#include <QTreeWidget>

#include <KIconLoader>
#include <KToolBar>


NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
{
    setName(i18n("Watched Nicks Online"));
    setType(ChatWindow::NicksOnline);

    setSpacing(0);
    m_toolBar = new KToolBar(this, true, true);
    m_addNickname = m_toolBar->addAction(KIcon("list-add-user"), i18n("&Add Nickname..."));
    m_addNickname->setWhatsThis(i18n("Click to add a new nick to the list of nicknames that appear on this screen."));
    m_removeNickname = m_toolBar->addAction(KIcon("list-remove-user"), i18n("&Remove Nickname"));
    m_removeNickname->setWhatsThis(i18n("Click to remove a nick from the list of nicknames that appear on this screen."));
    m_toolBar->addSeparator();
    m_newContact = m_toolBar->addAction(KIcon("contact-new"), i18n("Create New C&ontact..."));
    m_editContact = m_toolBar->addAction(KIcon("document-edit"), i18n("Edit C&ontact..."));
    m_editContact->setWhatsThis(i18n("Click to create, view, or edit the KAddressBook entry associated with the nickname selected above."));
    m_toolBar->addSeparator();
    m_chooseAssociation = m_toolBar->addAction(KIcon("office-address-book"), i18n("&Choose Association..."));
    m_changeAssociation = m_toolBar->addAction(KIcon("office-address-book"), i18n("&Change Association..."));
    m_changeAssociation->setWhatsThis(i18n("Click to associate the nickname selected above with an entry in KAddressBook."));
    m_deleteAssociation = m_toolBar->addAction(KIcon("edit-delete"), i18n("&Delete Association"));
    m_deleteAssociation->setWhatsThis(i18n("Click to remove the association between the nickname selected above and a KAddressBook entry."));
    m_toolBar->addSeparator();
    m_sendMail = m_toolBar->addAction(KIcon("mail-send"), i18n("&Send Email..."));
    m_toolBar->addSeparator();
    m_whois = m_toolBar->addAction(KIcon("office-address-book"), i18n("&Whois"));
    m_openQuery = m_toolBar->addAction(KIcon("office-address-book"), i18n("Open &Query"));
    m_toolBar->addSeparator();
    m_joinChannel = m_toolBar->addAction(KIcon("irc-join-channel"), i18n("&Join Channel"));
    connect(m_toolBar, SIGNAL(actionTriggered(QAction*)), this, SLOT(slotPopupMenu_Activated(QAction*)));

    m_nickListView=new QTreeWidget(this);

    // Set to false every 8 seconds to permit a whois on watched nicks lacking information.
    // Remove when server or addressbook does this automatically.
    m_whoisRequested = true;

    m_kabcIconSet = KIcon("office-address-book");
    m_onlineIcon = KIcon("im-user");
    m_offlineIcon = KIcon("im-user-offline");
    m_nickListView->setColumnCount(2);
    m_nickListView->headerItem()->setText(0, i18n("Network/Nickname/Channel"));
    m_nickListView->headerItem()->setText(1, i18n("Additional Information"));
    m_nickListView->setRootIsDecorated(true);
    m_nickListView->setSortingEnabled(true);

    Preferences::restoreColumnState(m_nickListView, "NicksOnline ViewSettings");

    QString nickListViewWT = i18n(
        "<p>These are all the nicknames on your Nickname Watch list, listed under the "
        "server network they are connected to.  The list also includes the nicknames "
        "in KAddressBook associated with the server network.</p>"
        "<p>The <b>Additional Information</b> column shows the information known "
        "for each nickname.</p>"
        "<p>The channels the nickname has joined are listed underneath each nickname.</p>"
        "<p>Nicknames appearing under <b>Offline</b> are not connected to any of the "
        "servers in the network.</p>"
        "<p>Right-click with the mouse on a nickname to perform additional functions.</p>");
    m_nickListView->setWhatsThis(nickListViewWT);
    m_nickListView->viewport()->installEventFilter(this);

    connect(m_nickListView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
        this,SLOT(processDoubleClick(QTreeWidgetItem*,int)));

    setupToolbarActions(0);

    // Create context menu.
    m_popupMenu = new KMenu(this);
    m_popupMenu->setObjectName("nicksonline_context_menu");
    m_nickListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_nickListView, SIGNAL(customContextMenuRequested(QPoint)),
        this, SLOT(slotCustomContextMenuRequested(QPoint)));
    connect(m_nickListView, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotNickListView_SelectionChanged()));

    // Display info for all currently-connected servers.
    refreshAllServerOnlineLists();

    // Connect and start refresh timer.
    m_timer = new QTimer(this);
    m_timer->setObjectName("nicksOnlineTimer");
    connect(m_timer, SIGNAL (timeout()), this, SLOT(timerFired()));
    // TODO: User preference for refresh interval.
    m_timer->start(8000);
}

NicksOnline::~NicksOnline()
{
    Preferences::saveColumnState(m_nickListView, "NicksOnline ViewSettings");

    m_timer->stop();
    delete m_timer;
    delete m_nickListView;
}

bool NicksOnline::eventFilter(QObject*obj, QEvent* event )
{
    if( ( obj == m_nickListView->viewport() ) && ( event->type() == QEvent::ToolTip ) )
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>( event );

        QTreeWidgetItem *item = m_nickListView->itemAt( helpEvent->pos() );

        if( item )
        {
            NickInfoPtr nickInfo = getNickInfo(item);
            if ( nickInfo )
            {
               QString text =  nickInfo->tooltip();
               if( !text.isEmpty() )
                       QToolTip::showText( helpEvent->globalPos(), text );
               else
                       QToolTip::hideText();
            }
            else
	        QToolTip::hideText();
        }
        else
                QToolTip::hideText();
    }


    return ChatWindow::eventFilter( obj, event );
}

QTreeWidget* NicksOnline::getNickListView()
{
    return m_nickListView;
}

/**
 * Returns the named child of parent item in a NicksOnlineItem
 * @param parent            Pointer to a NicksOnlineItem.
 * @param name              The name in the desired child QListViewItem, must be in column 0.
 * @param type              The type of entry to be found
 * @return                  Pointer to the child QListViewItem or 0 if not found.
 */
QTreeWidgetItem* NicksOnline::findItemChild(const QTreeWidgetItem* parent, const QString& name, NicksOnlineItem::NickListViewColumn type)
{
    if (!parent) return 0;
    for (int i = 0; i < parent->childCount(); ++i)
    {
        QTreeWidgetItem* child = parent->child(i);
        if(static_cast<NicksOnlineItem*>(child)->type() == type && child->text(0) == name) return child;
    }
    return 0;
}

/**
 * Returns the first occurrence of a child item of a given type in a NicksOnlineItem
 * @param parent            Pointer to a NicksOnlineItem.
 * @param type              The type of entry to be found
 * @return                  Pointer to the child QListViewItem or 0 if not found.
 */
QTreeWidgetItem* NicksOnline::findItemType(const QTreeWidgetItem* parent, NicksOnlineItem::NickListViewColumn type)
{
    if (!parent) return 0;
    for (int i = 0; i < parent->childCount(); ++i)
    {
        QTreeWidgetItem* child = parent->child(i);
        if(static_cast<NicksOnlineItem*>(child)->type() == type) return child;
    }
    return 0;
}

/**
 * Returns a pointer to the network QListViewItem with the given name.
 * @param name              The name of the network.
 * @return                  Pointer to the QListViewItem or 0 if not found.
 */
QTreeWidgetItem* NicksOnline::findNetworkRoot(int serverGroupId)
{
    for (int i = 0; i < m_nickListView->invisibleRootItem()->childCount(); ++i)
    {
        QTreeWidgetItem* child = m_nickListView->invisibleRootItem()->child(i);

        if (child->data(0, Qt::UserRole).toInt() == serverGroupId)
            return child;
    }

    return 0;
}

/**
 * Return a string containing formatted additional information about a nick.
 * @param nickInfo          A pointer to NickInfo structure for the nick.  May be Null.
 * @param addressee         Addressbook entry for the nick.  May be empty.
 * @return                  A string formatted for display containing the information
 *                          about the nick.
 * @return needWhois        True if a WHOIS needs to be performed on the nick
 *                          to get additional information.
 */
QString NicksOnline::getNickAdditionalInfo(NickInfoPtr nickInfo, KABC::Addressee addressee,
bool& needWhois)
{
    QString info;
    if (!addressee.isEmpty())
    {
        if (addressee.fullEmail().isEmpty())
            info += addressee.realName();
        else
            info += addressee.fullEmail();
    }
    QString niInfo;
    if (nickInfo)
    {
        if (nickInfo->isAway())
        {
            niInfo += i18n("Away");
            if (!nickInfo->getAwayMessage().isEmpty())
                niInfo += '(' + nickInfo->getAwayMessage() + ')';
        }
        if (!nickInfo->getHostmask().isEmpty())
            niInfo += ' ' + nickInfo->getHostmask();
        if (!nickInfo->getRealName().isEmpty())
            niInfo += " (" + nickInfo->getRealName() + ')';
        if (!nickInfo->getNetServer().isEmpty())
        {
            niInfo += i18n( " online via %1", nickInfo->getNetServer() );
            if (!nickInfo->getNetServerInfo().isEmpty())
                niInfo += " (" + nickInfo->getNetServerInfo() + ')';
        }
        if (!nickInfo->getOnlineSince().isNull())
            niInfo += i18n( " since %1", nickInfo->getPrettyOnlineSince() );
    }
    needWhois = niInfo.isEmpty();
    if (!info.isEmpty() && !needWhois) info += ' ';
    return info + niInfo;
}

/**
 * Refresh the nicklistview for a single server.
 * @param server            The server to be refreshed.
 */
void NicksOnline::updateServerOnlineList(Server* servr)
{
    // Return if connection is an ephemeral one, because
    // we cant watch them anyway.
    if (!servr->getServerGroup())
        return;

    bool newNetworkRoot = false;
    QString serverName = servr->getServerName();
    QString networkName = servr->getDisplayName();
    QTreeWidgetItem* networkRoot = findNetworkRoot(servr->getServerGroup()->id());
    // If network is not in our list, add it.
    if (!networkRoot)
    {
        networkRoot = new NicksOnlineItem(NicksOnlineItem::NetworkRootItem,m_nickListView,networkName);
        networkRoot->setData(0, Qt::UserRole, servr->getServerGroup()->id());
        newNetworkRoot = true;
    }
    // Store server name in hidden column.
    // Note that there could be more than one server in the network connected,
    // but it doesn't matter because all the servers in a network have the same
    // watch list.
    networkRoot->setText(nlvcServerName, serverName);
    // Update list of servers in the network that are connected.
    QStringList serverList = networkRoot->text(nlvcAdditionalInfo).split(',', QString::SkipEmptyParts);
    if (!serverList.contains(serverName)) serverList.append(serverName);
    networkRoot->setText(nlvcAdditionalInfo, serverList.join(","));
    // Get watch list.
    QStringList watchList = servr->getWatchList();
    QStringList::iterator itEnd = watchList.end();
    QString nickname;

    for (QStringList::iterator it = watchList.begin(); it != itEnd; ++it)
    {
        nickname = (*it);
        NickInfoPtr nickInfo = getOnlineNickInfo(networkName, nickname);

        if (nickInfo && nickInfo->getPrintedOnline())
        {
            // Nick is online.
            // Which server did NickInfo come from?
            Server* server=nickInfo->getServer();
            // Get addressbook entry (if any) for the nick.
            KABC::Addressee addressee = nickInfo->getAddressee();
            // Construct additional information string for nick.
            bool needWhois = false;
            QString nickAdditionalInfo = getNickAdditionalInfo(nickInfo, addressee, needWhois);
            // Add to network if not already added.
            QTreeWidgetItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
            if (!nickRoot)
                nickRoot = new NicksOnlineItem(NicksOnlineItem::NicknameItem, networkRoot, nickname, nickAdditionalInfo);
            NicksOnlineItem* nickitem = static_cast<NicksOnlineItem*>(nickRoot);
            nickitem->setConnectionId(server->connectionId ());
            // Mark nick as online
            nickitem->setOffline(false);
            // Update icon
            nickitem->setIcon(nlvcNick, m_onlineIcon);
            nickRoot->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            nickRoot->setText(nlvcServerName, serverName);
            // If no additional info available, request a WHOIS on the nick.
            if (!m_whoisRequested)
            {
                if (needWhois)
                {
                    requestWhois(networkName, nickname);
                    m_whoisRequested = true;
                }
            }
            // Set Kabc icon if the nick is associated with an addressbook entry.
            if (!addressee.isEmpty())
                nickRoot->setIcon(nlvcKabc, QIcon(m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Normal, QIcon::On)));
            else
                nickRoot->setIcon(nlvcKabc, QIcon(m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Disabled, QIcon::Off)));

            QStringList channelList = server->getNickChannels(nickname);
            QStringList::iterator itEnd2 = channelList.end();

            for (QStringList::iterator it2 = channelList.begin(); it2 != itEnd2; ++it2)
            {
                // Known channels where nickname is online and mode in each channel.
                // FIXME: If user connects to multiple servers in same network, the
                // channel info will differ between the servers, resulting in inaccurate
                // mode and led info displayed.

                QString channelName = (*it2);

                ChannelNickPtr channelNick = server->getChannelNick(channelName, nickname);
                QString nickMode;
                if (channelNick->hasVoice()) nickMode = nickMode + i18n(" Voice");
                if (channelNick->isHalfOp()) nickMode = nickMode + i18n(" HalfOp");
                if (channelNick->isOp()) nickMode = nickMode + i18n(" Operator");
                if (channelNick->isOwner()) nickMode = nickMode + i18n(" Owner");
                if (channelNick->isAdmin()) nickMode = nickMode + i18n(" Admin");
                QTreeWidgetItem* channelItem = findItemChild(nickRoot, channelName, NicksOnlineItem::ChannelItem);
                if (!channelItem) channelItem = new NicksOnlineItem(NicksOnlineItem::ChannelItem,nickRoot,
                        channelName, nickMode);
                channelItem->setText(nlvcAdditionalInfo, nickMode);

                // Icon for mode of nick in each channel.
                Images::NickPrivilege nickPrivilege = Images::Normal;
                if (channelNick->hasVoice()) nickPrivilege = Images::Voice;
                if (channelNick->isHalfOp()) nickPrivilege = Images::HalfOp;
                if (channelNick->isOp()) nickPrivilege = Images::Op;
                if (channelNick->isOwner()) nickPrivilege = Images::Owner;
                if (channelNick->isAdmin()) nickPrivilege = Images::Admin;
                if (server->getJoinedChannelMembers(channelName) != 0)
                    channelItem->setIcon(nlvcChannel,
                        QIcon(Application::instance()->images()->getNickIcon(nickPrivilege, false)));
                else
                    channelItem->setIcon(nlvcChannel,
                        QIcon(Application::instance()->images()->getNickIcon(nickPrivilege, true)));
            }
            // Remove channel if nick no longer in it.
            for (int i = 0; i < nickRoot->childCount(); ++i)
            {
                QTreeWidgetItem* child = nickRoot->child(i);
                if (!channelList.contains(child->text(nlvcNick)))
                {
                    delete nickRoot->takeChild(i);
                    i--;
                }
            }
        }
        else
        {
            // Nick is offline.
            QTreeWidgetItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
            if (!nickRoot)
                nickRoot = new NicksOnlineItem(NicksOnlineItem::NicknameItem, networkRoot, nickname);
            // remove channels from the nick
            qDeleteAll(nickRoot->takeChildren());
            NicksOnlineItem* nickitem = static_cast<NicksOnlineItem*>(nickRoot);
            nickitem->setConnectionId(servr->connectionId ());
            // Mark nick as offline
            nickitem->setOffline (true);
            // Update icon
            nickitem->setIcon(nlvcNick, m_offlineIcon);
            nickRoot->setText(nlvcServerName, serverName);
            // Get addressbook entry for the nick.
            KABC::Addressee addressee = servr->getOfflineNickAddressee(nickname);
            // Format additional information for the nick.
            bool needWhois = false;
            QString nickAdditionalInfo = getNickAdditionalInfo(NickInfoPtr(), addressee, needWhois);
            nickRoot->setText(nlvcAdditionalInfo, i18nc("(Offline) nickname details (e.g. real name from address book)",
                "(Offline) %1", nickAdditionalInfo));
            // Set Kabc icon if the nick is associated with an addressbook entry.
            if (!addressee.isEmpty())
                nickRoot->setIcon(nlvcKabc, QIcon(m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Normal, QIcon::On)));
            else
                nickRoot->setIcon(nlvcKabc, QIcon(m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Disabled, QIcon::Off)));
        }
    }
    // Erase nicks no longer being watched.
    for (int i = 0; i < networkRoot->childCount(); ++i)
    {
        QTreeWidgetItem* item = networkRoot->child(i);
        QString nickname = item->text(nlvcNick);
        if (!watchList.contains(nickname) && serverName == item->text(nlvcServerName))
        {
            delete networkRoot->takeChild(i);
            i--;
        }
    }
    // Expand server if newly added to list.
    if (newNetworkRoot)
    {
        networkRoot->setExpanded(true);
        // Connect server NickInfo updates.
        connect (servr, SIGNAL(nickInfoChanged(Server*, const NickInfoPtr)),
            this, SLOT(slotNickInfoChanged(Server*, const NickInfoPtr)));
    }
}

/**
 * Determines if a nick is online in any of the servers in a network and returns
 * a NickInfo if found, otherwise 0.
 * @param networkName        Server network name.
 * @param nickname           Nick name.
 * @return                   NickInfo if nick is online in any server, otherwise 0.
 */
NickInfoPtr NicksOnline::getOnlineNickInfo(QString& networkName, QString& nickname)
{
    // Get list of pointers to all servers.
    Application* konvApp = static_cast<Application*>(kapp);
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();
    foreach (Server* server, serverList)
    {
        if (server->getDisplayName() == networkName)
        {
            NickInfoPtr nickInfo = server->getNickInfo(nickname);
            if (nickInfo) return nickInfo;
        }
    }
    return NickInfoPtr(); //TODO FIXME NULL NULL NULL
}

/**
 * Requests a WHOIS for a specified server network and nickname.
 * The request is sent to the first server found in the network.
 * @param groupName          Server group name.
 * @param nickname           Nick name.
 */
void NicksOnline::requestWhois(QString& networkName, QString& nickname)
{
    Application* konvApp = static_cast<Application*>(kapp);
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();
    foreach (Server* server, serverList)
    {
        if (server->getDisplayName() == networkName)
        {
            server->requestWhois(nickname);
            return;
        }
    }
}

/**
 * Updates the notify list based on the current state of the tree
 */
void NicksOnline::updateNotifyList()
{
  // notify list
  QMap<int, QStringList> notifyList;
  // fill in the notify list
  for (int i = 0; i < m_nickListView->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* networkRoot = m_nickListView->topLevelItem(i);
    // nick list for this network root
    QStringList nicks;
    for (int j = 0; j < networkRoot->childCount(); ++j)
    {
      NicksOnlineItem *item = dynamic_cast<NicksOnlineItem*>(networkRoot->child(j));
      if (item->type() == NicksOnlineItem::NicknameItem)
      {
        // add the nick to the list
        nicks << item->text(nlvcNick);
      }
    }
    // insert nick list to the notify list
    notifyList.insert(networkRoot->data(0, Qt::UserRole).toInt(), nicks);
  }
  // update notify list
  Preferences::setNotifyList(notifyList);
  // save notify list
  static_cast<Application*>(kapp)->saveOptions(false);
}

/**
 * Refresh the nicklistview for all servers.
 */
void NicksOnline::refreshAllServerOnlineLists()
{
    Application* konvApp = static_cast<Application*>(kapp);
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();
    // Remove servers no longer connected.
    for (int i = 0; i < m_nickListView->invisibleRootItem()->childCount(); ++i)
    {
        QTreeWidgetItem *child = m_nickListView->invisibleRootItem()->child(i);
        QString networkName = child->text(nlvcNetwork);
        QStringList serverNameList = child->text(nlvcAdditionalInfo).split(',', QString::SkipEmptyParts);
        QStringList::Iterator itEnd = serverNameList.end();
        QStringList::Iterator it = serverNameList.begin();
        while (it != itEnd)
        {
            QString serverName = *it;
            // Locate server in server list.
            bool found = false;
            foreach (Server* server, serverList)
            {
                if ((server->getServerName() == serverName) &&
                    (server->getDisplayName() == networkName)) found = true;
            }
            if (!found)
                it = serverNameList.erase(it);
            else
                ++it;
        }
        // Remove Networks with no servers connected, otherwise update list of connected
        // servers.
        if (serverNameList.empty())
        {
            delete m_nickListView->invisibleRootItem()->takeChild(i);
            i--;
        }
        else
            child->setText(nlvcAdditionalInfo, serverNameList.join(","));
    }
    // Display info for all currently-connected servers.
    foreach (Server* server, serverList)
    {
        updateServerOnlineList(server);
    }
    // Refresh addressbook buttons.
    slotNickListView_SelectionChanged();
}

void NicksOnline::timerFired()
{
    // Allow one WHOIS request per cycle.
    m_whoisRequested = false;
    refreshAllServerOnlineLists();
}

/**
 * When a user double-clicks a nickname in the nicklistview, let server know so that
 * it can perform the user's chosen default action for that.
 */
void NicksOnline::processDoubleClick(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    NicksOnlineItem* nickitem = dynamic_cast<NicksOnlineItem*>(item);

    if (!nickitem || nickitem->isOffline())
        return;
    // Only emit signal when the user double clicked a nickname rather than
    // a server name or channel name.
    if (nickitem->type() == NicksOnlineItem::NicknameItem)
        emit doubleClicked(nickitem->connectionId(), nickitem->text(nlvcNick));
    if (nickitem->type() == NicksOnlineItem::ChannelItem)
    {
      NicksOnlineItem* nickRoot = dynamic_cast<NicksOnlineItem*>(nickitem->parent());
      Server* server = Application::instance()->getConnectionManager()->getServerByConnectionId(nickRoot->connectionId());
      ChatWindow* channel = server->getChannelByName(nickitem->text(nlvcChannel));

      if (channel)
        emit showView(channel);
      else
      {
        // Get the server object corresponding to the connection id.
        server->queue( "JOIN "+ nickitem->text(nlvcChannel) );
      }
    }
}

/**
 * Returns the server name and nickname of the specified nicklistview item.
 * @param item              The nicklistview item.
 * @return serverName       Name of the server for the nick at the item, or Null if not a nick.
 * @return nickname         The nickname at the item.
 */
bool NicksOnline::getItemServerAndNick(const QTreeWidgetItem* item, QString& serverName, QString& nickname)
{
    if (!item) return false;
    // convert into NicksOnlineItem
    const NicksOnlineItem* nlItem=static_cast<const NicksOnlineItem*>(item);
    // If on a network, return false;
    if (nlItem->type() == NicksOnlineItem::NetworkRootItem) return false;
    // get server name
    serverName = item->text(nlvcServerName);
    // If on a channel, move up to the nickname.
    if (nlItem->type() == NicksOnlineItem::ChannelItem)
    {
        item = item->parent();
        serverName = item->text(nlvcServerName);
    }
    nickname = item->text(nlvcNick);
    return true;
}

NickInfoPtr NicksOnline::getNickInfo(const QTreeWidgetItem* item)
{
    QString serverName;
    QString nickname;

    getItemServerAndNick(item, serverName, nickname);

    if (serverName.isEmpty() || nickname.isEmpty())
        return NickInfoPtr(); //TODO FIXME NULL NULL NULL

    Server* server = Application::instance()->getConnectionManager()->getServerByName(serverName);

    if (server)
        return server->getNickInfo(nickname);

    return NickInfoPtr(); //TODO FIXME NULL NULL NULL
}

/**
 * Given a server name and nickname, returns the item in the Nick List View displaying
 * the nick.
 * @param serverName        Name of server.
 * @param nickname          Nick name.
 * @return                  Pointer to QListViewItem displaying the nick, or 0 if not found.
 *
 * @see getItemServerAndNick
 */
QTreeWidgetItem* NicksOnline::getServerAndNickItem(const QString& serverName,
const QString& nickname)
{
    Server* server = Application::instance()->getConnectionManager()->getServerByName(serverName);
    if (!server) return 0;
    QString networkName = server->getDisplayName();
    QList<QTreeWidgetItem*> items = m_nickListView->findItems(networkName, Qt::MatchExactly | Qt::MatchCaseSensitive, nlvcNetwork);
    if (items.count() == 0) return 0;
    QTreeWidgetItem* networkRoot = items.at(0);
    if (!networkRoot) return 0;
    QTreeWidgetItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
    return nickRoot;
}

/**
 * Perform an addressbook command (edit contact, create new contact,
 * change/delete association.)
 * @param id                The command id.  @ref CommandIDs.
 *
 * The operation is performed on the nickname at the currently-selected item in
 * the nicklistview.
 *
 * Also refreshes the nicklistview display to reflect the new addressbook state
 * for the nick.
 */
void NicksOnline::doCommand(QAction* id)
{
    if(id == 0)
        return;
    if ( id == m_addNickname )
    {
        int serverGroupId = -1;

        if (m_nickListView->selectedItems().count())
        {
            NicksOnlineItem *networkRoot = dynamic_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0));
            if (networkRoot)
            {
                while (networkRoot->type() != NicksOnlineItem::NetworkRootItem)
                    networkRoot = dynamic_cast<NicksOnlineItem*>(networkRoot->parent());

                serverGroupId = networkRoot->data(0, Qt::UserRole).toInt();
            }
        }
        EditNotifyDialog *end = new EditNotifyDialog(this, serverGroupId);
        connect(end, SIGNAL(notifyChanged(int,QString)), this, SLOT(slotAddNickname(int,QString)));
        end->show();
        return;
    }

    QString serverName;
    QString nickname;
    if (m_nickListView->selectedItems().count() == 0) return;
    QTreeWidgetItem* item = m_nickListView->selectedItems().at(0);
    NicksOnlineItem* nickitem = dynamic_cast<NicksOnlineItem*>(item);

    if(!nickitem)
        return;

    if ( id == m_removeNickname )
    {
      // remove watch from the tree widget
      delete nickitem;
      // update notify list
      updateNotifyList();
      return;
    }

    if (!getItemServerAndNick(item, serverName, nickname))
      return;

    // Get the server object corresponding to the connection id.
    Server* server = Application::instance()->getConnectionManager()->getServerByConnectionId(nickitem->connectionId());

    if (!server) return;

    // Get NickInfo object corresponding to the nickname.
    NickInfoPtr nickInfo = server->getNickInfo(nickname);
    // Get addressbook entry for the nick.
    KABC::Addressee addressee;

    if(nickInfo)
    {
        addressee = nickInfo->getAddressee();
    }
    else
    {
        addressee = server->getOfflineNickAddressee(nickname);
    }
    if ( id == m_sendMail )
            Konversation::Addressbook::self()->sendEmail(addressee);
    else if (  id == m_editContact )
            Konversation::Addressbook::self()->editAddressee(addressee.uid());
    else if ( id == m_chooseAssociation || id == m_changeAssociation )
    {
        LinkAddressbookUI *linkaddressbookui = NULL;

        if(nickInfo)
        {
            linkaddressbookui = new LinkAddressbookUI(server->getViewContainer()->getWindow(), nickInfo->getNickname(), server->getServerName(), server->getDisplayName(), nickInfo->getRealName());
        }
        else
        {
            linkaddressbookui = new LinkAddressbookUI(server->getViewContainer()->getWindow(), nickname, server->getServerName(), server->getDisplayName(), addressee.realName());
        }

        linkaddressbookui->show();
    }
    else if ( id == m_newContact || id == m_deleteAssociation )
    {
            Konversation::Addressbook *addressbook = Konversation::Addressbook::self();

            if(addressbook && addressbook->getAndCheckTicket())
            {
                if(id == m_deleteAssociation)
                {
                    if (addressee.isEmpty())
                    {
                        return;
                    }

                    addressbook->unassociateNick(addressee, nickname, server->getServerName(), server->getDisplayName());
                }
                else
                {
                    addressee.setGivenName(nickname);
                    addressee.setNickName(nickname);
                    addressbook->associateNickAndUnassociateFromEveryoneElse(addressee, nickname, server->getServerName(), server->getDisplayName());
                }
                if(addressbook->saveTicket())
                {
                    //saveTicket will refresh the addressees for us.
                    if(id == m_newContact)
                    {
                        Konversation::Addressbook::self()->editAddressee(addressee.uid());
                    }
                }
            }
    }
    else if ( id == m_joinChannel )
    {
        if (m_nickListView->selectedItems().count() > 0)
        {
            // only join real channels
            if (static_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0))->type() == NicksOnlineItem::ChannelItem)
            {
                QString contactChannel = m_nickListView->selectedItems().at(0)->text(nlvcChannel);
                server->queue( "JOIN "+contactChannel );
            }
        }
    }
    else if ( id == m_whois )
    {
            server->queue("WHOIS "+nickname);
    }
    else if ( id == m_openQuery )
    {
            NickInfoPtr nickInfo = server->obtainNickInfo(nickname);
            class Query* query = server->addQuery(nickInfo, true /*we initiated*/);
            emit showView(query);
    }
    else
            refreshItem(item);
}

/**
 * Get the addressbook state of the nickname at the specified nicklistview item.
 * @param item              Item of the nicklistview.
 * @return                  Addressbook state.
 * 0 = not a nick, 1 = nick has no addressbook association, 2 = nick has association
 */
int NicksOnline::getNickAddressbookState(QTreeWidgetItem* item)
{
    int nickState = nsNotANick;
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
    {
        Server *server = Application::instance()->getConnectionManager()->getServerByName(serverName);
        if (!server) return nsNotANick;
        NickInfoPtr nickInfo = server->getNickInfo(nickname);
        if (nickInfo)
        {
            if (nickInfo->getAddressee().isEmpty())
                nickState = nsNoAddress;
            else
                nickState = nsHasAddress;
        }
        else
        {
            if (server->getOfflineNickAddressee(nickname).isEmpty())
                nickState = nsNoAddress;
            else
                nickState = nsHasAddress;
        }
    }
    return nickState;
}

/**
 * Sets up toolbar actions based on the given item.
 * @param item              Item of the nicklistview.
 */
void NicksOnline::setupToolbarActions(NicksOnlineItem *item)
{
  // disable all actions
  m_removeNickname->setEnabled(false);
  m_newContact->setEnabled(false);
  m_editContact->setEnabled(false);
  m_chooseAssociation->setEnabled(false);
  m_changeAssociation->setEnabled(false);
  m_deleteAssociation->setEnabled(false);
  m_whois->setEnabled(false);
  m_openQuery->setEnabled(false);
  m_sendMail->setEnabled(false);
  m_joinChannel->setEnabled(false);
  // check for null
  if (item == 0)
    return;
  // add items depending on the item type
  switch (item->type())
  {
  case NicksOnlineItem::ChannelItem:
    m_joinChannel->setEnabled(true);
    break;
  case NicksOnlineItem::NicknameItem:
    m_removeNickname->setEnabled(true);
    int nickState = getNickAddressbookState(item);
    if (nickState == nsNoAddress)
    {
      m_chooseAssociation->setEnabled(true);
      m_newContact->setEnabled(true);
    }
    else if (nickState == nsHasAddress)
    {
      m_changeAssociation->setEnabled(true);
      m_deleteAssociation->setEnabled(true);
      m_editContact->setEnabled(true);
      m_sendMail->setEnabled(true);
    }
    if (!item->isOffline())
    {
      m_whois->setEnabled(true);
      m_openQuery->setEnabled(true);
    }
    break;
  }
}

/**
 * Sets up popup menu actions based on the given item.
 * @param item              Item of the nicklistview.
 */
void NicksOnline::setupPopupMenuActions(NicksOnlineItem *item)
{
  // clear the popup menu
  m_popupMenu->clear();
  // check for null
  if (item == 0)
    return;
  // add items depending on the item type
  switch (item->type())
  {
  case NicksOnlineItem::NetworkRootItem:
    m_popupMenu->insertAction(0, m_addNickname);
    break;
  case NicksOnlineItem::ChannelItem:
    m_popupMenu->insertAction(0, m_joinChannel);
    break;
  case NicksOnlineItem::NicknameItem:
    m_popupMenu->insertAction(0, m_removeNickname);
    int nickState = getNickAddressbookState(item);
    if (nickState == nsNoAddress)
    {
      m_popupMenu->addSeparator();
      m_popupMenu->insertAction(0, m_newContact);
      m_popupMenu->addSeparator();
      m_popupMenu->insertAction(0, m_chooseAssociation);
    }
    else if (nickState == nsHasAddress)
    {
      m_popupMenu->addSeparator();
      m_popupMenu->insertAction(0, m_editContact);
      m_popupMenu->addSeparator();
      m_popupMenu->insertAction(0, m_changeAssociation);
      m_popupMenu->insertAction(0, m_deleteAssociation);
      m_popupMenu->addSeparator();
      m_popupMenu->insertAction(0, m_sendMail);
    }
    if (!item->isOffline())
    {
      m_popupMenu->addSeparator();
      m_popupMenu->insertAction(0, m_whois);
      m_popupMenu->insertAction(0, m_openQuery);
    }
    break;
  }
}

/**
 * Received when user selects a different item in the nicklistview.
 */
void NicksOnline::slotNickListView_SelectionChanged()
{
    if (m_nickListView->selectedItems().count() == 0)
      return;
    setupToolbarActions(dynamic_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0)));
}

/**
 * Received when right-clicking an item in the NickListView.
 */
void NicksOnline::slotCustomContextMenuRequested(QPoint point)
{
    QTreeWidgetItem *item = m_nickListView->itemAt(point);
    if (item == 0)
      return;
    // select the item
    item->setSelected(true);
    // set up actions
    setupPopupMenuActions(dynamic_cast<NicksOnlineItem*>(item));
    // show the popup menu
    if (m_popupMenu->actions().count() > 0)
      m_popupMenu->popup(QCursor::pos());
}

/**
 * Received from popup menu when user chooses something.
 */
void NicksOnline::slotPopupMenu_Activated(QAction* id)
{
    doCommand(id);
}

/**
 * Received from server when a NickInfo changes its information.
 */
void NicksOnline::slotNickInfoChanged(Server* server, const NickInfoPtr nickInfo)
{
    if (!nickInfo) return;
    QString nickname = nickInfo->getNickname();

    if (!server) return;
    QString serverName = server->getServerName();
    QTreeWidgetItem* item = getServerAndNickItem(serverName, nickname);
    refreshItem(item);
}

/**
 * Received when user added a new nick to the watched nicks.
 */
void NicksOnline::slotAddNickname(int serverGroupId, QString nickname)
{
    Preferences::addNotify(serverGroupId, nickname);
    static_cast<Application*>(kapp)->saveOptions(true);
}

/**
 * Refreshes the information for the given item in the list.
 * @param item               Pointer to listview item.
 */
void NicksOnline::refreshItem(QTreeWidgetItem* item)
{
    if (!item)
        return;
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
    {
        Server *server = Application::instance()->getConnectionManager()->getServerByName(serverName);
        if (server)
        {
            NickInfoPtr nickInfo = server->getNickInfo(nickname);
            KABC::Addressee addressee;
            int nickState = nsNoAddress;
            if (nickInfo)
                addressee = nickInfo->getAddressee();
            else
                addressee = server->getOfflineNickAddressee(nickname);
            if (!addressee.isEmpty())
                nickState = nsHasAddress;

            switch (nickState)
            {
                case nsNotANick:
                    break;
                case nsNoAddress:
                {
                    item->setIcon(nlvcKabc, QIcon(m_kabcIconSet.pixmap(
                        KIconLoader::Small, QIcon::Disabled, QIcon::Off)));
                    break;
                }
                case nsHasAddress:
                {
                    item->setIcon(nlvcKabc, QIcon(m_kabcIconSet.pixmap(
                        KIconLoader::Small, QIcon::Normal, QIcon::On))); break;
                }
            }
            QString nickAdditionalInfo;
            bool needWhois = false;
            if (nickInfo)
                nickAdditionalInfo = getNickAdditionalInfo(nickInfo, addressee, needWhois);
            item->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            if (m_nickListView->selectedItems().count() != 0 && item == m_nickListView->selectedItems().at(0))
                setupToolbarActions(dynamic_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0)));
        }
    }
}

void NicksOnline::childAdjustFocus() {}

#include "nicksonline.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
