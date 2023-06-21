// -*- mode: c++; c-file-style: "bsd"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
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
#include "mainwindow.h"
#include "viewcontainer.h"

#include <QToolTip>
#include <QTreeWidget>

#include <KToolBar>


NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
{
    setName(i18n("Watched Nicks Online"));
    setType(ChatWindow::NicksOnline);

    setSpacing(0);
    m_toolBar = new KToolBar(this, true, true);
    m_addNickname = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("list-add-user")), i18n("&Add Nickname..."));
    m_addNickname->setWhatsThis(i18n("Click to add a new nick to the list of nicknames that appear on this screen."));
    m_removeNickname = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("list-remove-user")), i18n("&Remove Nickname"));
    m_removeNickname->setWhatsThis(i18n("Click to remove a nick from the list of nicknames that appear on this screen."));
    m_toolBar->addSeparator();
    m_whois = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("office-address-book")), i18n("&Whois"));
    m_openQuery = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("office-address-book")), i18n("Open &Query"));
    m_toolBar->addSeparator();
    m_joinChannel = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("irc-join-channel")), i18n("&Join Channel"));
    connect(m_toolBar, &KToolBar::actionTriggered, this, &NicksOnline::slotPopupMenu_Activated);

    m_nickListView=new QTreeWidget(this);

    // Set to false every 8 seconds to permit a whois on watched nicks lacking information.
    // Remove when server does this automatically.
    m_whoisRequested = true;

    m_onlineIcon = QIcon::fromTheme(QStringLiteral("im-user"));
    m_offlineIcon = QIcon::fromTheme(QStringLiteral("im-user-offline"));
    m_nickListView->setColumnCount(2);
    m_nickListView->headerItem()->setText(0, i18n("Network/Nickname/Channel"));
    m_nickListView->headerItem()->setText(1, i18n("Additional Information"));
    m_nickListView->setRootIsDecorated(true);
    m_nickListView->setSortingEnabled(true);

    Preferences::restoreColumnState(m_nickListView, QStringLiteral("NicksOnline ViewSettings"));

    QString nickListViewWT = i18n(
        "<p>These are all the nicknames on your Nickname Watch list, listed under the "
        "server network they are connected to.</p>"
        "<p>The <b>Additional Information</b> column shows the information known "
        "for each nickname.</p>"
        "<p>The channels the nickname has joined are listed underneath each nickname.</p>"
        "<p>Nicknames appearing under <b>Offline</b> are not connected to any of the "
        "servers in the network.</p>"
        "<p>Right-click with the mouse on a nickname to perform additional functions.</p>");
    m_nickListView->setWhatsThis(nickListViewWT);
    m_nickListView->viewport()->installEventFilter(this);

    connect(m_nickListView, &QTreeWidget::itemDoubleClicked, this, &NicksOnline::processDoubleClick);

    setupToolbarActions(nullptr);

    // Create context menu.
    m_popupMenu = new QMenu(this);
    m_popupMenu->setObjectName(QStringLiteral("nicksonline_context_menu"));
    m_nickListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_nickListView, &QTreeWidget::customContextMenuRequested, this, &NicksOnline::slotCustomContextMenuRequested);
    connect(m_nickListView, &QTreeWidget::itemSelectionChanged, this, &NicksOnline::slotNickListView_SelectionChanged);

    // Display info for all currently-connected servers.
    refreshAllServerOnlineLists();

    // Connect and start refresh timer.
    m_timer = new QTimer(this);
    m_timer->setObjectName(QStringLiteral("nicksOnlineTimer"));
    connect(m_timer, &QTimer::timeout, this, &NicksOnline::timerFired);
    // TODO: User preference for refresh interval.
    m_timer->start(8000);
}

NicksOnline::~NicksOnline()
{
    Preferences::saveColumnState(m_nickListView, QStringLiteral("NicksOnline ViewSettings"));

    m_timer->stop();
    delete m_timer;
    delete m_nickListView;
}

