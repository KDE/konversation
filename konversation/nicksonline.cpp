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

NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
{  
  setName(i18n("Watched Nicks Online"));
  setType(ChatWindow::NicksOnline);

  nickListView=new KListView(this);
  
  nickListView->addColumn(i18n("Server/Nickname"));
  nickListView->setRootIsDecorated(false);
  nickListView->setFullWidth(true);
  
  setMargin(KDialog::marginHint());
  setSpacing(KDialog::spacingHint());

//  QHBox* buttonBox=new QHBox(this);
//  buttonBox->setSpacing(KDialog::spacingHint());

  QPushButton* editButton=new QPushButton(i18n("&Edit..."),this,"edit_notify_button");
  editButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));

  connect(editButton,SIGNAL (clicked()),SIGNAL (editClicked()) );
  connect(nickListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT(processDoubleClick(QListViewItem*)));
}

NicksOnline::~NicksOnline()
{
  delete nickListView;
}

void NicksOnline::setOnlineList(const QString& serverName,const QStringList& list,bool changed)
{
#ifdef USE_NICKINFO
  QListViewItem* serverRoot=nickListView->findItem(serverName,0);
  // If server is not in our list, or if the list changed, then display the new list.
  if ( true )
  {
    delete serverRoot;
    if (nickListView->columns() == 1)
    {
      nickListView->addColumn(i18n("Additional Information"));
      nickListView->setColumnText(0, i18n("Server/Nickname/Channel"));
    }
    KListViewItem* newServerRoot=new KListViewItem(nickListView,serverName);
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    Server* server = konvApp->getServerByName(serverName);
    const NickInfoList* nickInfoList = server->getNicksOnline();
    NickInfoListIterator itOnline(*nickInfoList);
    // Online nicknames.
    NickInfo* nickInfo;
    for ( ; (nickInfo=itOnline.current()) ; ++itOnline)
    {
      QString nickname = nickInfo->getNickname();
      QString nickAdditionalInfo = "";
      if (nickInfo->isAway())
      {
        nickAdditionalInfo = nickAdditionalInfo + i18n("Away");
        QString awayMsg = nickInfo->getAwayMessage();
        if (!awayMsg.isEmpty()) nickAdditionalInfo = nickAdditionalInfo + "(" + awayMsg + ")";
      }
      nickAdditionalInfo = nickAdditionalInfo + " ";
      nickAdditionalInfo = nickAdditionalInfo + nickInfo->getHostmask();
      KListViewItem* nickRoot = new KListViewItem(newServerRoot, nickname, nickAdditionalInfo);
      QStringList channelList = server->getNickChannels(nickname);
      for ( unsigned int index=0; index<channelList.count(); index++ )
      {
        // Known channels where nickname is online.
        new KListViewItem(nickRoot, channelList[index]);
      }
      nickRoot->setOpen(true);
    }
    // Offline nicknames.
    KListViewItem* offlineRoot = new KListViewItem(newServerRoot, i18n("Offline"));
    nickInfoList = server->getNicksOffline();
    NickInfoListIterator itOffline(*nickInfoList);
    for ( ; (nickInfo=itOffline.current()) ; ++itOffline)
    {
      new KListViewItem(offlineRoot, nickInfo->getNickname());
    }
    newServerRoot->setOpen(true);
    offlineRoot->setOpen(true);
    nickListView->adjustColumn(0);
  }

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
