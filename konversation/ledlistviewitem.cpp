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

LedListViewItem::LedListViewItem(KListView* parent, QString passed_label, bool passed_opState, bool passed_voiceState, int passed_color, int passed_column) : KListViewItem(parent, passed_label)
{
	opState = passed_opState;
	voiceState = passed_voiceState;
	color = passed_color;
	column = passed_column;	
	
	//currentLeds = leds.getLed(color, false);
	//opLedOff = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
	currentLeds = leds.getLed(color, true);
	opLedOn = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);
	currentLeds = leds.getLed(3, false);
	voiceLedOff = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::Off);
	currentLeds = leds.getLed(3, true);
	voiceLedOn = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);

	this->setText(0, "");
  this->setState(opState, voiceState);
	this->repaint();
}

LedListViewItem::~LedListViewItem()
{
}

void LedListViewItem::setState(bool passed_opState, bool passed_voiceState)
{
	opState = passed_opState;
	voiceState = passed_voiceState;
	(opState) ? this->setPixmap((column + 1), opLedOn) : (voiceState) ? this->setPixmap((column + 1), voiceLedOn) : this->setPixmap((column + 1), voiceLedOff);
	//this->setPixmap(column, currentLed);
	this->repaint();
}

void LedListViewItem::toggleOpState()
{
	opState = !opState;
	this->setState(opState, voiceState);
	this->repaint();
}

void LedListViewItem::toggleVoiceState()
{
	voiceState = !voiceState;
	this->setState(opState, voiceState);
	this->repaint();
}

void LedListViewItem::setText(int passed_column, QString passed_label)
{
	KListViewItem::setText((passed_column), passed_label);
}
