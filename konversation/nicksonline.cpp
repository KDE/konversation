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

NicksOnline::NicksOnline(QWidget* parent): ChatWindow(parent)
{  
  setName(i18n("Watched Nicks Online"));
  setType(ChatWindow::NicksOnline);

  nickListView=new KListView(this);
  
  nickListView->addColumn(i18n("Nickname"));
  nickListView->setRootIsDecorated(false);
  nickListView->setFullWidth(true);
  
  setMargin(KDialog::marginHint());
  setSpacing(KDialog::spacingHint());

  QHBox* buttonBox=new QHBox(this);
  buttonBox->setSpacing(KDialog::spacingHint());

  QPushButton* editButton=new QPushButton(i18n("&Edit..."),buttonBox,"edit_notify_button");
  QPushButton* closeButton=new QPushButton(i18n("&Close"),buttonBox,"close_nicksonline_window");

  connect(editButton,SIGNAL (clicked()),SIGNAL (editClicked()) );
  connect(closeButton,SIGNAL (clicked()),this,SLOT (closeButton()) );
  connect(nickListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT(processDoubleClick(QListViewItem*)));
}

NicksOnline::~NicksOnline()
{
  delete nickListView;
}

void NicksOnline::setOnlineList(const QString& serverName,const QStringList& list,bool changed)
{
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
}

void NicksOnline::closeEvent(QCloseEvent* ce)
{
  ce->accept();
  closeButton();
}

void NicksOnline::closeButton()
{
  emit closeClicked(size());
}

void NicksOnline::processDoubleClick(QListViewItem* item)
{
  // only emit signal when the user double clicked a nickname rather than a server name
  if(item->parent())
    emit doubleClicked(item->parent()->text(0),item->text(0));
}

void NicksOnline::adjustFocus()
{
}

#include "nicksonline.moc"
