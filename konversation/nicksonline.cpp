/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicksonline.cpp  -  shows a user tree of friends per server
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
#include "nicksonline.h"
#include "server.h"
#include "konversationapplication.h"
#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"
#include "linkaddressbook/nicksonlinetooltip.h"

#include "images.h"

#ifdef USE_MDI
NicksOnline::NicksOnline(QString caption): ChatWindow(caption)
#else
NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
#endif
{
    setName(i18n("Watched Nicks Online"));
    setType(ChatWindow::NicksOnline);
    
    // Convenience constant for the internationalized string "Offline".
    c_i18nOffline = i18n("Offline");
    
    m_nickListView=new KListView(this);
    
    // TODO: Need to derive from KListView and override sort() method in order to sort in
    // locale-aware order.
    
    // Set to false every 8 seconds to permit a whois on watched nicks lacking information.
    // Remove when server or addressbook does this automatically.
    m_whoisRequested = true;
    
    m_nickListView->addColumn(i18n("Group/Nickname/Channel"));
    m_kabcIconSet = KGlobal::iconLoader()->loadIconSet("kaddressbook",KIcon::Small);
    m_nickListView->addColumn(i18n("Additional Information"));
    m_nickListView->addColumn("ServerName");
    m_nickListView->hideColumn(nlvcServerName);
    m_nickListView->setFullWidth(false);
    m_nickListView->setRootIsDecorated(true);
    m_nickListView->setShowToolTips(false);
    QString nickListViewWT = i18n(
        "<p>These are all the nicknames on your Nickname Watch list, listed under the "
        "server group they are connected to.  The list also includes the nicknames "
        "in KAddressBook associated with the server group.</p>"
        "<p>The <b>Additional Information</b> column shows the information known "
        "for each nickname.</p>"
        "<p>The channels the nickname has joined are listed underneath each nickname.</p>"
        "<p>Nicknames appearing under <b>Offline</b> are not connected to any of the "
        "servers in the group.</p>"
        "<p>Right-click with the mouse on a nickname to perform additional functions.</p>");
    QWhatsThis::add(m_nickListView, nickListViewWT);
        
    m_tooltip = new Konversation::KonversationNicksOnlineToolTip(m_nickListView->viewport(), this);
        
    
    #ifndef USE_MDI
    setMargin(KDialog::marginHint());
    setSpacing(KDialog::spacingHint());
    #endif
    
    QHBox* buttonBox=new QHBox(this);
    buttonBox->setSpacing(KDialog::spacingHint());
    QPushButton* editButton=new QPushButton(i18n("&Edit Watch List..."),
        buttonBox,"edit_notify_button");
    QString editButtonWT = i18n(
        "Click to edit the list of nicknames that appear on this screen.");
    QWhatsThis::add(editButton, editButtonWT);
    
    connect(editButton, SIGNAL(clicked()), SIGNAL(editClicked()) );
    connect(m_nickListView, SIGNAL(doubleClicked(QListViewItem*)),
        this,SLOT(processDoubleClick(QListViewItem*)));
    
    QLabel* addressbookLabel = new QLabel(i18n("Address Book:"),
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
        
    setupAddressbookButtons(0);
    
    // Create context menu.  Individual menu entries are created in rightButtonClicked slot.
    m_popupMenu = new QPopupMenu(this,"nicksonline_context_menu");
    connect(m_nickListView, SIGNAL(rightButtonClicked(QListViewItem *, const QPoint &, int )),
        this, SLOT(slotNickListView_RightButtonClicked(QListViewItem*, const QPoint &)));
        
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

KListView* NicksOnline::getNickListView() {
  return m_nickListView;
}
    

/**
* Returns the named child of parent item in a KListView.
* @param parent            Pointer to a QListViewItem.
* @param name              The name in the desired child QListViewItem.  Name
*                          is assumed to be in column 0 of the item.
* @return                  Pointer to the child QListViewItem or 0 if not found.
*/
QListViewItem* NicksOnline::findItemChild(const QListViewItem* parent, const QString& name)
{
    if (!parent) return 0;
    QListViewItem* child;
    for (child = parent->firstChild(); (child) ; child = child->nextSibling())
    {
        if (child->text(0) == name) return child;
    }
    return 0;
}

/**
* Return a string containing formatted additional information about a nick.
* @param nickInfo          A pointer to NickInfo structure for the nick.
* @return                  A string formatted for display containing the information
*                          about the nick.
*/
QString NicksOnline::getNickAdditionalInfo(NickInfoPtr nickInfo)
{
    QString nickAdditionalInfo;
    if (nickInfo->isAway())
    {
        nickAdditionalInfo = nickAdditionalInfo + i18n("Away");
        if (!nickInfo->getAwayMessage().isEmpty())
        nickAdditionalInfo = nickAdditionalInfo + "(" + nickInfo->getAwayMessage() + ")";
    }
    if (!nickInfo->getHostmask().isEmpty())
        nickAdditionalInfo = nickAdditionalInfo + " " + nickInfo->getHostmask();
    if (!nickInfo->getRealName().isEmpty())
        nickAdditionalInfo = nickAdditionalInfo + " (" + nickInfo->getRealName() + ")";
    if (!nickInfo->getNetServer().isEmpty())
    {
        nickAdditionalInfo = nickAdditionalInfo + " online via " + nickInfo->getNetServer();
        if (!nickInfo->getNetServerInfo().isEmpty())
        nickAdditionalInfo = nickAdditionalInfo + " (" + nickInfo->getNetServerInfo() + ")";
    }
    if (!nickInfo->getOnlineSince().isNull())
        nickAdditionalInfo = nickAdditionalInfo + " since " + nickInfo->getOnlineSince().toString(Qt::LocalDate);
    return nickAdditionalInfo;
}

/**
* Refresh the nicklistview for a single server.
* @param server            The server to be refreshed.
*/
void NicksOnline::updateServerOnlineList(Server* servr)
{
    // Get a green LED for flagging of joined channels.
    Images leds;
    QIconSet currentLeds = leds.getGreenLed(false);
    QPixmap joinedLed = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);
    bool newGroupRoot = false;
    QString serverName = servr->getServerName();
    QString groupName = servr->getServerGroup();
    QListViewItem* groupRoot = m_nickListView->findItem(groupName, nlvcGroup);
    // If server is not in our list, add it.
    if (!groupRoot)
    {
        groupRoot = new KListViewItem(m_nickListView,groupName);
        newGroupRoot = true;
    }
    // Store server name in hidden column.
    // Note that there could be more than one server in the group connected,
    // but it doesn't matter because all the servers in a group have the same
    // watch list.
    groupRoot->setText(nlvcServerName, serverName);
    // Update list of servers in the group that are connected.
    QStringList serverList = QStringList::split(",", groupRoot->text(nlvcAdditionalInfo));
    if (!serverList.contains(serverName)) serverList.append(serverName);
    groupRoot->setText(nlvcAdditionalInfo, serverList.join(","));
    // Get item in nicklistview for the Offline branch.
    QListViewItem* offlineRoot = findItemChild(groupRoot, c_i18nOffline);
    if (!offlineRoot) offlineRoot = new KListViewItem(groupRoot, c_i18nOffline);
    offlineRoot->setText(nlvcServerName, serverName);
    // Get watch list.
    QStringList watchList = servr->getWatchList();
    for (unsigned int nickIndex = 0; nickIndex<watchList.count(); nickIndex++)
    {
        QString nickname = watchList[nickIndex];
        NickInfoPtr nickInfo = getOnlineNickInfo(groupName, nickname);
        if (nickInfo)
        {
            // Nick is online.
            // Which server did NickInfo come from?
            Server* server=nickInfo->getServer();
            // Construct additional information string for nick.
            QString nickAdditionalInfo = getNickAdditionalInfo(nickInfo);
            // Remove from offline branch if present.
            QListViewItem* item = findItemChild(offlineRoot, nickname);
            if (item) delete item;
            // Add to group if not already added.
            QListViewItem* nickRoot = findItemChild(groupRoot, nickname);
            if (!nickRoot) nickRoot = new KListViewItem(groupRoot, nickname, nickAdditionalInfo);
            nickRoot->setText(nlvcAdditionalInfo, nickAdditionalInfo);
            nickRoot->setText(nlvcServerName, serverName);
            // If no additional info available, request a WHOIS on the nick.
            if (!m_whoisRequested)
            {
                if (nickAdditionalInfo.isEmpty())
                {
                    requestWhois(groupName, nickname);
                    m_whoisRequested = true;
                }
            }
            // Set Kabc icon if the nick is associated with an addressbook entry.
            if (!nickInfo->getAddressee().isEmpty())
            nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                QIconSet::Small, QIconSet::Normal, QIconSet::On));
            else
            nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                QIconSet::Small, QIconSet::Disabled, QIconSet::Off));
            QStringList channelList = server->getNickChannels(nickname);
            for (unsigned int channelIndex=0; channelIndex<channelList.count(); channelIndex++)
            {
                // Known channels where nickname is online and mode in each channel.
                // FIXME: If user connects to multiple servers in same network, the
                // channel info will differ between the servers, resulting in inaccurate
                // mode and led info displayed.
                QString channelName = channelList[channelIndex];
                ChannelNickPtr channelNick = server->getChannelNick(channelName, nickname);
                QString nickMode;
                if (channelNick->hasVoice()) nickMode = nickMode + i18n(" Voice");
                if (channelNick->isHalfOp()) nickMode = nickMode + i18n(" HalfOp");
                if (channelNick->isOp()) nickMode = nickMode + i18n(" Operator");
                if (channelNick->isOwner()) nickMode = nickMode + i18n(" Owner");
                if (channelNick->isAdmin()) nickMode = nickMode + i18n(" Admin");
                QListViewItem* channelItem = findItemChild(nickRoot, channelName);
                if (!channelItem) channelItem = new KListViewItem(nickRoot,
                    channelName, nickMode);
                channelItem->setText(nlvcAdditionalInfo, nickMode);
            
                if (server->getJoinedChannelMembers(channelName) != 0)
                    channelItem->setPixmap(nlvcChannel, joinedLed);
                else
                    channelItem->setPixmap(nlvcChannel, 0);
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
          QListViewItem* item = findItemChild(groupRoot, nickname);
          if (item) delete item;
          // Add to offline list if not already listed.
          QListViewItem* nickRoot = findItemChild(offlineRoot, nickname);
          if (!nickRoot) nickRoot = new KListViewItem(offlineRoot, nickname);
          nickRoot->setText(nlvcServerName, serverName);
        }
    }
    // Erase nicks no longer being watched.
    QListViewItem* item = groupRoot->firstChild();
    while (item)
    {
      QListViewItem* nextItem = item->nextSibling();
      QString nickname = item->text(nlvcNick);
      if (nickname != c_i18nOffline)
      {
        if (watchList.find(nickname) == watchList.end()) delete item;
      }
      item = nextItem;
    }
    item = offlineRoot->firstChild();
    while (item)
    {
      QListViewItem* nextItem = item->nextSibling();
      QString nickname = item->text(nlvcNick);
      if (watchList.find(nickname) == watchList.end()) delete item;
      item = nextItem;
    }
    // Expand server if newly added to list.
    if (newGroupRoot) 
    {
        groupRoot->setOpen(true);
        // Connect server NickInfo updates.
        connect (servr, SIGNAL(nickInfoChanged(Server*, const NickInfoPtr)),
            this, SLOT(slotNickInfoChanged(Server*, const NickInfoPtr)));
    }
}

