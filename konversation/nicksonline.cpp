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

#include <qlayout.h>
#include <qstringlist.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kprocess.h>
#include <kmessagebox.h>

#include "nicksonline.h"
#include "server.h"
#include "konversationapplication.h"
#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"

#ifdef USE_NICKINFO
#include "images.h"
#endif

#ifdef USE_MDI
NicksOnline::NicksOnline(QString caption): ChatWindow(caption)
#else
NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
#endif
{
  setName(i18n("Watched Nicks Online"));
  setType(ChatWindow::NicksOnline);

  m_nickListView=new KListView(this);

  // TODO: Need to derive from KListView and override sort() method in order to sort in
  // locale-aware order.

#ifdef USE_NICKINFO
  m_nickListView->addColumn(i18n("Server/Nickname/Channel"));
  m_kabcIconSet = KGlobal::iconLoader()->loadIconSet("kaddressbook",KIcon::Small);
  m_nickListView->addColumn(m_kabcIconSet, QString::null);
  m_nickListView->addColumn(i18n("Additional Information"));
  m_nickListView->setFullWidth(false);
  m_nickListView->setRootIsDecorated(true);
#else
  m_nickListView->addColumn(i18n("Server/Nickname"));
  m_nickListView->setFullWidth(true);
  m_nickListView->setRootIsDecorated(false);
#endif

#ifndef USE_MDI
  setMargin(KDialog::marginHint());
  setSpacing(KDialog::spacingHint());
#endif

#if USE_NICKINFO
  QHBox* buttonBox=new QHBox(this);
  buttonBox->setSpacing(KDialog::spacingHint());
  QPushButton* editButton=new QPushButton(i18n("&Edit Watch List..."),
    buttonBox,"edit_notify_button");
#else
  QPushButton* editButton=new QPushButton(i18n("&Edit..."),this,"edit_notify_button");
  editButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
#endif

  connect(editButton, SIGNAL(clicked()), SIGNAL(editClicked()) );
  connect(m_nickListView, SIGNAL(doubleClicked(QListViewItem*)),
    this,SLOT(processDoubleClick(QListViewItem*)));

#ifdef USE_NICKINFO
  QLabel* addressbookLabel = new QLabel(i18n("Addressbook:"),
    buttonBox, "nicksonline_addressbook_label");
  addressbookLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_editContactButton = new QPushButton(i18n("Edit C&ontact..."),
    buttonBox, "nicksonline_editcontact_button");
  m_changeAssociationButton = new QPushButton(i18n("&Change Association..."),
    buttonBox, "nicksonline_changeassociation_button");
  m_deleteAssociationButton = new QPushButton(i18n("&Delete Association..."),
    buttonBox, "nicksonline_deleteassociation_button");
    
  connect(m_editContactButton, SIGNAL(clicked()),
    this, SLOT(slotEditContactButton_Clicked()));
  connect(m_changeAssociationButton, SIGNAL(clicked()),
    this, SLOT(slotChangeAssociationButton_Clicked()));
  connect(m_deleteAssociationButton, SIGNAL(clicked()),
    this, SLOT(slotDeleteAssociationButton_Clicked()));
  connect(m_nickListView, SIGNAL(selectionChanged()),
    this, SLOT(slotNickListView_SelectionChanged()));
    
  setupAddressbookButtons(0);

  // Display info for all currently-connected servers.
  refreshAllServerOnlineLists();
  // Connect and start refresh timer.
  m_timer = new QTimer(this, "nicksOnlineTimer");
  connect(m_timer, SIGNAL (timeout()), this, SLOT(timerFired()));
  // TODO: User preference for refresh interval.
  m_timer->start(8000);
#endif
}

NicksOnline::~NicksOnline()
{
#ifdef USE_NICKINFO
  m_timer->stop();
  delete m_timer;
#endif
  delete m_nickListView;
}

// Returns the named child of parent item in KListView.
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

