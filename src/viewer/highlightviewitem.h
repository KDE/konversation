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


class KUrl;

class HighlightViewItem : public QTreeWidgetItem
{
    public:
        HighlightViewItem(QTreeWidget* parent, Highlight* highlight);
        ~HighlightViewItem();

        void setID(const int itemID);
        int getID();

        void setPattern(const QString& pattern);
        QString getPattern();

        void setRegExp(const bool regexp);
        bool getRegExp();

        void setColor(const QColor color);
        QColor getColor();

        void setSoundURL(const KUrl& url);
        KUrl getSoundURL();

        void setAutoText(const QString& autoText);
        QString getAutoText();

        void setChatWindows(const QString& chatWindows);
        QString getChatWindows();

    protected:
        int m_itemID;

        QString m_pattern;
        QColor m_color;
        KUrl m_soundURL;
        QString m_autoText;
        QString m_chatWindows;
};
#endif
