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


NicksOnlineItem::NicksOnlineItem(int type, QTreeWidget* parent, const QString& name, const QString& col2) :
                 QTreeWidgetItem(parent)
{
  setText(0, name);
  setText(1, col2);
  m_type=type;
  m_connectionId = 0;
  m_offline = false;
}

NicksOnlineItem::NicksOnlineItem(int type, QTreeWidgetItem* parent, const QString& name, const QString& col2) :
                 QTreeWidgetItem(parent)
{
  setText(0, name);
  setText(1, col2);
  m_type=type;
  m_connectionId = 0;
  m_offline = false;
}

bool NicksOnlineItem::operator<(const QTreeWidgetItem &item) const
{
  // if we are the Offline item, make sure we get sorted at the end of the list
  if(m_type==OfflineItem) return 1;
  // if we are competing with an Offline item, always lose
  if(item.type()==OfflineItem) return -1;
  // otherwise compare items case-insensitively
  return text(treeWidget()->sortColumn()).toLower() < item.text(treeWidget()->sortColumn()).toLower();
}

/**
 * Returns the type of the item.
 * @return                  One of the enum NickListViewColumn
 */
int NicksOnlineItem::type() const
{
  return m_type;
}