// Return a string contained formatted additional information about a nick.
#ifdef USE_NICKINFO
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
#else
QString NicksOnline::getNickAdditionalInfo(NickInfoPtr /*nickInfo*/) { return QString::null; }
#endif

#ifdef USE_NICKINFO
void NicksOnline::updateServerOnlineList(Server* server, bool)
{
  bool whoisRequested = false;
  bool newServerRoot = false;
  QString nickname;
  QListViewItem* child;
  QListViewItem* nextChild;
  QString serverName = server->getServerName();
  QListViewItem* serverRoot = m_nickListView->findItem(serverName,nlvcServerNickChannel);
  // If server is not in our list, or if the list changed, then display the new list.
  if ( true )
  {
    if (!serverRoot)
    {
      serverRoot = new KListViewItem(m_nickListView,serverName);
      newServerRoot = true;
    }
    // Get a green LED for flagging of joined channels.
    Images leds;
    QIconSet currentLeds = leds.getGreenLed(false);
    QPixmap joinedLed = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);
    // List online nicknames.
    const NickInfoMap* nickInfoList = server->getNicksOnline();
    NickInfoMap::ConstIterator itOnline;
    NickInfoPtr nickInfo;
    QListViewItem* nickRoot;
    for ( itOnline = nickInfoList->begin(); itOnline != nickInfoList->end() ; ++itOnline)
    {
      QString lcNickName = itOnline.key();
      nickInfo = itOnline.data();
      nickname = nickInfo->getNickname();
      // Construct additional information string for nick.
      QString nickAdditionalInfo = getNickAdditionalInfo(nickInfo);
      if (nickInfo->getNetServer().isEmpty())
      {
        // Request additional info on the nick, but only one at a time.
        if (!whoisRequested)
        {
          server->requestWhois(nickname);
          whoisRequested = true;
        }
      }
        
      nickRoot = findItemChild(serverRoot, nickname);
      if (!nickRoot) nickRoot = new KListViewItem(serverRoot, nickname, nickAdditionalInfo);
      nickRoot->setText(nlvcAdditionalInfo, nickAdditionalInfo);
      
      // Set Kabc icon if the nick is associated with an addressbook entry.
      if (!Konversation::Addressbook::self()->getKABCAddresseeFromNick(nickname).isEmpty())
        nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
          QIconSet::Small, QIconSet::Normal, QIconSet::On));
      else
        nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
          QIconSet::Small, QIconSet::Disabled, QIconSet::Off));

      QStringList channelList = server->getNickChannels(nickname);
      for ( unsigned int index=0; index<channelList.count(); index++ )
      {
        // Known channels where nickname is online and mode in each channel.
        QString channelName = channelList[index];
        ChannelNickPtr channelNick = server->getChannelNick(channelName, lcNickName);
        QString nickMode;
        if (channelNick->hasVoice()) nickMode = nickMode + i18n(" Voice");
        if (channelNick->isHalfOp()) nickMode = nickMode + i18n(" HalfOp");
        if (channelNick->isOp()) nickMode = nickMode + i18n(" Operator");
        if (channelNick->isOwner()) nickMode = nickMode + i18n(" Owner");
        if (channelNick->isAdmin()) nickMode = nickMode + i18n(" Admin");
        QListViewItem* channelItem = findItemChild(nickRoot, channelName);
        if (!channelItem) channelItem = new KListViewItem(nickRoot, channelName, nickMode);
        channelItem->setText(nlvcAdditionalInfo, nickMode);

        if (server->getJoinedChannelMembers(channelName) != 0)
        {
          channelItem->setPixmap(nlvcServerNickChannel, joinedLed);
        }
        else
        {
          channelItem->setPixmap(nlvcServerNickChannel, 0);
        }
      }
      // Remove channel if nick no longer in it.
      child = nickRoot->firstChild();
      while (child)
      {
        nextChild = child->nextSibling();
        if (channelList.find(child->text(nlvcServerNickChannel)) == channelList.end())
          delete child;
        child = nextChild;
      }
    }
    QString i18nOffline = i18n("Offline");
    // Remove nicks from list if no longer online.
    child = serverRoot->firstChild();
    while (child)
    {
      nextChild = child->nextSibling();
      if (!nickInfoList->contains(child->text(nlvcServerNickChannel).lower()) &&
        (child->text(nlvcServerNickChannel) != i18nOffline))
          delete child;
      child = nextChild;
    }
    // List offline nicknames.
    QListViewItem* offlineRoot = findItemChild(serverRoot, i18nOffline);
    if (!offlineRoot) offlineRoot = new KListViewItem(serverRoot, i18nOffline);
    nickInfoList = server->getNicksOffline();
    NickInfoMap::ConstIterator itOffline;
    for ( itOffline = nickInfoList->begin(); itOffline != nickInfoList->end() ; ++itOffline)
    {
      nickInfo = itOffline.data();
      nickname = nickInfo->getNickname();
      nickRoot = findItemChild(offlineRoot, nickname);
      if (!nickRoot)
      {
        nickRoot = new KListViewItem(offlineRoot, nickname);
      }
      QString nickAdditionalInfo = getNickAdditionalInfo(nickInfo);
      nickRoot->setText(nlvcAdditionalInfo, nickAdditionalInfo);
      // Set Kabc icon if the nick is associated with an addressbook entry.
      if (!nickInfo->getAddressee().isEmpty())
        nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
          QIconSet::Small, QIconSet::Normal, QIconSet::On));
      else
        nickRoot->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
          QIconSet::Small, QIconSet::Disabled, QIconSet::Off));
    }
    // Remove nick from list if no longer in offline list.
    child = offlineRoot->firstChild();
    while (child)
    {
      nextChild = child->nextSibling();
      if (!nickInfoList->contains(child->text(nlvcServerNickChannel).lower())) delete child;
      child = nextChild;
    }
    // Expand server if newly added to list.
    if (newServerRoot) serverRoot->setOpen(true);
    m_nickListView->adjustColumn(nlvcServerNickChannel);
    m_nickListView->adjustColumn(nlvcKabc);
    m_nickListView->adjustColumn(nlvcAdditionalInfo);
  }
}
#else
void NicksOnline::updateServerOnlineList(Server*, bool) {}
#endif

