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

/* Eisfuchs: include static variable */
int Highlight::id;

Highlight::Highlight(QString passed_itemText, QColor passed_itemColor)
{
	itemText = passed_itemText;
	itemColor = passed_itemColor;
  /* Eisfuchs: Unique ID for every Highlight */
	itemID = id++;
}

Highlight::~Highlight()
{
}
