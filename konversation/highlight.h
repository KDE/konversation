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
    Highlight(const QString& passed_itemText,const QColor& passed_itemColor);
    ~Highlight();

    QString getText();
    QColor getColor();
    int getID();

    void setText(const QString& passed_itemText);
    void setColor(const QColor& passed_itemColor);

  protected:
    static unsigned int id;
    
    int itemID;
    QString itemText;
    QColor itemColor;
};

#endif
