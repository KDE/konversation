//
// C++ Implementation: nicksonlineitem
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <kdebug.h>

#include "nicksonlineitem.h"

NicksOnlineItem::NicksOnlineItem(int type,QListView* parent,QString name,QString col2,QString col3,
                                 QString col4,QString col5,QString col6,QString col7,QString col8) :
                 KListViewItem(parent,name,col2,col3,col4,col5,col6,col7,col8)
{
  m_type=type;
}

NicksOnlineItem::NicksOnlineItem(int type,QListViewItem* parent,QString name,QString col2,QString col3,
                                 QString col4,QString col5,QString col6,QString col7,QString col8) :
                 KListViewItem(parent,name,col2,col3,col4,col5,col6,col7,col8)
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
int NicksOnlineItem::compare(QListViewItem* i,int col,bool ascending) const
{
  // if we are the Offline item, make sure we get sorted at the end of the list
  if(m_type==OfflineItem) return ascending ? 1 : -1;
  // if we are competing with an Offline item, always lose
  if(static_cast<NicksOnlineItem*>(i)->type()==OfflineItem) return ascending ? -1 : 1;

  // otherwise compare items case-insensitively
  return key(col,ascending).lower().localeAwareCompare(i->key(col,ascending).lower());
}

/**
 * Returns the type of the item.
 * @return                  One of the enum NickListViewColumn
 */
int NicksOnlineItem::type()
{
  return m_type;
}
