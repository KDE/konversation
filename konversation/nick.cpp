/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nick.cpp  -  description
  begin:     Fri Jan 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kdebug.h>
#include <klistview.h>

#include "ledlistviewitem.h"
#include "nick.h"
#include "addressbook.h"

Nick::Nick(KListView* listView,
           const QString& newName,
           const QString& newMask,
           bool admin,
           bool owner,
           bool op,
           bool halfop,
           bool voice)
{
  addressee=Konversation::Addressbook::self()->getKABCAddresseeFromNick(newName);
  QString realname = addressee.realName();
  if(!realname.isEmpty() && realname.lower() != newName.lower())
  	listViewItem=new LedListViewItem(listView,newName + " (" + realname + ")",newMask,admin,owner,op,halfop,voice);
  else
	listViewItem=new LedListViewItem(listView,newName,newMask,admin,owner,op,halfop,voice);
  nickname=newName;
  hostmask=newMask;

  setAdmin(admin);
  setOwner(owner);
  setOp(op);
  setHalfop(halfop);
  setVoice(voice);
}

Nick::~Nick()
{
  delete listViewItem;
}

void Nick::setNickname(const QString& newName)
{
  KABC::Addressee newaddressee = Konversation::Addressbook::self()->getKABCAddresseeFromNick(newName);
  
  if(addressee.isEmpty() && !newaddressee.isEmpty()) //We now know who this person is
    Konversation::Addressbook::self()->associateNick(newaddressee,nickname);  //Associate the old nickname with new contact
  else if(!addressee.isEmpty() && newaddressee.isEmpty()) {
    Konversation::Addressbook::self()->associateNick(addressee, newName);
    newaddressee = addressee;
  }

  addressee = newaddressee;
  nickname = newName;
  
  QString realname = addressee.realName();
  if(!realname.isEmpty() && realname.lower() != newName.lower())
    listViewItem->setText(1,newName + " (" + realname + ")");
  else 
    listViewItem->setText(1,newName);
}

void Nick::refreshAddressee() {
  addressee = Konversation::Addressbook::self()->getKABCAddresseeFromNick(nickname); 
  QString realname = addressee.realName();

  kdDebug() << "refreshing addressee nick '" << nickname << "' and found realname '" << realname << "'" << endl;
  if(!realname.isEmpty() && realname.lower() != nickname.lower())
    listViewItem->setText(1,nickname + " (" + realname + ")");
  else
    listViewItem->setText(1,nickname);     
}

void Nick::setHostmask(const QString& newMask)
{
  hostmask=newMask;
  listViewItem->setText(2,hostmask);
}

void Nick::setAdmin(bool state)
{
  admin=state;
  listViewItem->setState(admin,owner,op,halfop,voice);
}

void Nick::setOwner(bool state)
{
  owner=state;
  listViewItem->setState(admin,owner,op,halfop,voice);
}

void Nick::setOp(bool state)
{
  op=state;
  listViewItem->setState(admin,owner,op,halfop,voice);
}

void Nick::setHalfop(bool state)
{
  halfop=state;
  listViewItem->setState(admin,owner,op,halfop,voice);
}

void Nick::setVoice(bool state)
{
  voice=state;
  listViewItem->setState(admin,owner,op,halfop,voice);
}

bool Nick::isAdmin()  { return admin; }
bool Nick::isOwner()  { return owner; }
bool Nick::isOp()     { return op; }
bool Nick::isHalfop() { return halfop; }
bool Nick::hasVoice() { return voice; }

QString Nick::getNickname() { return nickname; }
QString Nick::getHostmask() { return hostmask; }

bool Nick::isSelected() { return listViewItem->isSelected(); }