bool NicksOnline::eventFilter(QObject*obj, QEvent* event )
{
    if( ( obj == m_nickListView->viewport() ) && ( event->type() == QEvent::ToolTip ) )
    {
        auto* helpEvent = static_cast<QHelpEvent*>(event);

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

/**
 * Returns the named child of parent item in a NicksOnlineItem
 * @param parent            Pointer to a NicksOnlineItem.
 * @param name              The name in the desired child QListViewItem, must be in column 0.
 * @param type              The type of entry to be found
 * @return                  Pointer to the child QListViewItem or 0 if not found.
 */
QTreeWidgetItem* NicksOnline::findItemChild(const QTreeWidgetItem* parent, const QString& name,
                                            NicksOnlineItem::NickListViewColumn type) const
{
    if (!parent) return nullptr;
    for (int i = 0; i < parent->childCount(); ++i)
    {
        QTreeWidgetItem* child = parent->child(i);
        if (static_cast<NicksOnlineItem*>(child)->type() == type && child->text(0) == name)
            return child;
    }
    return nullptr;
}

/**
 * Returns the first occurrence of a child item of a given type in a NicksOnlineItem
 * @param parent            Pointer to a NicksOnlineItem.
 * @param type              The type of entry to be found
 * @return                  Pointer to the child QListViewItem or 0 if not found.
 */
QTreeWidgetItem* NicksOnline::findItemType(const QTreeWidgetItem* parent, NicksOnlineItem::NickListViewColumn type) const
{
    if (!parent) return nullptr;
    for (int i = 0; i < parent->childCount(); ++i)
    {
        QTreeWidgetItem* child = parent->child(i);
        if (static_cast<NicksOnlineItem*>(child)->type() == type)
            return child;
    }
    return nullptr;
}

/**
 * Returns a pointer to the network QListViewItem with the given name.
 * @param name              The name of the network.
 * @return                  Pointer to the QListViewItem or 0 if not found.
 */
QTreeWidgetItem* NicksOnline::findNetworkRoot(int serverGroupId) const
{
    for (int i = 0; i < m_nickListView->invisibleRootItem()->childCount(); ++i)
    {
        QTreeWidgetItem* child = m_nickListView->invisibleRootItem()->child(i);

        if (child->data(0, Qt::UserRole).toInt() == serverGroupId)
            return child;
    }

    return nullptr;
}

/**
 * Return a string containing formatted additional information about a nick.
 * @param nickInfo          A pointer to NickInfo structure for the nick.  May be Null.
 * @return                  A string formatted for display containing the information
 *                          about the nick.
 * @return needWhois        True if a WHOIS needs to be performed on the nick
 *                          to get additional information.
 */
QString NicksOnline::getNickAdditionalInfo(NickInfoPtr nickInfo, bool& needWhois) const
{
    Q_UNUSED(needWhois)

    QString niInfo;
    if (nickInfo)
    {
        if (nickInfo->isAway())
        {
            niInfo += i18n("Away");
            if (!nickInfo->getAwayMessage().isEmpty())
                niInfo += QLatin1String(" (") + nickInfo->getAwayMessage() + QLatin1Char(')');
        }
        if (!nickInfo->getHostmask().isEmpty())
            niInfo += QLatin1Char(' ') + nickInfo->getHostmask();
        if (!nickInfo->getRealName().isEmpty())
            niInfo += QLatin1String(" (") + nickInfo->getRealName() + QLatin1Char(')');
        if (!nickInfo->getNetServer().isEmpty())
        {
            niInfo += i18n( " online via %1", nickInfo->getNetServer() );
            if (!nickInfo->getNetServerInfo().isEmpty())
                niInfo += QLatin1String(" (") + nickInfo->getNetServerInfo() + QLatin1Char(')');
        }
        if (!nickInfo->getOnlineSince().isNull())
            niInfo += i18n( " since %1", nickInfo->getPrettyOnlineSince() );
    }

    return niInfo;
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
    QStringList serverList = networkRoot->text(nlvcAdditionalInfo).split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (!serverList.contains(serverName)) serverList.append(serverName);
    networkRoot->setText(nlvcAdditionalInfo, serverList.join(QLatin1Char(',')));
    // Get watch list.
    const QStringList watchList = servr->getWatchList();

    for (const QString& nickname : watchList) {
        NickInfoPtr nickInfo = getOnlineNickInfo(networkName, nickname);

        if (nickInfo && nickInfo->getPrintedOnline())
        {
            // Nick is online.
            // Which server did NickInfo come from?
            Server* server=nickInfo->getServer();
            // Construct additional information string for nick.
            bool needWhois = false;
            QString nickAdditionalInfo = getNickAdditionalInfo(nickInfo, needWhois);
            // Add to network if not already added.
            QTreeWidgetItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
            if (!nickRoot)
                nickRoot = new NicksOnlineItem(NicksOnlineItem::NicknameItem, networkRoot, nickname, nickAdditionalInfo);
            auto* nickitem = static_cast<NicksOnlineItem*>(nickRoot);
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

            const QStringList channelList = server->getNickChannels(nickname);

            for (const QString& channelName : channelList) {
                // Known channels where nickname is online and mode in each channel.
                // FIXME: If user connects to multiple servers in same network, the
                // channel info will differ between the servers, resulting in inaccurate
                // mode and led info displayed.

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
                const bool isAway = (server->getJoinedChannelMembers(channelName) == nullptr);
                channelItem->setIcon(nlvcChannel,
                                     Application::instance()->images()->getNickIcon(nickPrivilege, isAway));
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
            auto* nickitem = static_cast<NicksOnlineItem*>(nickRoot);
            nickitem->setConnectionId(servr->connectionId ());
            // Mark nick as offline
            nickitem->setOffline (true);
            // Update icon
            nickitem->setIcon(nlvcNick, m_offlineIcon);
            nickRoot->setText(nlvcServerName, serverName);
            nickRoot->setText(nlvcAdditionalInfo, QString());
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
        connect (servr, QOverload<Server*, NickInfoPtr>::of(&Server::nickInfoChanged),
            this, &NicksOnline::slotNickInfoChanged);
    }
}

/**
 * Determines if a nick is online in any of the servers in a network and returns
 * a NickInfo if found, otherwise 0.
 * @param networkName        Server network name.
 * @param nickname           Nick name.
 * @return                   NickInfo if nick is online in any server, otherwise 0.
 */
NickInfoPtr NicksOnline::getOnlineNickInfo(const QString& networkName, const QString& nickname) const
{
    // Get list of pointers to all servers.
    Application* konvApp = Application::instance();
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();
    for (Server* server : serverList) {
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
void NicksOnline::requestWhois(const QString& networkName, const QString& nickname)
{
    Application* konvApp = Application::instance();
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();
    for (Server* server : serverList) {
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
      auto* item = static_cast<NicksOnlineItem*>(networkRoot->child(j));
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
  Application::instance()->saveOptions(false);
}

/**
 * Refresh the nicklistview for all servers.
 */
void NicksOnline::refreshAllServerOnlineLists()
{
    Application* konvApp = Application::instance();
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();
    // Remove servers no longer connected.
    for (int i = 0; i < m_nickListView->invisibleRootItem()->childCount(); ++i)
    {
        QTreeWidgetItem *child = m_nickListView->invisibleRootItem()->child(i);
        QString networkName = child->text(nlvcNetwork);
        QStringList serverNameList = child->text(nlvcAdditionalInfo).split(QLatin1Char(','), Qt::SkipEmptyParts);
        QStringList::Iterator itEnd = serverNameList.end();
        QStringList::Iterator it = serverNameList.begin();
        while (it != itEnd)
        {
            QString serverName = *it;
            // Locate server in server list.
            bool found = false;
            for (Server* server : serverList) {
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
            child->setText(nlvcAdditionalInfo, serverNameList.join(QLatin1Char(',')));
    }
    // Display info for all currently-connected servers.
    for (Server* server : serverList) {
        updateServerOnlineList(server);
    }
    // Refresh buttons.
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
    Q_UNUSED(column)

    auto* nickitem = dynamic_cast<NicksOnlineItem*>(item);

    if (!nickitem || nickitem->isOffline())
        return;
    // Only emit signal when the user double clicked a nickname rather than
    // a server name or channel name.
    if (nickitem->type() == NicksOnlineItem::NicknameItem)
        Q_EMIT doubleClicked(nickitem->connectionId(), nickitem->text(nlvcNick));
    if (nickitem->type() == NicksOnlineItem::ChannelItem)
    {
      auto* nickRoot = static_cast<NicksOnlineItem*>(nickitem->parent());
      Server* server = Application::instance()->getConnectionManager()->getServerByConnectionId(nickRoot->connectionId());
      ChatWindow* channel = server->getChannelByName(nickitem->text(nlvcChannel));

      if (channel)
        Q_EMIT showView(channel);
      else
      {
        // Get the server object corresponding to the connection id.
        server->queue( QStringLiteral("JOIN ")+ nickitem->text(nlvcChannel) );
      }
    }
}

/**
 * Returns the server name and nickname of the specified nicklistview item.
 * @param item              The nicklistview item.
 * @return serverName       Name of the server for the nick at the item, or Null if not a nick.
 * @return nickname         The nickname at the item.
 */
bool NicksOnline::getItemServerAndNick(const QTreeWidgetItem* item, QString& serverName, QString& nickname) const
{
    if (!item) return false;
    // convert into NicksOnlineItem
    const auto* nlItem = static_cast<const NicksOnlineItem*>(item);
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

NickInfoPtr NicksOnline::getNickInfo(const QTreeWidgetItem* item) const
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
                                                   const QString& nickname) const
{
    Server* server = Application::instance()->getConnectionManager()->getServerByName(serverName);
    if (!server) return nullptr;
    QString networkName = server->getDisplayName();
    QList<QTreeWidgetItem*> items = m_nickListView->findItems(networkName, Qt::MatchExactly | Qt::MatchCaseSensitive, nlvcNetwork);
    if (items.isEmpty()) return nullptr;
    QTreeWidgetItem* networkRoot = items.at(0);
    if (!networkRoot) return nullptr;
    QTreeWidgetItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
    return nickRoot;
}

/**
 * Perform a command.
 * @param id                The command id.  @ref CommandIDs.
 *
 * The operation is performed on the nickname at the currently-selected item in
 * the nicklistview.
 *
 * Also refreshes the nicklistview display to reflect the new state
 * for the nick.
 */
void NicksOnline::doCommand(QAction* id)
{
    if(id == nullptr)
        return;
    if ( id == m_addNickname )
    {
        int serverGroupId = -1;

        if (!m_nickListView->selectedItems().isEmpty()) {
            NicksOnlineItem *networkRoot = dynamic_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0));
            if (networkRoot)
            {
                while (networkRoot->type() != NicksOnlineItem::NetworkRootItem)
                    networkRoot = static_cast<NicksOnlineItem*>(networkRoot->parent());

                serverGroupId = networkRoot->data(0, Qt::UserRole).toInt();
            }
        }
        auto *end = new EditNotifyDialog(this, serverGroupId);
        connect(end, &EditNotifyDialog::notifyChanged, this, &NicksOnline::slotAddNickname);
        end->show();
        return;
    }

    QString serverName;
    QString nickname;
    if (m_nickListView->selectedItems().isEmpty()) return;
    QTreeWidgetItem* item = m_nickListView->selectedItems().at(0);
    auto* nickitem = dynamic_cast<NicksOnlineItem*>(item);

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

    if ( id == m_joinChannel )
    {
        if (!m_nickListView->selectedItems().isEmpty()) {
            // only join real channels
            if (static_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0))->type() == NicksOnlineItem::ChannelItem) {
                QString contactChannel = m_nickListView->selectedItems().at(0)->text(nlvcChannel);
                server->queue( QStringLiteral("JOIN ")+contactChannel );
            }
        }
    }
    else if ( id == m_whois )
    {
            server->queue(QStringLiteral("WHOIS ")+nickname);
    }
    else if ( id == m_openQuery )
    {
            NickInfoPtr nickInfo = server->obtainNickInfo(nickname);
            class Query* query = server->addQuery(nickInfo, true /*we initiated*/);
            Q_EMIT showView(query);
    }
    else
            refreshItem(item);
}

/**
 * Sets up toolbar actions based on the given item.
 * @param item              Item of the nicklistview.
 */
void NicksOnline::setupToolbarActions(NicksOnlineItem *item)
{
  // disable all actions
  m_removeNickname->setEnabled(false);
  m_whois->setEnabled(false);
  m_openQuery->setEnabled(false);
  m_joinChannel->setEnabled(false);
  // check for null
  if (item == nullptr)
    return;
  // add items depending on the item type
  switch (item->type())
  {
  case NicksOnlineItem::ChannelItem:
    m_joinChannel->setEnabled(true);
    break;
  case NicksOnlineItem::NicknameItem:
    m_removeNickname->setEnabled(true);
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
  if (item == nullptr)
    return;
  // add items depending on the item type
  switch (item->type())
  {
  case NicksOnlineItem::NetworkRootItem:
    m_popupMenu->insertAction(nullptr, m_addNickname);
    break;
  case NicksOnlineItem::ChannelItem:
    m_popupMenu->insertAction(nullptr, m_joinChannel);
    break;
  case NicksOnlineItem::NicknameItem:
    m_popupMenu->insertAction(nullptr, m_removeNickname);
    if (!item->isOffline())
    {
      m_popupMenu->addSeparator();
      m_popupMenu->insertAction(nullptr, m_whois);
      m_popupMenu->insertAction(nullptr, m_openQuery);
    }
    break;
  }
}

/**
 * Received when user selects a different item in the nicklistview.
 */
void NicksOnline::slotNickListView_SelectionChanged()
{
    if (m_nickListView->selectedItems().isEmpty())
      return;
    setupToolbarActions(dynamic_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0)));
}

/**
 * Received when right-clicking an item in the NickListView.
 */
void NicksOnline::slotCustomContextMenuRequested(const QPoint& point)
{
    QTreeWidgetItem *item = m_nickListView->itemAt(point);
    if (item == nullptr)
      return;
    // select the item
    item->setSelected(true);
    // set up actions
    setupPopupMenuActions(dynamic_cast<NicksOnlineItem*>(item));
    // show the popup menu
    if (!m_popupMenu->actions().isEmpty())
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
void NicksOnline::slotNickInfoChanged(Server* server, const NickInfoPtr &nickInfo)
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
void NicksOnline::slotAddNickname(int serverGroupId, const QString& nickname)
{
    Preferences::addNotify(serverGroupId, nickname);
    Application::instance()->saveOptions(true);
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
            QString nickAdditionalInfo;
            bool needWhois = false;
            if (nickInfo)
                nickAdditionalInfo = getNickAdditionalInfo(nickInfo, needWhois);
            item->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            if (!m_nickListView->selectedItems().isEmpty() && item == m_nickListView->selectedItems().at(0))
                setupToolbarActions(dynamic_cast<NicksOnlineItem*>(m_nickListView->selectedItems().at(0)));
        }
    }
}

void NicksOnline::childAdjustFocus() {}

#include "moc_nicksonline.cpp"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
