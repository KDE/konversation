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
#include <klocale.h>
#include <qtextstream.h>
#include <kabc/phonenumber.h>


#ifdef USE_NICKINFO
Nick::Nick(KListView *listView,
           ChannelNickPtr channelnick)
{
  listViewItem=new LedListViewItem(listView,channelnick->getNickname(),channelnick->getHostmask(),this);
  setAdmin(channelnick->isAdmin());
  setOwner(channelnick->isOwner());
  setOp(channelnick->isOp());
  setHalfop(channelnick->isHalfOp());
  setVoice(channelnick->hasVoice());
  channelnickptr = channelnick;
}
#else
Nick::Nick(KListView* listView,
           const QString& newName,
           const QString& newMask,
           bool admin,
           bool owner,
           bool op,
           bool halfop,
           bool voice)
{
  listViewItem=new LedListViewItem(listView,newName,newMask,admin,owner,op,halfop,voice, this);
  nickname=newName;
  hostmask=newMask;

  setAdmin(admin);
  setOwner(owner);
  setOp(op);
  setHalfop(halfop);
  setVoice(voice);
}
#endif
Nick::~Nick()
{
  delete listViewItem;
}


void Nick::setAdmin(bool state)
{
  admin=state;
#ifndef USE_NICKINFO
  listViewItem->refresh();
#endif
}

void Nick::setOwner(bool state)
{
  owner=state;
#ifndef USE_NICKINFO
  listViewItem->refresh();
#endif
}

void Nick::setOp(bool state)
{
  op=state;
#ifndef USE_NICKINFO
  listViewItem->refresh();
#endif
}

void Nick::setHalfop(bool state)
{
  halfop=state;
#ifndef USE_NICKINFO
  listViewItem->refresh();
#endif
}

void Nick::setVoice(bool state)
{
  voice=state;
#ifndef USE_NICKINFO
  listViewItem->refresh();
#endif
}




bool Nick::isAdmin()  { return admin; }
bool Nick::isOwner()  { return owner; }
bool Nick::isOp()     { return op; }
bool Nick::isHalfop() { return halfop; }
bool Nick::hasVoice() { return voice; }

#ifdef USE_NICKINFO

NickInfoPtr Nick::getNickInfo() {
  return channelnickptr->getNickInfo();
}

ChannelNickPtr Nick::getChannelNick() {
  return channelnickptr;
}

QString Nick::getNickname() { return channelnickptr->getNickname(); }
QString Nick::getHostmask() { return channelnickptr->getHostmask(); }

#else 

QString Nick::getNickname() { return nickname; }
QString Nick::getHostmask() { return hostmask; }
    
void Nick::setNickname(const QString& newName)
{
  listViewItem->setText(1,newName);
}

void Nick::setHostmask(const QString& newMask)
{
  hostmask=newMask;
  listViewItem->setText(2,hostmask);
}

#endif

bool Nick::isSelected() { return listViewItem->isSelected(); }

