/***************************************************************************
                          highlight.cpp  -  description
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

#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <qstring.h>
#include <qcolor.h>

#include <kurl.h>

/**
  *@author Matthias Gierlings
  */


class Highlight
{
  public:
    Highlight(const QString& itemText,const QColor& itemColor, const KURL& soundURL);
    ~Highlight();

    QString getText();
    QColor getColor();
    int getID();
    KURL getSoundURL();

    void setText(const QString& itemText);
    void setColor(const QColor& itemColor);
    void setSoundURL(const KURL& url);

  protected:
    static unsigned int s_id;
    
    int m_itemID;
    QString m_itemText;
    QColor m_itemColor;
    KURL m_soundURL;
};

#endif
