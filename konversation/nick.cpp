/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
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
           const ChannelNickPtr& channelnick)
{
  Q_ASSERT(channelnick);
  if(!channelnick) return;
  
  channelnickptr = channelnick;
  listViewItem=new NickListViewItem(listView,listView->lastItem(),
				    channelnick->getNickname(),channelnick->getHostmask(),this);
  
  QObject::connect(listViewItem,SIGNAL(refreshed()),listView,SLOT(startResortTimer()));
}

Nick::~Nick()
{
  delete listViewItem;
}

bool Nick::isAdmin() const
{ 
  return channelnickptr->isAdmin(); 
}

bool Nick::isOwner() const
{ 
  return channelnickptr->isOwner(); 
}

bool Nick::isOp() const
{ 
  return channelnickptr->isOp(); 
}

bool Nick::isHalfop() const
{ 
  return channelnickptr->isHalfOp(); 
}

bool Nick::hasVoice() const
{ 
  return channelnickptr->hasVoice(); 
}

NickInfoPtr Nick::getNickInfo() const {
  Q_ASSERT(channelnickptr);
  if(!channelnickptr) return NULL;
  return channelnickptr->getNickInfo();
}

ChannelNickPtr Nick::getChannelNick() const {
  Q_ASSERT(channelnickptr);
  return channelnickptr;
}

QString Nick::getNickname() const 
{ 
  return channelnickptr->getNickname(); 
}

QString Nick::getHostmask() const
{ 
  return channelnickptr->getHostmask(); 
}

bool Nick::isSelected() const
{ 
  return listViewItem->isSelected(); 
}

