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

Nick::Nick(KListView* listView,
           const QString& nickname,
           const QString& hostmask,
           bool admin,
           bool owner,
           bool op,
           bool halfop,
           bool voice)
{
  listViewItem=new LedListViewItem(listView,nickname,hostmask,admin,owner,op,halfop,voice);
  setNickname(nickname);
  setHostmask(hostmask);

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
  nickname=newName;
  listViewItem->setText(1,newName);
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
