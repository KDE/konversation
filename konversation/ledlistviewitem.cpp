/***************************************************************************
                          ledlistviewitem.cpp  -  description
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

#include "ledlistviewitem.h"
#include <iostream.h>

LedListViewItem::LedListViewItem(KListView* parent, QString passed_label, bool passed_state, int passed_color, int passed_column) : KListViewItem(parent, passed_label)
{
	state = passed_state;
	color = passed_color;
	column = passed_column;	
	
	currentLeds = leds.getLed(color, false);
	ledOff = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
	currentLeds = leds.getLed(color, true);
	ledOn = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);

	this->setState(state);
	this->repaint();
}

LedListViewItem::~LedListViewItem()
{
}

void LedListViewItem::setState(bool passed_state)
{
	state = passed_state;
	(state) ? this->setPixmap(column, ledOn) : this->setPixmap(column, ledOff);
	//this->setPixmap(column, currentLed);
	this->repaint();
}

void LedListViewItem::toggleState()
{
	state = !state;
	this->setState(state);
	this->repaint();
}
