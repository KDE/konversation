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

  $Id$
*/

#include <kdebug.h>

#include "nick.h"

Nick::Nick(KListView* listView,const QString& nickname, const QString& hostmask, bool op, bool voice)
{
  listViewItem=new LedListViewItem(listView,nickname,hostmask,op,voice);
  setNickname(nickname);
  setHostmask(hostmask);

  setOp(op);
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

void Nick::setOp(bool setop)
{
  op=setop;
  listViewItem->setState(op,voice);
}

void Nick::setVoice(bool setvoice)
{
  voice=setvoice;
  listViewItem->setState(op,voice);
}

bool Nick::isOp() { return op; }
bool Nick::hasVoice() { return voice; }

QString Nick::getNickname() { return nickname; }
QString Nick::getHostmask() { return hostmask; }

bool Nick::isSelected() { return listViewItem->isSelected(); }
