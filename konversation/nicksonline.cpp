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

#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>
#include <klistview.h>

#include "nicksonline.h"
#include "server.h"
#include "konversationapplication.h"

#ifdef USE_NICKINFO
#include "images.h"
#endif

NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
{  
  setName(i18n("Watched Nicks Online"));
  setType(ChatWindow::NicksOnline);

  nickListView=new KListView(this);

#ifdef USE_NICKINFO
  nickListView->addColumn(i18n("Server/Nickname/Channel"));
  nickListView->addColumn(i18n("Additional Information"));
  nickListView->setFullWidth(false);
  nickListView->setRootIsDecorated(true);
#else
  nickListView->addColumn(i18n("Server/Nickname"));
  nickListView->setFullWidth(true);
  nickListView->setRootIsDecorated(false);
#endif
  
  setMargin(KDialog::marginHint());
  setSpacing(KDialog::spacingHint());

//  QHBox* buttonBox=new QHBox(this);
//  buttonBox->setSpacing(KDialog::spacingHint());

  QPushButton* editButton=new QPushButton(i18n("&Edit..."),this,"edit_notify_button");
  editButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));

  connect(editButton,SIGNAL (clicked()),SIGNAL (editClicked()) );
  connect(nickListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT(processDoubleClick(QListViewItem*)));
  
#ifdef USE_NICKINFO
  // Display info for all currently-connected servers.
  refreshAllServerOnlineLists();
  // Connect and start refresh timer.
  timer = new QTimer(this, "nicksOnlineTimer");
  connect(timer, SIGNAL (timeout()), this, SLOT(timerFired()));
  // TODO: User preference for refresh interval.
  timer->start(8000);
#endif
}

NicksOnline::~NicksOnline()
{
#ifdef USE_NICKINFO
  timer->stop();
  delete timer;
#endif
  delete nickListView;
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

#ifdef USE_NICKINFO
void NicksOnline::updateServerOnlineList(Server* server, bool changed)
{
  bool whoisRequested = false;
  bool newServerRoot = false;
  QString nickname;
  QListViewItem* child;
  QListViewItem* nextChild;
  QString serverName = server->getServerName();
  QListViewItem* serverRoot = nickListView->findItem(serverName,0);
  // If server is not in our list, or if the list changed, then display the new list.
  if ( true )
  {
    if (!serverRoot)
    {
      serverRoot = new KListViewItem(nickListView,serverName);
      newServerRoot = true;
    }
    // Get a green LED for flagging of joined channels.
    Images leds;
    QIconSet currentLeds = leds.getGreenLed(false);
    QPixmap joinedLed = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);
    // List online nicknames.
    const NickInfoList* nickInfoList = server->getNicksOnline();
    NickInfoListIterator itOnline(*nickInfoList);
    NickInfo* nickInfo;
    for ( ; (nickInfo=itOnline.current()) ; ++itOnline)
    {
      QString lcNickName = itOnline.currentKey();
      nickname = nickInfo->getNickname();
      // Construct additional information string for nick.
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
      else
      {
        // Request additional info on the nick, but only one at a time.
        if (!whoisRequested)
        {
          server->requestWhois(nickname);
          whoisRequested = true;
        }
      }
      if (!nickInfo->getOnlineSince().isNull())
        nickAdditionalInfo = nickAdditionalInfo + " since " + nickInfo->getOnlineSince().toString(Qt::LocalDate);
      
      QListViewItem* nickRoot = findItemChild(serverRoot, nickname);
      if (!nickRoot) nickRoot = new KListViewItem(serverRoot, nickname, nickAdditionalInfo);
      nickRoot->setText(1, nickAdditionalInfo);
      
      QStringList channelList = server->getNickChannels(nickname);
      for ( unsigned int index=0; index<channelList.count(); index++ )
      {
        // Known channels where nickname is online and mode in each channel.
        QString channelName = channelList[index];
        ChannelNick* channelNick = server->getChannelNick(channelName, lcNickName);
        unsigned int nickModeWord = channelNick->mode;
        QString nickMode;
        if (nickModeWord & 1) nickMode = nickMode + i18n(" Voice");
        nickModeWord >>= 1;
        if (nickModeWord & 1) nickMode = nickMode + i18n(" HalfOp");
        nickModeWord >>= 1;
        if (nickModeWord & 1) nickMode = nickMode + i18n(" Operator");
        nickModeWord >>= 1;
        if (nickModeWord & 1) nickMode = nickMode + i18n(" Owner");
        nickModeWord >>= 1;
        if (nickModeWord & 1) nickMode = nickMode + i18n(" Admin");
        QListViewItem* channelItem = findItemChild(nickRoot, channelName);
        if (!channelItem) channelItem = new KListViewItem(nickRoot, channelName, nickMode);
        channelItem->setText(1, nickMode);
        
        if (server->getJoinedChannelMembers(channelName) != 0)
        {
          channelItem->setPixmap(0, joinedLed);
        }
        else
        {
          channelItem->setPixmap(0, 0);
        }
      }
      // Remove channel if nick no longer in it.
      child = nickRoot->firstChild();
      while (child)
      {
        nextChild = child->nextSibling();
        if (channelList.find(child->text(0)) == channelList.end()) delete child;
        child = nextChild;
      }
    }
    // Remove nicks from list if no longer online.
    child = serverRoot->firstChild();
    while (child)
    {
      nextChild = child->nextSibling();
      if (!nickInfoList->find(child->text(0))) delete child;
      child = nextChild;
    }
    // List offline nicknames.
    QListViewItem* offlineRoot = findItemChild(serverRoot, i18n("Offline"));
    if (!offlineRoot) offlineRoot = new KListViewItem(serverRoot, i18n("Offline"));
    nickInfoList = server->getNicksOffline();
    NickInfoListIterator itOffline(*nickInfoList);
    for ( ; (nickInfo=itOffline.current()) ; ++itOffline)
    {
      nickname = nickInfo->getNickname();
      if (!findItemChild(offlineRoot, nickname))
      {
        new KListViewItem(offlineRoot, nickname);
      }
    }
    // Remove nick from list if no longer in offline list.
    child = offlineRoot->firstChild();
    while (child)
    {
      nextChild = child->nextSibling();
      if (!nickInfoList->find(child->text(0))) delete child;
      child = nextChild;
    }
    // Expand server if newly added to list.
    if (newServerRoot) serverRoot->setOpen(true);
    nickListView->adjustColumn(0);
    nickListView->adjustColumn(1);
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
  QListViewItem* child = nickListView->firstChild();
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

void NicksOnline::setOnlineList(const QString& serverName,const QStringList& list,bool changed)
{
#ifdef USE_NICKINFO
  // Get the server object corresponding to the server name.
  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  Server* server = konvApp->getServerByName(serverName);
  updateServerOnlineList(server, changed);
#else

  QListViewItem* serverRoot=nickListView->findItem(serverName,0);
  // If server is not in our list, or if the list changed, then display the new list.
  if ( (serverRoot == 0) || changed)
  {
    delete serverRoot;  
    if(list.count())
    {
      KListViewItem* newServerRoot=new KListViewItem(nickListView,serverName);
      for(unsigned int i=list.count();i!=0;i--)
      {
        new KListViewItem(newServerRoot,list[i-1]);
      }
      newServerRoot->setOpen(true);
    }
  }
#endif
}

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

void NicksOnline::adjustFocus()
{
}

#include "nicksonline.moc"
