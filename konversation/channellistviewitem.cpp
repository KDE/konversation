/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  channellistviewitem.cpp  -  item in the channel list
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kdebug.h>

#include "channellistviewitem.h"

ChannelListViewItem::ChannelListViewItem(KListView* parent,QString channel,QString users,QString topic)
                         : KListViewItem(parent,channel,users,topic)
{
}

ChannelListViewItem::~ChannelListViewItem()
{
}

int ChannelListViewItem::compare(QListViewItem* item, int col, bool ascending) const
{
  if(col==1)
  {
    bool ok;
    int i=text(col).toInt(&ok);
    if(ok)
    {
      int j=item->text(col).toInt(&ok);
      if(ok) return (i<j) ? -1 : (i>j) ? 1 : 0;
    }
  }
  return KListViewItem::compare(item,col,ascending);
}