/**
* Determines if a nick is online in any of the servers in a group and returns
* a NickInfo if found, otherwise 0.
* @param groupName          Server group name.
* @param nickname           Nick name.
* @return                   NickInfo if nick is online in any server, otherwise 0.
*/
NickInfoPtr NicksOnline::getOnlineNickInfo(QString& groupName, QString& nickname)
{
    // Get list of pointers to all servers.
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    QPtrList<Server> serverList = konvApp->getServerList();
    for (Server* server = serverList.first(); server; server = serverList.next())
    {
        if (server->getServerGroup() == groupName)
        {
            NickInfoPtr nickInfo = server->getNickInfo(nickname);
            if (nickInfo) return nickInfo;
        }
    }
    return 0;
}

/**
* Requests a WHOIS in all servers for a specified server group and nickname.
* @param groupName          Server group name.
* @param nickname           Nick name.
*/
void NicksOnline::requestWhois(QString& groupName, QString& nickname)
{
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    QPtrList<Server> serverList = konvApp->getServerList();
    for (Server* server = serverList.first(); server; server = serverList.next())
    {
        if (server->getServerGroup() == groupName) server->requestWhois(nickname);
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
        QString groupName = child->text(nlvcGroup);
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
                    (server->getServerGroup() == groupName)) found = true;
            }
            if (!found)
                it = serverNameList.remove(it);
            else
                ++it;
        }
        // Remove groups with no servers connected, otherwise update list of connected
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
    m_nickListView->hideColumn(nlvcServerName);
    m_nickListView->adjustColumn(nlvcGroupNickChannel);
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

