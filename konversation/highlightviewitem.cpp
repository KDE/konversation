/***************************************************************************
                          highlightviewitem.cpp  -  description
                             -------------------
    begin                : Sat Jun 15 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "highlightviewitem.h"

#include <kurl.h>

HighlightViewItem::HighlightViewItem(KListView* parent, Highlight* passed_Highlight)
  : KListViewItem(parent, passed_Highlight->getText())
{
  itemColor = passed_Highlight->getColor();
  itemID = passed_Highlight->getID();
  setSoundURL(passed_Highlight->getSoundURL());
}

HighlightViewItem::~HighlightViewItem()
{
}

void HighlightViewItem::paintCell(QPainter* p, const QColorGroup &cg, int column, int width, int alignment)
{
  // EIS: Kopiere die Farben aus der cg in deine eigene itemColorGroup, und
  // ändere dann erst die Farben, wie du sie brauchst, sonst sind alle Farben
  // die du nicht selber definiert hast, durchsichtig
  itemColorGroup=cg;
  itemColorGroup.setColor(QColorGroup::Text, itemColor);
  KListViewItem::paintCell(p, itemColorGroup, column, width, alignment);
}

HighlightViewItem* HighlightViewItem::itemBelow()
{
  return (HighlightViewItem*) KListViewItem::itemBelow();
}

void HighlightViewItem::setSoundURL(const KURL& url)
{
  soundURL = url;
  setText(1, soundURL.prettyURL());
}
