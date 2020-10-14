/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Matthias Gierlings <gismore@users.sourceforge.net>
*/

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

    private:
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
