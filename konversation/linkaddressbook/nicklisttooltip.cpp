/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    nicklisttooltip.h  -  The class that controls what the tooltip looks like when you hover over a person in the nicklistview.  This is used to show contact information about the person from the addressbook.
    begin:     Sun 25 July 2004
    copyright: (C) 2004 by John Tapsell
    email:     john@geola.co.uk
*/

#ifndef NICKLISTTOOLTIP_H
#define NICKLISTTOOLTIP_H

#include <klocale.h>
#include <qtooltip.h>
#include <qlistview.h>
#include "nicklisttooltip.h"
#include "../nick.h"
#include "../nicklistview.h"
#include "../ledlistviewitem.h"
#include "../nickinfo.h"

class NickListView;

namespace Konversation {
KonversationNickListViewToolTip::KonversationNickListViewToolTip(QWidget *parent, NickListView *lv) : QToolTip(parent)
{
	m_listView = lv;
}

KonversationNickListViewToolTip::~KonversationNickListViewToolTip() {
}

void KonversationNickListViewToolTip::maybeTip( const QPoint &pos )
{
    if( !parentWidget() || !m_listView )
        return;

    QListViewItem *item = m_listView->itemAt( pos );
    if( !item )
        return;
    LedListViewItem *ledItem = dynamic_cast<LedListViewItem *>( item );
    Nick *nick = NULL;
    if(ledItem)
        nick = ledItem->getNick();
    
    QString toolTip;
    QRect itemRect = m_listView->itemRect( item );

    if(! nick )
	return;

    uint leftMargin = m_listView->treeStepSize() *
        ( item->depth() + ( m_listView->rootIsDecorated() ? 1 : 0 ) ) +
        m_listView->itemMargin();
    uint xAdjust = itemRect.left() + leftMargin;
    uint yAdjust = itemRect.top();
    QPoint relativePos( pos.x() - xAdjust, pos.y() - yAdjust );
    toolTip = nick->getNickInfo()->tooltip();
    if(!toolTip.isEmpty()) 
        tip(itemRect, toolTip);

}

} // namespace Konversation

#endif
