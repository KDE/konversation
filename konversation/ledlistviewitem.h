/***************************************************************************
                          ledlistviewitem.h  -  description
                             -------------------
    begin                : Thu Jul 25 2002
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

#ifndef LEDLISTVIEWITEM_H
#define LEDLISTVIEWITEM_H

#include <klistview.h>
#include <qiconset.h>
#include <qpixmap.h>
#include <kstandarddirs.h>
#include <images.h>
#include <qobject.h>

/**
  *@author Matthias Gierlings
  */

class LedListViewItem : public KListViewItem
{
	public:
		LedListViewItem(KListView* parent, QString passed_label, bool passed_state, int passed_color, int passed_column);
		~LedListViewItem();

	private:	
		QPixmap						ledOn, ledOff;
		QIconSet					currentLeds;
		Images						leds;
    QString						label;
		bool							state;
	  int								color, column;
	
	public:
		bool getState() {return state;}
 		void setState(bool passed_state);
		void toggleState();
	

};
#endif
