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

// Qt includes.
#include <qlayout.h>
#include <qstringlist.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qptrlist.h>
#include <qwhatsthis.h>

// KDE includes.
#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kprocess.h>
#include <kmessagebox.h>

// Konversation includes.
#include "channel.h"
#include "nicksonline.h"
#include "server.h"
#include "konversationapplication.h"
#include "images.h"
#include "query.h"
#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"
#include "linkaddressbook/nicksonlinetooltip.h"
#include "konversationmainwindow.h"
#include "nicksonlineitem.h"

NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
{
    setName(i18n("Watched Nicks Online"));
    setType(ChatWindow::NicksOnline);

    m_nickListView=new KListView(this);

    // Set to false every 8 seconds to permit a whois on watched nicks lacking information.
    // Remove when server or addressbook does this automatically.
    m_whoisRequested = true;

    m_nickListView->addColumn(i18n("Network/Nickname/Channel"));
    m_kabcIconSet = KGlobal::iconLoader()->loadIconSet("kaddressbook",KIcon::Small);
    m_nickListView->addColumn(i18n("Additional Information"));
    m_nickListView->setFullWidth(false);
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
    QWhatsThis::add(m_nickListView, nickListViewWT);

    m_tooltip = new Konversation::KonversationNicksOnlineToolTip(m_nickListView->viewport(), this);

    setMargin(margin());
    setSpacing(spacing());

    QHBox* buttonBox=new QHBox(this);
    buttonBox->setSpacing(spacing());
    QPushButton* editButton=new QPushButton(i18n("&Edit Watch List..."),
        buttonBox,"edit_notify_button");
    QString editButtonWT = i18n(
        "Click to edit the list of nicknames that appear on this screen.");
    QWhatsThis::add(editButton, editButtonWT);

    connect(editButton, SIGNAL(clicked()), SIGNAL(editClicked()) );
    connect(m_nickListView, SIGNAL(doubleClicked(QListViewItem*)),
        this,SLOT(processDoubleClick(QListViewItem*)));

    QLabel* addressbookLabel = new QLabel(i18n("Address book:"),
        buttonBox, "nicksonline_addressbook_label");
    QString addressbookLabelWT = i18n(
        "When you select a nickname in the list above, the buttons here are used "
        "to associate the nickname with an entry in KAddressBook.");
    QWhatsThis::add(addressbookLabel, addressbookLabelWT);
    addressbookLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editContactButton = new QPushButton(i18n("Edit C&ontact..."),
        buttonBox, "nicksonline_editcontact_button");
    QString editContactButtonWT = i18n(
        "Click to create, view, or edit the KAddressBook entry associated with the nickname "
        "selected above.");
    QWhatsThis::add(m_editContactButton, editContactButtonWT);
    m_editContactButton->setIconSet(m_kabcIconSet);
    m_changeAssociationButton = new QPushButton(i18n("&Change Association..."),
        buttonBox, "nicksonline_changeassociation_button");
    QString changeAssociationButtonWT = i18n(
        "Click to associate the nickname selected above with an entry in KAddressBook.");
    QWhatsThis::add(m_changeAssociationButton, changeAssociationButtonWT);
    m_changeAssociationButton->setIconSet(m_kabcIconSet);
    m_deleteAssociationButton = new QPushButton(i18n("&Delete Association"),
        buttonBox, "nicksonline_deleteassociation_button");
    QString deleteAssociationButtonWT = i18n(
        "Click to remove the association between the nickname selected above and a "
        "KAddressBook entry.");
    QWhatsThis::add(m_deleteAssociationButton, deleteAssociationButtonWT);
    m_deleteAssociationButton->setIconSet(m_kabcIconSet);

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
    m_popupMenu = new QPopupMenu(this,"nicksonline_context_menu");
    connect(m_nickListView, SIGNAL(rightButtonClicked(QListViewItem *, const QPoint &, int )),
        this, SLOT(slotNickListView_RightButtonClicked(QListViewItem*, const QPoint &)));
    connect(m_popupMenu, SIGNAL(activated(int)),
        this, SLOT(slotPopupMenu_Activated(int)));

    // Display info for all currently-connected servers.
    refreshAllServerOnlineLists();

    // Connect and start refresh timer.
    m_timer = new QTimer(this, "nicksOnlineTimer");
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

KListView* NicksOnline::getNickListView()
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
QListViewItem* NicksOnline::findItemChild(const QListViewItem* parent, const QString& name, NicksOnlineItem::NickListViewColumn type)
{
    if (!parent) return 0;
    QListViewItem* child;
    for (child = parent->firstChild(); (child) ; child = child->nextSibling())
    {
        if(static_cast<NicksOnlineItem*>(child)->type() == type && child->text(0) == name) return child;
    }
    return 0;
}

/**
 * Returns the first occurence of a child item of a given type in a NicksOnlineItem
 * @param parent            Pointer to a NicksOnlineItem.
 * @param type              The type of entry to be found
 * @return                  Pointer to the child QListViewItem or 0 if not found.
 */
QListViewItem* NicksOnline::findItemType(const QListViewItem* parent, NicksOnlineItem::NickListViewColumn type)
{
    if (!parent) return 0;
    QListViewItem* child;
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
QListViewItem* NicksOnline::findNetworkRoot(const QString& name)
{
    QListViewItem* child;
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
                niInfo += "(" + nickInfo->getAwayMessage() + ")";
        }
        if (!nickInfo->getHostmask().isEmpty())
            niInfo += " " + nickInfo->getHostmask();
        if (!nickInfo->getRealName().isEmpty())
            niInfo += " (" + nickInfo->getRealName() + ")";
        if (!nickInfo->getNetServer().isEmpty())
        {
            niInfo += i18n( " online via %1" ).arg( nickInfo->getNetServer() );
            if (!nickInfo->getNetServerInfo().isEmpty())
                niInfo += " (" + nickInfo->getNetServerInfo() + ")";
        }
        if (!nickInfo->getOnlineSince().isNull())
            niInfo += i18n( " since %1" ).arg( nickInfo->getPrettyOnlineSince() );
    }
    needWhois = niInfo.isEmpty();
    if (!info.isEmpty() && !needWhois) info += " ";
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
    // TODO: The method name "getServerGroup" is a misnomer.  Actually returns the
    // network for a server.
    QString networkName = servr->getServerGroup();
    QListViewItem* networkRoot = findNetworkRoot(networkName);
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
    QStringList serverList = QStringList::split(",", networkRoot->text(nlvcAdditionalInfo));
    if (!serverList.contains(serverName)) serverList.append(serverName);
    networkRoot->setText(nlvcAdditionalInfo, serverList.join(","));
    // Get item in nicklistview for the Offline branch.
    QListViewItem* offlineRoot = findItemType(networkRoot, NicksOnlineItem::OfflineItem);
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
            QListViewItem* item = findItemChild(offlineRoot, nickname, NicksOnlineItem::NicknameItem);
            if (item) delete item;
            // Add to network if not already added.
            QListViewItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
            if (!nickRoot) nickRoot = new NicksOnlineItem(NicksOnlineItem::NicknameItem,networkRoot, nickname, nickAdditionalInfo);
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
                    QIconSet::Small, QIconSet::Normal, QIconSet::On));
            else
                nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                    QIconSet::Small, QIconSet::Disabled, QIconSet::Off));

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
                QListViewItem* channelItem = findItemChild(nickRoot, channelName, NicksOnlineItem::ChannelItem);
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
            QListViewItem* child = nickRoot->firstChild();
            while (child)
            {
                QListViewItem* nextChild = child->nextSibling();
                if (channelList.find(child->text(nlvcNick)) == channelList.end())
                    delete child;
                child = nextChild;
            }
        }
        else
        {
            // Nick is offline.
            // Remove from online nicks, if present.
            QListViewItem* item = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
            if (item) delete item;
            // Add to offline list if not already listed.
            QListViewItem* nickRoot = findItemChild(offlineRoot, nickname, NicksOnlineItem::NicknameItem);
            if (!nickRoot) nickRoot = new NicksOnlineItem(NicksOnlineItem::NicknameItem,offlineRoot, nickname);
            nickRoot->setText(nlvcServerName, serverName);
            // Get addressbook entry for the nick.
            KABC::Addressee addressee = servr->getOfflineNickAddressee(nickname);
            // Format additional information for the nick.
            bool needWhois = false;
            QString nickAdditionalInfo = getNickAdditionalInfo(0, addressee, needWhois);
            nickRoot->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            // Set Kabc icon if the nick is associated with an addressbook entry.
            if (!addressee.isEmpty())
                nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                    QIconSet::Small, QIconSet::Normal, QIconSet::On));
            else
                nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                    QIconSet::Small, QIconSet::Disabled, QIconSet::Off));
        }
    }
    // Erase nicks no longer being watched.
    QListViewItem* item = networkRoot->firstChild();
    while (item)
    {
        QListViewItem* nextItem = item->nextSibling();
        if (static_cast<NicksOnlineItem*>(item)->type() != NicksOnlineItem::OfflineItem)
        {
            QString nickname = item->text(nlvcNick);
            if ((watchList.find(nickname) == watchList.end()) &&
                (serverName == item->text(nlvcServerName))) delete item;
        }
        item = nextItem;
    }
    item = offlineRoot->firstChild();
    while (item)
    {
        QListViewItem* nextItem = item->nextSibling();
        QString nickname = item->text(nlvcNick);
        if ((watchList.find(nickname) == watchList.end()) &&
            (serverName == item->text(nlvcServerName))) delete item;
        item = nextItem;
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
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    QPtrList<Server> serverList = konvApp->getServerList();
    for (Server* server = serverList.first(); server; server = serverList.next())
    {
        if (server->getServerGroup() == networkName)
        {
            NickInfoPtr nickInfo = server->getNickInfo(nickname);
            if (nickInfo) return nickInfo;
        }
    }
    return 0;
}

