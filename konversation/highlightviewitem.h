/***************************************************************************
                          highlightviewitem.h  -  description
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

#ifndef HIGHLIGHTVIEWITEM_H
#define HIGHLIGHTVIEWITEM_H

#include <klistview.h>
#include <qlistview.h>
#include "highlight.h"

/**
  *@author Matthias Gierlings
  */

class HighlightViewItem : public KListViewItem
{
	public:
		HighlightViewItem(KListView* parent, Highlight* passed_Highlight);
		~HighlightViewItem();

  	QString	getText()		{return text(0);};
		QColor	getColor()	{return itemColor;};
		int			getID()			{return itemID;};

//	void	setText(QString passed_itemText)	{itemText = passed_itemText;};
		void	setColor(QColor passed_itemColor)	{itemColor = passed_itemColor;};
		void	setID(int passed_itemID)					{itemID = passed_itemID;};

	private:
		QColor			itemColor;
		QColorGroup itemColorGroup;
		int					itemID;

		void paintCell(QPainter* p, const QColorGroup &cg, int column, int width, int alignment);
};

#endif
