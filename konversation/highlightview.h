/***************************************************************************
                          highlightview.h  -  description
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

#ifndef HIGHLIGHTVIEW_H
#define HIGHLIGHTVIEW_H

#include <klistview.h>

#include <qvgroupbox.h>

#include "highlightviewitem.h"

/**
  *@author Matthias Gierlings
  */

class HighlightView : public KListView
{

	public:
		HighlightView(QWidget* parent);
		~HighlightView();

		HighlightViewItem* currentItem();
		HighlightViewItem* selectedItem();
		HighlightViewItem* firstChild();
};

#endif