/**
 * Requests a WHOIS for a specified server network and nickname.
 * The request is sent to the first server found in the network.
 * @param groupName          Server group name.
 * @param nickname           Nick name.
 */
void NicksOnline::requestWhois(QString& networkName, QString& nickname)
{
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    QPtrList<Server> serverList = konvApp->getServerList();
    for (Server* server = serverList.first(); server; server = serverList.next())
    {
        if (server->getServerGroup() == networkName)
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
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    // Get list of pointers to all servers.
    QPtrList<Server> serverList = konvApp->getServerList();
    Server* server;
    // Remove servers no longer connected.
    QListViewItem* child = m_nickListView->firstChild();
    while (child)
    {
        QListViewItem* nextChild = child->nextSibling();
        QString networkName = child->text(nlvcNetwork);
        QStringList serverNameList = QStringList::split(",", child->text(nlvcAdditionalInfo));
        QStringList::Iterator itEnd = serverNameList.end();
        QStringList::Iterator it = serverNameList.begin();
        while (it != itEnd)
        {
            QString serverName = *it;
            // Locate server in server list.
            bool found = false;
            for (server = serverList.first(); server; server = serverList.next())
            {
                if ((server->getServerName() == serverName) &&
                    (server->getServerGroup() == networkName)) found = true;
            }
            if (!found)
                it = serverNameList.remove(it);
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
    for (server = serverList.first(); server; server = serverList.next())
    {
        updateServerOnlineList(server);
    }
    // Adjust column widths.
    m_nickListView->adjustColumn(nlvcNetworkNickChannel);
    m_nickListView->adjustColumn(nlvcAdditionalInfo);
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
void NicksOnline::processDoubleClick(QListViewItem* item)
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
bool NicksOnline::getItemServerAndNick(const QListViewItem* item, QString& serverName, QString& nickname)
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

NickInfoPtr NicksOnline::getNickInfo(const QListViewItem* item)
{
    QString serverName;
    QString nickname;
    getItemServerAndNick(item, serverName, nickname);
    if(!serverName || !nickname) return 0;
    Server *server = static_cast<KonversationApplication *>(kapp)->getServerByName(serverName);
    return server->getNickInfo(nickname);
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
QListViewItem* NicksOnline::getServerAndNickItem(const QString& serverName,
const QString& nickname)
{
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    Server* server = konvApp->getServerByName(serverName);
    QString networkName = server->getServerGroup();
    QListViewItem* networkRoot = m_nickListView->findItem(networkName, nlvcNetwork);
    if (!networkRoot) return 0;
    QListViewItem* nickRoot = findItemChild(networkRoot, nickname, NicksOnlineItem::NicknameItem);
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
void NicksOnline::doCommand(int id)
{
    if(id < 0)
    {
        return;
    }

    QString serverName;
    QString nickname;
    QListViewItem* item = m_nickListView->selectedItem();

    if(!getItemServerAndNick(item, serverName, nickname))
    {
        return;
    }

    // Get the server object corresponding to the server name.
    KonversationApplication *konvApp = static_cast<KonversationApplication *>(kapp);
    Server* server = konvApp->getServerByName(serverName);

    if(!server)
    {
        return;
    }

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

    switch(id)
    {
        case ciSendEmail:
            Konversation::Addressbook::self()->sendEmail(addressee);
            return;                               //no need to refresh item
        case ciAddressbookEdit:
            Konversation::Addressbook::self()->editAddressee(addressee.uid());
            return;                               //no need to refresh item - nickinfo changed will be called anyway.
        case ciAddressbookChange:
            if(nickInfo)
            {
                nickInfo->showLinkAddressbookUI();
            }
            else
            {
                LinkAddressbookUI *linkaddressbookui = new LinkAddressbookUI(server->getMainWindow(), NULL, nickname, server->getServerName(), server->getServerGroup(), addressee.realName());
                linkaddressbookui->show();
            }
            break;
        case ciAddressbookNew:
        case ciAddressbookDelete:
        {
            Konversation::Addressbook *addressbook = Konversation::Addressbook::self();

            if(addressbook && addressbook->getAndCheckTicket())
            {
                if(id == ciAddressbookDelete)
                {
                    if (addressee.isEmpty())
                    {
                        return;
                    }

                    addressbook->unassociateNick(addressee, nickname, server->getServerName(), server->getServerGroup());
                }
                else
                {
                    addressee.setGivenName(nickname);
                    addressee.setNickName(nickname);
                    addressbook->associateNickAndUnassociateFromEveryoneElse(addressee, nickname, server->getServerName(), server->getServerGroup());
                }
                if(addressbook->saveTicket())
                {
                    //saveTicket will refresh the addressees for us.
                    if(id == ciAddressbookNew)
                    {
                        Konversation::Addressbook::self()->editAddressee(addressee.uid());
                    }
                }
            }
            break;
        }
        case ciJoinChannel:
        {
            // only join real channels
            if (static_cast<NicksOnlineItem*>(m_nickListView->selectedItem())->type() == NicksOnlineItem::ChannelItem)
            {
                QString contactChannel = m_nickListView->selectedItem()->text(nlvcChannel);
                server->queue( "JOIN "+contactChannel );
            }
            break;
        }
        case ciWhois:
            server->queue("WHOIS "+nickname);
            return;
        case ciOpenQuery:
            NickInfoPtr nickInfo = server->obtainNickInfo(nickname);
            class Query* query = server->addQuery(nickInfo, true /*we initiated*/);
            server->getMainWindow()->showView(query);
            return;
    }

    refreshItem(item);
}

/**
 * Get the addressbook state of the nickname at the specified nicklistview item.
 * @param item              Item of the nicklistview.
 * @return                  Addressbook state.
 * 0 = not a nick, 1 = nick has no addressbook association, 2 = nick has association
 */
int NicksOnline::getNickAddressbookState(QListViewItem* item)
{
    int nickState = nsNotANick;
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
    {
        Server *server =
            static_cast<KonversationApplication *>(kapp)->getServerByName(serverName);
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
        case nsNoAddress:   { doCommand(ciAddressbookNew); break; }
        case nsHasAddress:  { doCommand(ciAddressbookEdit); break; }
    }
}

/**
 * Received when user clicks the Change Association button.
 */
void NicksOnline::slotChangeAssociationButton_Clicked() { doCommand(ciAddressbookChange); }
/**
 * Received when user clicks the Delete Association button.
 */
void NicksOnline::slotDeleteAssociationButton_Clicked() { doCommand(ciAddressbookDelete); }
/**
 * Received when user selects a different item in the nicklistview.
 */
void NicksOnline::slotNickListView_SelectionChanged()
{
    QListViewItem* item = m_nickListView->selectedItem();
    int nickState = getNickAddressbookState(item);
    setupAddressbookButtons(nickState);
}

/**
 * Received when right-clicking an item in the NickListView.
 */
void NicksOnline::slotNickListView_RightButtonClicked(QListViewItem* item, const QPoint& pt)
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
            m_popupMenu->insertItem(i18n("&Choose Association..."), ciAddressbookChange);
            m_popupMenu->insertItem(i18n("Create New C&ontact..."), ciAddressbookNew);
            m_popupMenu->insertSeparator();
            m_popupMenu->insertItem(i18n("&Whois"), ciWhois);
            m_popupMenu->insertItem(i18n("Open &Query"), ciOpenQuery);
            if (item->text(nlvcServerName).isEmpty())
                m_popupMenu->insertItem(i18n("&Join Channel"), ciJoinChannel);
            break;
        }
        case nsHasAddress:
        {
            m_popupMenu->insertItem(SmallIcon("mail_generic"), i18n("&Send Email..."), ciSendEmail);
            m_popupMenu->insertSeparator();
            m_popupMenu->insertItem(SmallIcon("contents"), i18n("Edit C&ontact..."), ciAddressbookEdit);
            m_popupMenu->insertSeparator();
            m_popupMenu->insertItem(i18n("&Change Association..."), ciAddressbookChange);
            m_popupMenu->insertItem(SmallIconSet("editdelete"), i18n("&Delete Association"), ciAddressbookDelete);
            m_popupMenu->insertSeparator();
            m_popupMenu->insertItem(i18n("&Whois"), ciWhois);
            m_popupMenu->insertItem(i18n("Open &Query"), ciOpenQuery);
            if (item->text(nlvcServerName).isEmpty())
                m_popupMenu->insertItem(i18n("&Join Channel"), ciJoinChannel);
            break;
        }
    }
    if (nickState != nsNotANick)
        m_popupMenu->popup(pt);
}

/**
 * Received from popup menu when user chooses something.
 */
void NicksOnline::slotPopupMenu_Activated(int id)
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
    QListViewItem* item = getServerAndNickItem(serverName, nickname);
    refreshItem(item);
}

/**
 * Refreshes the information for the given item in the list.
 * @param item               Pointer to listview item.
 */
void NicksOnline::refreshItem(QListViewItem* item)
{
    if (!item) return;
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
    {
        Server *server =
            static_cast<KonversationApplication *>(kapp)->getServerByName(serverName);
        if (server)
        {
            NickInfoPtr nickInfo = server->getNickInfo(nickname);
            KABC::Addressee addressee;
            if (nickInfo)
                addressee = nickInfo->getAddressee();
            else
                addressee = server->getOfflineNickAddressee(nickname);
            int nickState = 2;
            if (addressee.isEmpty()) nickState = 1;
            switch (nickState)
            {
                case nsNotANick:
                    break;
                case nsNoAddress:
                {
                    item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                        QIconSet::Small, QIconSet::Disabled, QIconSet::Off)); break;
                }
                case nsHasAddress:
                {
                    item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                        QIconSet::Small, QIconSet::Normal, QIconSet::On)); break;
                }
            }
            QString nickAdditionalInfo;
            bool needWhois = false;
            if (nickInfo) nickAdditionalInfo = getNickAdditionalInfo(nickInfo, addressee,
                    needWhois);
            item->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            if (item == m_nickListView->selectedItem()) setupAddressbookButtons(nickState);
        }
    }
}

void NicksOnline::childAdjustFocus() {}

#include "nicksonline.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
