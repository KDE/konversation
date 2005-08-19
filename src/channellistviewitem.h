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

#ifndef CHANNELLISTVIEWITEM_H
#define CHANNELLISTVIEWITEM_H

#include <qstring.h>

#include <klistview.h>

class ChannelListViewItem : public KListViewItem
{
    public:
        ChannelListViewItem(KListView* parent,QString channel,QString users,QString topic);
        ~ChannelListViewItem();

        int compare(QListViewItem* item, int col, bool ascending) const;
};
#endif
