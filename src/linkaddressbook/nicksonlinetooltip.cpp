/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    nicksonlinetooltip.cpp  -  The class that controls what the tooltip looks like when you hover over a person in the nicklistview.  This is used to show contact information about the person from the addressbook.
    begin:     Sun 25 July 2004
    copyright: (C) 2004 by John Tapsell
    email:     john@geola.co.uk
*/

#include <klocale.h>
#include <qtooltip.h>
#include <qlistview.h>
#include <klistview.h>
#include "nicklisttooltip.h"
#include "../nick.h"
#include "../nicklistview.h"
#include "../nicklistviewitem.h"
#include "../nickinfo.h"
#include "../nicksonline.h"

class NickListView;

namespace Konversation {
KonversationNicksOnlineToolTip::KonversationNicksOnlineToolTip(QWidget *parent, NicksOnline *nicksOnline) : QToolTip(parent)
{
	m_nicksOnline = nicksOnline;
}

KonversationNicksOnlineToolTip::~KonversationNicksOnlineToolTip() {
}

void KonversationNicksOnlineToolTip::maybeTip( const QPoint &pos )
{
    if( !parentWidget() || !m_nicksOnline || !m_nicksOnline->getNickListView() )
        return;
    KListView *m_listView = m_nicksOnline->getNickListView();
    QListViewItem *item = m_listView->itemAt( pos );
    if( !item )
        return;
    NickInfoPtr nickInfo = m_nicksOnline->getNickInfo(item);
   
    if(!nickInfo )
	return;
    QString toolTip;
    QRect itemRect = m_listView->itemRect( item );


    uint leftMargin = m_listView->treeStepSize() *
        ( item->depth() + ( m_listView->rootIsDecorated() ? 1 : 0 ) ) +
        m_listView->itemMargin();
    uint xAdjust = itemRect.left() + leftMargin;
    uint yAdjust = itemRect.top();
    QPoint relativePos( pos.x() - xAdjust, pos.y() - yAdjust );
    toolTip = nickInfo->tooltip();
    if(!toolTip.isEmpty()) 
        tip(itemRect, toolTip);
}

} // namespace Konversation