#ifdef USE_MDI
void NicksOnline::closeYourself(ChatWindow*)
{
    emit chatWindowCloseRequest(this);
}
#endif

/**
* Returns the server name and nickname of the specified nicklistview item.
* @param item              The nicklistview item.
* @return serverName       Name of the server for the nick at the item, or Null if not a nick.
* @return nickname         The nickname at the item.
*/
bool NicksOnline::getItemServerAndNick(const QListViewItem* item, QString& serverName, QString& nickname)
{
    if (!item) return false;
    serverName = item->text(nlvcServerName);
    // If on a channel, move up to the nickname.
    if (serverName.isEmpty())
    {
      item = item->parent();
      serverName = item->text(nlvcServerName);
    }
    nickname = item->text(nlvcNick);
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
    QString groupName = server->getServerGroup();
    QListViewItem* groupRoot = m_nickListView->findItem(groupName, nlvcGroup);
    if (!groupRoot) return 0;
    QListViewItem* nickRoot = findItemChild(groupRoot, nickname);
    return nickRoot;
}

/**
* Invokes the KAddressBook contact editor for the specified contact id.
* @param uid               Id of the contact.
* @return                  False if unable to invoke the Contact editor.
*/
bool NicksOnline::editAddressee(const QString &uid)
{
    Q_ASSERT(!uid.isEmpty());
    KProcess *proc = new KProcess;
    *proc << "kaddressbook";
    *proc << "--editor-only" << "--uid" << uid;
    kdDebug() << "running kaddressbook --editor-only --uid " << uid << endl;
    if(!proc->start()) {
        KMessageBox::error(this, "Could not run your addressbook program (kaddressbook).  This is most likely because it isn't installed.  Please install the 'kdepim' packages.");
        return false;
    }
    return true;
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
    if (id < 0) return;
    QString serverName;
    QString nickname;
    QListViewItem* item = m_nickListView->selectedItem();
    if (!getItemServerAndNick(item, serverName, nickname)) return;
    // Get the server object corresponding to the server name.
    KonversationApplication *konvApp = 
        static_cast<KonversationApplication *>(KApplication::kApplication());
    Server* server = konvApp->getServerByName(serverName);
    if (!server) return;
    // Get NickInfo object corresponding to the nickname.
    NickInfoPtr nickInfo = server->getNickInfo(nickname);
    if (!nickInfo) return;
    
    switch(id)
    {
        case ciAddressbookEdit:
        {
            editAddressee(nickInfo->getAddressee().uid());
            break;
        }
        case ciAddressbookNew:
        case ciAddressbookDelete:
        {
            Konversation::Addressbook *addressbook = Konversation::Addressbook::self();
            if(addressbook->getAndCheckTicket())
            {
            if(id == ciAddressbookDelete) {
                KABC::Addressee addr = nickInfo->getAddressee();
                addressbook->unassociateNick(addr, nickname, server->getServerName(), server->getServerGroup());
            } else {
                KABC::Addressee addr;
                addr.setGivenName(nickname);
                addr.setNickName(nickname);
                addressbook->associateNickAndUnassociateFromEveryoneElse(addr, nickname, server->getServerName(), server->getServerGroup());
            }
            if(addressbook->saveTicket())
            {
                //saveTicket will refresh the addressees for us.
                if(id == ciAddressbookNew)
                if(!editAddressee(nickInfo->getAddressee().uid())) break;
            }
            }
            break;
        }
        case ciAddressbookChange:
        {
            LinkAddressbookUI *linkaddressbookui = new LinkAddressbookUI(this, NULL, nickname, server->getServerName(), server->getServerGroup(), nickInfo->getRealName());
            linkaddressbookui->show();
            break;
        }
    }
    slotNickInfoChanged(server, nickInfo);
}

