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

#include "nicklistviewitem.h"
#include "nick.h"
#include "addressbook.h"
#include <klocale.h>
#include <qtextstream.h>
#include <kabc/phonenumber.h>


Nick::Nick(KListView *listView,
           ChannelNickPtr channelnick)
{
  Q_ASSERT(channelnick);
  if(!channelnick) return;
  
    channelnickptr = channelnick;
  listViewItem=new NickListViewItem(listView,channelnick->getNickname(),channelnick->getHostmask(),this);

  QObject::connect(listViewItem,SIGNAL(refreshed()),listView,SLOT(resort()));

}
Nick::~Nick()
{
  delete listViewItem;
}

bool Nick::isAdmin()  { return channelnickptr->isAdmin(); }
bool Nick::isOwner()  { return channelnickptr->isOwner(); }
bool Nick::isOp()     { return channelnickptr->isOp(); }
bool Nick::isHalfop() { return channelnickptr->isHalfOp(); }
bool Nick::hasVoice() { return channelnickptr->hasVoice(); }



NickInfoPtr Nick::getNickInfo() {
  Q_ASSERT(channelnickptr);
  if(!channelnickptr) return NULL;
  return channelnickptr->getNickInfo();
}

ChannelNickPtr Nick::getChannelNick() {
  Q_ASSERT(channelnickptr);
  return channelnickptr;
}

QString Nick::getNickname() { return channelnickptr->getNickname(); }
QString Nick::getHostmask() { return channelnickptr->getHostmask(); }


bool Nick::isSelected() { return listViewItem->isSelected(); }

