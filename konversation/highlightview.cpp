/***************************************************************************
                          highlightview.cpp  -  description
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

#include "highlightview.h"

HighlightView::HighlightView(QWidget* parent) : KListView(parent)
{
}

HighlightView::~HighlightView()
{
}

HighlightViewItem* HighlightView::currentItem()
{
	return (HighlightViewItem*) KListView::currentItem();
}

HighlightViewItem* HighlightView::selectedItem()
{
	return (HighlightViewItem*) KListView::selectedItem();
}
