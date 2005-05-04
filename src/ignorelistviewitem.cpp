/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Die Jun 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <klocale.h>
#include <kdebug.h>

#include "ignorelistviewitem.h"
#include "ignore.h"

IgnoreListViewItem::IgnoreListViewItem(QListView* parent,QString name,int newFlags):
        KListViewItem(parent,name)
{
  yes=i18n("yes");
  no=i18n("no");

  setFlags(newFlags);
}

IgnoreListViewItem::~IgnoreListViewItem()
{
}

void IgnoreListViewItem::setFlag(int flag,bool active)
{
  int column=1;
  for(int i=1;i<64;i+=i)
  {
    if(flag==i) setText(column,(active) ? yes : no);
    column++;
  }

  if(active) flags+=flag;
  else flags-=flag;
}

void IgnoreListViewItem::setFlags(int newFlags)
{
  flags=newFlags;

  setText(1,(flags & Ignore::Channel) ? yes : no);
  setText(2,(flags & Ignore::Query) ? yes : no);
  setText(3,(flags & Ignore::Notice) ? yes : no);
  setText(4,(flags & Ignore::CTCP) ? yes : no);
  setText(5,(flags & Ignore::DCC) ? yes : no);
  setText(6,(flags & Ignore::Exception) ? yes : no);
}

IgnoreListViewItem* IgnoreListViewItem::itemBelow()
{
  return (IgnoreListViewItem*) QListViewItem::itemBelow();
}
