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

#include <QString>
#include <QColor>

#include <QUrl>

/**
 *@author Matthias Gierlings
 */

class Highlight
{
    public:
        Highlight(const QString& pattern, bool regExp, const QColor& color,
            const QUrl &soundURL, const QString& autoText, const QString& chatWindows, bool notify);

        int getID() const;

        void setPattern(const QString& pattern);
        QString getPattern() const ;

        void setRegExp(bool state);
        bool getRegExp() const;

        void setColor(const QColor& color);
        QColor getColor() const;

        void setSoundURL(const QUrl &url);
        QUrl getSoundURL() const;

        void setAutoText(const QString& autoText);
        QString getAutoText() const;

        void setChatWindows(const QString& chatWindows);
        QString getChatWindows() const;
        QStringList getChatWindowList() const;

        void setNotify(bool notify);
        bool getNotify() const;

    protected:
        static unsigned int s_id;

        int m_itemID;

        QString m_itemPattern;
        bool m_regExp;
        QColor m_itemColor;
        QUrl m_soundURL;
        QString m_autoText;
        QString m_chatWindows;
        QStringList m_chatWindowList;
        bool m_notify;
};
#endif
