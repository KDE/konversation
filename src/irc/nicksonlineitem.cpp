/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "nicksonlineitem.h"


NicksOnlineItem::NicksOnlineItem(int type, Q3ListView* parent, const QString& name, const QString& col2) :
                 K3ListViewItem(parent, name, col2)
{
  m_type=type;
}

NicksOnlineItem::NicksOnlineItem(int type, Q3ListViewItem* parent, const QString& name, const QString& col2) :
                 K3ListViewItem(parent, name, col2)
{
  m_type=type;
}

/**
 * Reimplemented to make sure, "Offline" items always get sorted to the bottom of the list
 * @param i                 Pointer to the QListViewItem to compare with.
 * @param col               The column to compare
 * @param ascending         Specify sorting direction
 * @return                  -1 if this item's value is smaller than i, 0 if they are equal, 1 if it's greater
 */
int NicksOnlineItem::compare(Q3ListViewItem* i,int col,bool ascending) const
{
  // if we are the Offline item, make sure we get sorted at the end of the list
  if(m_type==OfflineItem) return ascending ? 1 : -1;
  // if we are competing with an Offline item, always lose
  if(static_cast<NicksOnlineItem*>(i)->type()==OfflineItem) return ascending ? -1 : 1;

  // otherwise compare items case-insensitively
  return key(col,ascending).toLower().localeAwareCompare(i->key(col,ascending).toLower());
}

/**
 * Returns the type of the item.
 * @return                  One of the enum NickListViewColumn
 */
int NicksOnlineItem::type() const
{
  return m_type;
}