void NicksOnline::refreshAllServerOnlineLists()
{
  // Display info for all currently-connected servers.
  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  QPtrList<Server> serverList = konvApp->getServerList();
  Server* server;
  for ( server = serverList.first(); server; server = serverList.next() )
  {
    updateServerOnlineList(server, true);
  }
  // Remove servers no longer connected.
  QListViewItem* child = m_nickListView->firstChild();
  while (child)
  {
    QListViewItem* nextChild = child->nextSibling();
    if (!konvApp->getServerByName(child->text(0))) delete child;
    child = nextChild;
  }
}

void NicksOnline::timerFired()
{
  refreshAllServerOnlineLists();
}

#ifdef USE_NICKINFO
void NicksOnline::setOnlineList(const QString& serverName,const QStringList&,bool changed)
{
  // Get the server object corresponding to the server name.
  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  Server* server = konvApp->getServerByName(serverName);
  updateServerOnlineList(server, changed);
}
#else
void NicksOnline::setOnlineList(const QString& serverName,const QStringList& list,bool changed)
{
  QListViewItem* serverRoot=m_nickListView->findItem(serverName,0);
  // If server is not in our list, or if the list changed, then display the new list.
  if ( (serverRoot == 0) || changed)
  {
    delete serverRoot;
    if(list.count())
    {
      KListViewItem* newServerRoot=new KListViewItem(m_nickListView,serverName);
      for(unsigned int i=list.count();i!=0;i--)
      {
        new KListViewItem(newServerRoot,list[i-1]);
      }
      newServerRoot->setOpen(true);
    }
  }
}
#endif

