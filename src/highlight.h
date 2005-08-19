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
        Highlight(const QString& itemPattern,
            bool regExp,
            const QColor& itemColor,
            const KURL& soundURL,
            const QString& autoText);

        QString getPattern();
        QString getAutoText();
        QColor getColor();
        int getID();
        bool getRegExp();
        KURL getSoundURL();

        void setPattern(const QString& itemPattern);
        void setColor(const QColor& itemColor);
        void setSoundURL(const KURL& url);
        void setAutoText(const QString& autoText);
        void setRegExp(bool state);

    protected:
        static unsigned int s_id;

        int m_itemID;
        bool m_regExp;

        QString m_itemPattern;
        QString m_autoText;
        QColor m_itemColor;
        KURL m_soundURL;
};
#endif
