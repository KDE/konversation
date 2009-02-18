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
#include "application.h" ////// header renamed
#include "connectionmanager.h"
#include "images.h"
#include "query.h"
#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"
#include "mainwindow.h" ////// header renamed
#include "viewcontainer.h"
#include "nicksonlineitem.h"

#include <qlayout.h>
#include <qstringlist.h>

#include <qpushbutton.h>
#include <qlabel.h>
#include <QToolTip>

#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>
#include <k3listview.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kvbox.h>


NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
{
    setName(i18n("Watched Nicks Online"));
    setType(ChatWindow::NicksOnline);

    m_nickListView=new K3ListView(this);

    // Set to false every 8 seconds to permit a whois on watched nicks lacking information.
    // Remove when server or addressbook does this automatically.
    m_whoisRequested = true;

    m_nickListView->addColumn(i18n("Network/Nickname/Channel"));
    m_kabcIconSet = KIcon("office-address-book");
    m_nickListView->addColumn(i18n("Additional Information"));
    m_nickListView->setFullWidth(true);
    m_nickListView->setRootIsDecorated(true);
    m_nickListView->setShowToolTips(false);
    m_nickListView->setShadeSortColumn(true);
    m_nickListView->setShowSortIndicator(true);

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
    m_nickListView->installEventFilter(this);
    setMargin(margin());
    setSpacing(spacing());

    KHBox* buttonBox=new KHBox(this);
    buttonBox->setSpacing(spacing());
    QPushButton* editButton = new QPushButton(i18n("&Edit Watch List..."), buttonBox);
    editButton->setObjectName("edit_notify_button");
    QString editButtonWT = i18n(
        "Click to edit the list of nicknames that appear on this screen.");
    editButton->setWhatsThis(editButtonWT);

    connect(editButton, SIGNAL(clicked()), SIGNAL(editClicked()) );
    connect(m_nickListView, SIGNAL(doubleClicked(Q3ListViewItem*)),
        this,SLOT(processDoubleClick(Q3ListViewItem*)));

    QLabel* addressbookLabel = new QLabel(i18n("Address book:"), buttonBox);
    addressbookLabel->setObjectName("nicksonline_addressbook_label");
    QString addressbookLabelWT = i18n(
        "When you select a nickname in the list above, the buttons here are used "
        "to associate the nickname with an entry in KAddressBook.");
    addressbookLabel->setWhatsThis(addressbookLabelWT);
    addressbookLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editContactButton = new QPushButton(i18n("Edit C&ontact..."), buttonBox);
    m_editContactButton->setObjectName("nicksonline_editcontact_button");
    QString editContactButtonWT = i18n(
        "Click to create, view, or edit the KAddressBook entry associated with the nickname "
        "selected above.");
    m_editContactButton->setWhatsThis(editContactButtonWT);
    m_editContactButton->setIcon(m_kabcIconSet);
    m_changeAssociationButton = new QPushButton(i18n("&Change Association..."), buttonBox);
    m_changeAssociationButton->setObjectName("nicksonline_changeassociation_button");
    QString changeAssociationButtonWT = i18n(
        "Click to associate the nickname selected above with an entry in KAddressBook.");
    m_changeAssociationButton->setWhatsThis(changeAssociationButtonWT);
    m_changeAssociationButton->setIcon(m_kabcIconSet);
    m_deleteAssociationButton = new QPushButton(i18n("&Delete Association"), buttonBox);
    m_deleteAssociationButton->setObjectName("nicksonline_deleteassociation_button");
    QString deleteAssociationButtonWT = i18n(
        "Click to remove the association between the nickname selected above and a "
        "KAddressBook entry.");
    m_deleteAssociationButton->setWhatsThis(deleteAssociationButtonWT);
    m_deleteAssociationButton->setIcon(m_kabcIconSet);

    connect(m_editContactButton, SIGNAL(clicked()),
        this, SLOT(slotEditContactButton_Clicked()));
    connect(m_changeAssociationButton, SIGNAL(clicked()),
        this, SLOT(slotChangeAssociationButton_Clicked()));
    connect(m_deleteAssociationButton, SIGNAL(clicked()),
        this, SLOT(slotDeleteAssociationButton_Clicked()));
    connect(m_nickListView, SIGNAL(selectionChanged()),
        this, SLOT(slotNickListView_SelectionChanged()));

    setupAddressbookButtons(nsNotANick);

    // Create context menu.  Individual menu entries are created in rightButtonClicked slot.
    m_popupMenu = new KMenu(this);

    m_popupMenu->setObjectName("nicksonline_context_menu");
    connect(m_nickListView, SIGNAL(rightButtonClicked(Q3ListViewItem *, const QPoint &, int )),
        this, SLOT(slotNickListView_RightButtonClicked(Q3ListViewItem*, const QPoint &)));
    connect(m_popupMenu, SIGNAL(triggered ( QAction *)),
        this, SLOT(slotPopupMenu_Activated(QAction*)));

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
    m_timer->stop();
    delete m_timer;
    delete m_nickListView;
}

