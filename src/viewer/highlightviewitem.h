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
        HighlightViewItem(QTreeWidget* parent, Highlight* passed_Highlight);
        ~HighlightViewItem();

        QString getPattern();
        QString getAutoText();
        QColor getColor() { return itemColor; }
        int getID() { return itemID; }
        bool getRegExp();
        KUrl getSoundURL() { return soundURL; }

        void setPattern(const QString& newPattern);
        void setAutoText(const QString& newAutoText);
        void setColor(QColor passed_itemColor) { itemColor = passed_itemColor; updateRowColor(); }
        void setID(int passed_itemID) { itemID = passed_itemID; }
        void setSoundURL(const KUrl& url);


    protected:
        QColor itemColor;
        int itemID;
        KUrl soundURL;
        QString autoText;

        void updateRowColor();
};
#endif
