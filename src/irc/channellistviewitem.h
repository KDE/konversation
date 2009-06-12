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

#include <QString>

#include <K3ListView>


class ChannelListViewItem : public K3ListViewItem
{
    public:
        ChannelListViewItem(K3ListView* parent, const QString& channel, const QString& users, const QString& topic);
        ~ChannelListViewItem();

        int compare(Q3ListViewItem* item, int col, bool ascending) const;
};
#endif
