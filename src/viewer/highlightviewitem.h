/***************************************************************************
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

#ifndef HIGHLIGHTVIEWITEM_H
#define HIGHLIGHTVIEWITEM_H

#include "highlight.h"

#include <QTreeWidget>


class QUrl;

class HighlightViewItem : public QTreeWidgetItem
{
    public:
        HighlightViewItem(QTreeWidget* parent, Highlight* highlight);
        ~HighlightViewItem();

        void setID(const int itemID);
        int getID() const;

        void setPattern(const QString& pattern);
        QString getPattern() const;

        void setRegExp(const bool regexp);
        bool getRegExp() const;

        void setColor(const QColor color);
        QColor getColor() const;

        void setSoundURL(const QUrl &url);
        QUrl getSoundURL() const;

        void setAutoText(const QString& autoText);
        QString getAutoText() const;

        void setChatWindows(const QString& chatWindows);
        QString getChatWindows() const;

        void setNotify(bool doNotify);
        bool getNotify() const;

    protected:
        int m_itemID;

        QString m_pattern;
        QColor m_color;
        QUrl m_soundURL;
        QString m_autoText;
        QString m_chatWindows;
        bool m_notify;
};
#endif
