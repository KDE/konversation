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

#include "highlight.h"

unsigned int Highlight::id;  // static

Highlight::Highlight(const QString& passed_itemText,const QColor& passed_itemColor)
{
  itemText=passed_itemText;
  itemColor=passed_itemColor;
  
  // unique ID for every Highlight
  itemID=id++;
}

Highlight::~Highlight()
{
}

int Highlight::getID()       { return itemID; }

QString Highlight::getText() { return itemText; }
QColor Highlight::getColor() { return itemColor; }

void Highlight::setText(const QString& passed_itemText)  { itemText=passed_itemText; }
void Highlight::setColor(const QColor& passed_itemColor) { itemColor=passed_itemColor; }