/**
* Get the addressbook state of the nickname at the specified nicklistview item.
* @param item              Item of the nicklistview.
* @return                  Addressbook state.
* 0 = not a nick, 1 = nick has no addressbook association, 2 = nick has association
*/
int NicksOnline::getNickAddressbookState(QListViewItem* item)
{
    int nickState = 0;
    QString serverName;
    QString nickname;
    if (getItemServerAndNick(item, serverName, nickname))
    {
        Server *server = static_cast<KonversationApplication *>(kapp)->getServerByName(serverName);
        if (!server) return 0;
        NickInfoPtr nickInfo = server->getNickInfo(nickname);
        if (!nickInfo) return 0;
        if (nickInfo->getAddressee().isEmpty())
        nickState = 1;
        else
        nickState =2;
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
        case 0:
        {
            m_editContactButton->setEnabled(false);
            m_changeAssociationButton->setEnabled(false);
            m_deleteAssociationButton->setEnabled(false);
            break;
        }
        case 1:
        {
            m_editContactButton->setText(i18n("New C&ontact..."));
            m_editContactButton->setEnabled(true);
            m_changeAssociationButton->setText(i18n("&Choose Association..."));
            m_changeAssociationButton->setEnabled(true);
            m_deleteAssociationButton->setEnabled(false);
            break;
        }
        case 2:
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
        case 0: break;
        case 1: { doCommand(ciAddressbookNew); break; }
        case 2: { doCommand(ciAddressbookEdit); break; }
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
        case 0:
        {
            break;
        }
        case 1:
        {
            m_popupMenu->insertItem(i18n("&Choose Association..."), ciAddressbookChange);
            m_popupMenu->insertItem(i18n("New C&ontact..."), ciAddressbookNew);
            break;
        }
        case 2:
        {
            m_popupMenu->insertItem(i18n("Edit C&ontact..."), ciAddressbookEdit);
            m_popupMenu->insertSeparator();
            m_popupMenu->insertItem(i18n("&Change Association..."), ciAddressbookChange);
            m_popupMenu->insertItem(i18n("&Delete Association"), ciAddressbookDelete);
            break;
        }
    }
    if (nickState != 0)
    {
        // TODO: Does this block the main event loop?
        int r = m_popupMenu->exec(pt);
        doCommand(r);
    }
}

