/***************************************************************************
                          highlight.cpp  -  description
                             -------------------
    begin                : Sat Jun 15 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net

    $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <qstring.h>
#include <qcolor.h>

/**
  *@author Matthias Gierlings
  */

class Highlight
{
	public:
		Highlight(QString passed_itemText, QColor passed_itemColor);
		~Highlight();

		QString getText() {return itemText;};
		QColor getColor() {return itemColor;};
		int getID() {return itemID;};

		void setText(QString passed_itemText) {itemText = passed_itemText;};
		void setColor(QColor passed_itemColor) {itemColor = passed_itemColor;};

  protected:
    static int id;

  private:
		QString		itemText;
		QColor		itemColor;
		int				itemID;

};


#endif