void NicksOnline::processDoubleClick(QListViewItem* item)
{
  // only emit signal when the user double clicked a nickname rather than a server name or channel name.
  QListViewItem* parentItem = item->parent();
  if(parentItem)
  {
    if (!parentItem->parent())
    {
      emit doubleClicked(parentItem->text(0),item->text(0));
    }
  }
}

#ifdef USE_MDI
void NicksOnline::closeYourself(ChatWindow*)
{
  emit chatWindowCloseRequest(this);
}
#endif

void NicksOnline::adjustFocus()
{
}

// Returns the server name and nickname of the specified nicklistview item.
bool NicksOnline::getItemServerAndNick(const QListViewItem* item, QString& serverName, QString& nickname)
{
    if (!item) return false;
    QListViewItem* parentItem = item->parent();
    if (!parentItem) return false;
    if (parentItem->text(nlvcServerNickChannel) == i18n("Offline"))
        parentItem = parentItem->parent();
    serverName = parentItem->text(nlvcServerNickChannel);
    nickname = item->text(nlvcServerNickChannel);
    return true;
}

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

#ifdef USE_NICKINFO
void NicksOnline::doCommand(int id)
{
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
        //Handle all the selected nicks in one go.  Either they all save, or none do.
        if(addressbook->getAndCheckTicket())
        {
          if(id == ciAddressbookDelete) {
            KABC::Addressee addr = addressbook->getKABCAddresseeFromNick(nickname);
            addressbook->unassociateNick(addr, nickname);
          } else {
            KABC::Addressee addr;
            addr.setGivenName(nickname);
            addr.setNickName(nickname);
            addressbook->associateNickAndUnassociateFromEveryoneElse(addr, nickname);
          }
          if(addressbook->saveTicket())
          {
            //Nicks have changed.  Refresh.
            nickInfo->refreshAddressee();
            if(id == ciAddressbookNew)
              if(!editAddressee(nickInfo->getAddressee().uid())) break;
          }
        }
        break;
      }
    case ciAddressbookChange:
      {
        LinkAddressbookUI *linkaddressbookui = new LinkAddressbookUI(this, NULL, nickname);
        linkaddressbookui->show();
        nickInfo->refreshAddressee();
        break;
      }
  }
  int nickState = getNickAddressbookState(item);
  switch (nickState)
  {
    case 0: break;
    case 1: { item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
              QIconSet::Small, QIconSet::Disabled, QIconSet::Off)); break; }
    case 2: { item->setPixmap(nlvcKabc, m_kabcIconSet.pixmap(
              QIconSet::Small, QIconSet::Normal, QIconSet::On)); break; }
  }
  setupAddressbookButtons(nickState);
}
#else
void NicksOnline::doCommand(int /*id*/) { };
#endif

int NicksOnline::getNickAddressbookState(QListViewItem* item)
{
  // 0 = not a nick, 1 = nick has no addressbook association, 2 = nick has association
  int nickState = 0;
  QString serverName;
  QString nickname;
  if (getItemServerAndNick(item, serverName, nickname))
  {
    if (Konversation::Addressbook::self()->getKABCAddresseeFromNick(nickname).isEmpty())
      nickState = 1;
    else
      nickState = 2;
  }
  return nickState;
}

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

void NicksOnline::slotEditContactButton_Clicked()
{ 
  switch (getNickAddressbookState(m_nickListView->selectedItem()))
  {
    case 0: break;
    case 1: { doCommand(ciAddressbookNew); break; }
    case 2: { doCommand(ciAddressbookEdit); break; }
  }
}

void NicksOnline::slotChangeAssociationButton_Clicked() { doCommand(ciAddressbookChange); }
void NicksOnline::slotDeleteAssociationButton_Clicked() { doCommand(ciAddressbookDelete); }

void NicksOnline::slotNickListView_SelectionChanged()
{
  QListViewItem* item = m_nickListView->selectedItem();
  int nickState = getNickAddressbookState(item);
  setupAddressbookButtons(nickState);
}

#include "nicksonline.moc"