/**
* Received from server when a NickInfo changes its information.
*/
void NicksOnline::slotNickInfoChanged(Server* server, const NickInfoPtr nickInfo)
{
    if (!nickInfo) return;
    QString nickname = nickInfo->getNickname();
    
    kdDebug() << "NicksOnline::slotNickInfoChanged: nickname: " << nickname << endl;
    
    if (!server) return;
    QString serverName = server->getServerName();
    QListViewItem* item = getServerAndNickItem(serverName, nickname);
    if (!item) return;
    int nickState = 2;
    NickInfoPtr nickyInfo = nickInfo;
    if (nickyInfo->getAddressee().isEmpty()) nickState = 1;
    switch (nickState)
    {
        case 0: break;
        case 1: { item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                QIconSet::Small, QIconSet::Disabled, QIconSet::Off)); break; }
        case 2: { item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
                QIconSet::Small, QIconSet::Normal, QIconSet::On)); break; }
    }
    QString nickAdditionalInfo = getNickAdditionalInfo(nickInfo);
    item->setText(nlvcAdditionalInfo, nickAdditionalInfo);
    if (item == m_nickListView->selectedItem()) setupAddressbookButtons(nickState);
}
void NicksOnline::childAdjustFocus()
{
}

void NicksOnline::setOnlineList(const QString& serverName,const QStringList& /*list*/,
bool /*changed*/)
{
  Server *server = static_cast<KonversationApplication *>(kapp)->getServerByName(serverName);
  updateServerOnlineList(server);
}

#include "nicksonline.moc"
