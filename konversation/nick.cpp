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

Nick::Nick(KListView* listView,const QString &nickname, const QString &hostmask, bool op, bool voice)
{
  kdDebug() << "Nick::Nick(" << nickname << "," << hostmask << ")" << endl;

  listViewItem = new LedListViewItem(listView, nickname,op,voice);
  setNickname(nickname);
  setHostmask(hostmask);

  setOp(op);
  setVoice(voice);
}

Nick::~Nick()
{
  kdDebug() << "Nick::~Nick(" << getNickname() << ")" << endl;
  delete listViewItem;
}

void Nick::setNickname(const QString &newName)
{
  nickname=newName;
  listViewItem->setText(1,newName);
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