bool NicksOnline::eventFilter(QObject*obj, QEvent* event )
{
    if( ( obj == m_nickListView ) && ( event->type() == QEvent::ToolTip ) )
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>( event );


        Q3ListViewItem *item = m_nickListView->itemAt( helpEvent->pos() );
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

K3ListView* NicksOnline::getNickListView()
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
Q3ListViewItem* NicksOnline::findItemChild(const Q3ListViewItem* parent, const QString& name, NicksOnlineItem::NickListViewColumn type)
{
    if (!parent) return 0;
    Q3ListViewItem* child;
    for (child = parent->firstChild(); (child) ; child = child->nextSibling())
    {
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
Q3ListViewItem* NicksOnline::findItemType(const Q3ListViewItem* parent, NicksOnlineItem::NickListViewColumn type)
{
    if (!parent) return 0;
    Q3ListViewItem* child;
    for (child = parent->firstChild(); (child) ; child = child->nextSibling())
    {
        if(static_cast<NicksOnlineItem*>(child)->type() == type) return child;
    }
    return 0;
}

/**
 * Returns a pointer to the network QListViewItem with the given name.
 * @param name              The name of the network, assumed to be in column 0 of the item.
 * @return                  Pointer to the QListViewItem or 0 if not found.
 */
Q3ListViewItem* NicksOnline::findNetworkRoot(const QString& name)
{
    Q3ListViewItem* child;
    for (child = getNickListView()->firstChild(); (child) ; child = child->nextSibling())
    {
        if (child->text(0) == name) return child;
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
    bool newNetworkRoot = false;
    QString serverName = servr->getServerName();
    QString networkName = servr->getDisplayName();
    Q3ListViewItem* networkRoot = findNetworkRoot(networkName);
    // If network is not in our list, add it.
    if (!networkRoot)
    {
        networkRoot = new NicksOnlineItem(NicksOnlineItem::NetworkRootItem,m_nickListView,networkName);
        newNetworkRoot = true;
    }
    // Store server name in hidden column.
    // Note that there could be more than one server in the network connected,
    // but it doesn't matter because all the servers in a network have the same
    // watch list.
    networkRoot->setText(nlvcServerName, serverName);
    // Update list of servers in the network that are connected.
    QStringList serverList = networkRoot->text(nlvcAdditionalInfo).split(",", QString::SkipEmptyParts);
    if (!serverList.contains(serverName)) serverList.append(serverName);
    networkRoot->setText(nlvcAdditionalInfo, serverList.join(","));
    // Get item in nicklistview for the Offline branch.
    Q3ListViewItem* offlineRoot = findItemType(networkRoot, NicksOnlineItem::OfflineItem);
    if (!offlineRoot)
    {
        offlineRoot = new NicksOnlineItem(NicksOnlineItem::OfflineItem,networkRoot,i18n("Offline"));
        offlineRoot->setText(nlvcServerName, serverName);
    }

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
            // Remove from offline branch if present.
            Q3ListViewItem* item = findItemChild(offlineRoot, nickname, NicksOnlineItem::NicknameItem);
            if (item)
                delete item;
            // Add to network if not already added.
            Q3ListViewItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
            if (!nickRoot)
                nickRoot = new NicksOnlineItem(NicksOnlineItem::NicknameItem, networkRoot, nickname, nickAdditionalInfo);
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
                nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Normal, QIcon::On));
            else
                nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Disabled, QIcon::Off));

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
                Q3ListViewItem* channelItem = findItemChild(nickRoot, channelName, NicksOnlineItem::ChannelItem);
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
                    channelItem->setPixmap(nlvcChannel,
                        KonversationApplication::instance()->images()->getNickIcon(nickPrivilege, false));
                else
                    channelItem->setPixmap(nlvcChannel,
                        KonversationApplication::instance()->images()->getNickIcon(nickPrivilege, true));
            }
            // Remove channel if nick no longer in it.
            Q3ListViewItem* child = nickRoot->firstChild();
            while (child)
            {
                Q3ListViewItem* nextChild = child->nextSibling();
                if (!channelList.contains(child->text(nlvcNick)))
                    delete child;
                child = nextChild;
            }
        }
        else
        {
            // Nick is offline.
            // Remove from online nicks, if present.
            Q3ListViewItem* item = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
            if (item) delete item;
            // Add to offline list if not already listed.
            Q3ListViewItem* nickRoot = findItemChild(offlineRoot, nickname, NicksOnlineItem::NicknameItem);
            if (!nickRoot) nickRoot = new NicksOnlineItem(NicksOnlineItem::NicknameItem,offlineRoot, nickname);
            nickRoot->setText(nlvcServerName, serverName);
            // Get addressbook entry for the nick.
            KABC::Addressee addressee = servr->getOfflineNickAddressee(nickname);
            // Format additional information for the nick.
            bool needWhois = false;
            QString nickAdditionalInfo = getNickAdditionalInfo(NickInfoPtr(), addressee, needWhois);
            nickRoot->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            // Set Kabc icon if the nick is associated with an addressbook entry.
            if (!addressee.isEmpty())
                nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Normal, QIcon::On));
            else
                nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                    KIconLoader::Small, QIcon::Disabled, QIcon::Off));
        }
    }
    // Erase nicks no longer being watched.
    Q3ListViewItem* item = networkRoot->firstChild();
    while (item)
    {
        Q3ListViewItem* nextItem = item->nextSibling();
        if (static_cast<NicksOnlineItem*>(item)->type() != NicksOnlineItem::OfflineItem)
        {
            QString nickname = item->text(nlvcNick);
            if (!watchList.contains(nickname) && serverName == item->text(nlvcServerName))
                delete item;
        }
        item = nextItem;
    }
    item = offlineRoot->firstChild();

    if(item) {
        while (item)
        {
            Q3ListViewItem* nextItem = item->nextSibling();
            QString nickname = item->text(nlvcNick);
            if (!watchList.contains(nickname) && serverName == item->text(nlvcServerName))
                delete item;
            item = nextItem;
        }
    }
    else
    {
        delete offlineRoot;
    }
    // Expand server if newly added to list.
    if (newNetworkRoot)
    {
        networkRoot->setOpen(true);
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
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
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
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
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
 * Refresh the nicklistview for all servers.
 */
void NicksOnline::refreshAllServerOnlineLists()
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();
    // Remove servers no longer connected.
    Q3ListViewItem* child = m_nickListView->firstChild();
    while (child)
    {
        Q3ListViewItem* nextChild = child->nextSibling();
        QString networkName = child->text(nlvcNetwork);
        QStringList serverNameList = child->text(nlvcAdditionalInfo).split(",", QString::SkipEmptyParts);
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
            delete child;
        else
            child->setText(nlvcAdditionalInfo, serverNameList.join(","));
        child = nextChild;
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
void NicksOnline::processDoubleClick(Q3ListViewItem* item)
{
    // Only emit signal when the user double clicked a nickname rather than
    // a server name or channel name.
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
        emit doubleClicked(serverName, nickname);
}

/**
 * Returns the server name and nickname of the specified nicklistview item.
 * @param item              The nicklistview item.
 * @return serverName       Name of the server for the nick at the item, or Null if not a nick.
 * @return nickname         The nickname at the item.
 */
bool NicksOnline::getItemServerAndNick(const Q3ListViewItem* item, QString& serverName, QString& nickname)
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
    // offline columns are not nick names
    if (nlItem->type() == NicksOnlineItem::OfflineItem) return false;
    return true;
}

NickInfoPtr NicksOnline::getNickInfo(const Q3ListViewItem* item)
{
    QString serverName;
    QString nickname;

    getItemServerAndNick(item, serverName, nickname);

    if (serverName.isEmpty() || nickname.isEmpty())
        return NickInfoPtr(); //TODO FIXME NULL NULL NULL

    Server* server = KonversationApplication::instance()->getConnectionManager()->getServerByName(serverName);

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
Q3ListViewItem* NicksOnline::getServerAndNickItem(const QString& serverName,
const QString& nickname)
{
    Server* server = KonversationApplication::instance()->getConnectionManager()->getServerByName(serverName);
    if (!server) return 0;
    QString networkName = server->getDisplayName();
    Q3ListViewItem* networkRoot = m_nickListView->findItem(networkName, nlvcNetwork);
    if (!networkRoot) return 0;
    Q3ListViewItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
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

    QString serverName;
    QString nickname;
    Q3ListViewItem* item = m_nickListView->selectedItem();

    if(!getItemServerAndNick(item, serverName, nickname))
    {
        return;
    }

    // Get the server object corresponding to the server name.
    Server* server = KonversationApplication::instance()->getConnectionManager()->getServerByName(serverName);

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
    else if ( id == m_addressBookChange )
    {
            if(nickInfo)
            {
                nickInfo->showLinkAddressbookUI();
            }
            else
            {
                LinkAddressbookUI *linkaddressbookui = new LinkAddressbookUI(server->getViewContainer()->getWindow(), nickname, server->getServerName(), server->getDisplayName(), addressee.realName());
                linkaddressbookui->show();
            }
    }
    else if ( id == m_chooseAssociation || id == m_deleteAssociation )
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
            // only join real channels
            if (static_cast<NicksOnlineItem*>(m_nickListView->selectedItem())->type() == NicksOnlineItem::ChannelItem)
            {
                QString contactChannel = m_nickListView->selectedItem()->text(nlvcChannel);
                server->queue( "JOIN "+contactChannel );
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
int NicksOnline::getNickAddressbookState(Q3ListViewItem* item)
{
    int nickState = nsNotANick;
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
    {
        Server *server = KonversationApplication::instance()->getConnectionManager()->getServerByName(serverName);
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
 * Sets the enabled/disabled state and labels of the addressbook buttons
 * based on the given nick addressbook state.
 * @param nickState         The state of the nick. 1 = not associated with addressbook,
 *                          2 = associated with addressbook.  @ref getNickAddressbookState.
 */
void NicksOnline::setupAddressbookButtons(int nickState)
{
    switch (nickState)
    {
        case nsNotANick:
        {
            m_editContactButton->setEnabled(false);
            m_changeAssociationButton->setEnabled(false);
            m_deleteAssociationButton->setEnabled(false);
            break;
        }
        case nsNoAddress:
        {
            m_editContactButton->setText(i18n("Create New C&ontact..."));
            m_editContactButton->setEnabled(true);
            m_changeAssociationButton->setText(i18n("&Choose Association..."));
            m_changeAssociationButton->setEnabled(true);
            m_deleteAssociationButton->setEnabled(false);
            break;
        }
        case nsHasAddress:
        {
            m_editContactButton->setText(i18n("Edit C&ontact..."));
            m_editContactButton->setEnabled(true);
            m_changeAssociationButton->setText(i18n("&Change Association..."));
            m_changeAssociationButton->setEnabled(true);
            m_deleteAssociationButton->setEnabled(true);
            break;
        }
    }
}

/**
 * Received when user clicks the Edit Contact (or New Contact) button.
 */
void NicksOnline::slotEditContactButton_Clicked()
{
    switch (getNickAddressbookState(m_nickListView->selectedItem()))
    {
        case nsNotANick:    break;
        case nsNoAddress:   { doCommand(m_newContact); break; }
        case nsHasAddress:  { doCommand(m_editContact); break; }
    }
}

/**
 * Received when user clicks the Change Association button.
 */
void NicksOnline::slotChangeAssociationButton_Clicked() { doCommand(m_addressBookChange); }
/**
 * Received when user clicks the Delete Association button.
 */
void NicksOnline::slotDeleteAssociationButton_Clicked() { doCommand(m_deleteAssociation); }
/**
 * Received when user selects a different item in the nicklistview.
 */
void NicksOnline::slotNickListView_SelectionChanged()
{
    Q3ListViewItem* item = m_nickListView->selectedItem();
    int nickState = getNickAddressbookState(item);
    setupAddressbookButtons(nickState);
}

/**
 * Received when right-clicking an item in the NickListView.
 */
void NicksOnline::slotNickListView_RightButtonClicked(Q3ListViewItem* item, const QPoint& pt)
{
    if (!item) return;
    m_popupMenu->clear();
    int nickState = getNickAddressbookState(item);
    switch (nickState)
    {
        case nsNotANick:
        {
            break;
        }
        case nsNoAddress:
        {
            m_chooseAssociation =  m_popupMenu->addAction(i18n("&Choose Association..."));
            m_newContact = m_popupMenu->addAction(i18n("Create New C&ontact..."));
            m_popupMenu->addSeparator();
            m_whois = m_popupMenu->addAction(i18n("&Whois"));
            m_openQuery = m_popupMenu->addAction(i18n("Open &Query"));
            if (item->text(nlvcServerName).isEmpty())
                m_joinChannel = m_popupMenu->addAction(i18n("&Join Channel"));
            break;
        }
        case nsHasAddress:
        {
            m_sendMail = m_popupMenu->addAction(KIcon("mail-send"), i18n("&Send Email..."));
            m_popupMenu->addSeparator();
            m_editContact = m_popupMenu->addAction(KIcon("document-edit"), i18n("Edit C&ontact..."));
            m_popupMenu->addSeparator();
            m_addressBookChange = m_popupMenu->addAction(i18n("&Change Association..."));
            m_deleteAssociation =  m_popupMenu->addAction(KIcon("edit-delete"), i18n("&Delete Association"));
            m_popupMenu->addSeparator();
            m_whois = m_popupMenu->addAction(i18n("&Whois"));
            m_openQuery = m_popupMenu->addAction(i18n("Open &Query"));
            if (item->text(nlvcServerName).isEmpty())
                m_joinChannel = m_popupMenu->addAction(i18n("&Join Channel"));
            break;
        }
    }
    if (nickState != nsNotANick)
        m_popupMenu->popup(pt);
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
    Q3ListViewItem* item = getServerAndNickItem(serverName, nickname);
    refreshItem(item);
}

/**
 * Refreshes the information for the given item in the list.
 * @param item               Pointer to listview item.
 */
void NicksOnline::refreshItem(Q3ListViewItem* item)
{
    if (!item)
        return;
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
    {
        Server *server = KonversationApplication::instance()->getConnectionManager()->getServerByName(serverName);
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
                    item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                        KIconLoader::Small, QIcon::Disabled, QIcon::Off));
                    break;
                }
                case nsHasAddress:
                {
                    item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                        KIconLoader::Small, QIcon::Normal, QIcon::On)); break;
                }
            }
            QString nickAdditionalInfo;
            bool needWhois = false;
            if (nickInfo)
                nickAdditionalInfo = getNickAdditionalInfo(nickInfo, addressee, needWhois);
            item->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            if (item == m_nickListView->selectedItem())
                setupAddressbookButtons(nickState);
        }
    }
}

void NicksOnline::childAdjustFocus() {}

#include "nicksonline.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
