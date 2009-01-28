/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  item in the channel list
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "channellistviewitem.h"


ChannelListViewItem::ChannelListViewItem(K3ListView* parent, const QString& channel, const QString& users, const QString& topic)
: K3ListViewItem(parent,channel,users,topic)
{
}

ChannelListViewItem::~ChannelListViewItem()
{
}

int ChannelListViewItem::compare(Q3ListViewItem* item, int col, bool ascending) const
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
    return K3ListViewItem::compare(item,col,ascending);
}
