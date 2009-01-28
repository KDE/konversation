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

#include <q3listview.h>


class KUrl;
class K3ListView;

class HighlightViewItem : public Q3CheckListItem
{
    public:
        HighlightViewItem(K3ListView* parent, Highlight* passed_Highlight);
        ~HighlightViewItem();

        QString getPattern();
        QString getAutoText();
        QColor getColor() { return itemColor; }
        int getID() { return itemID; }
        bool getRegExp();
        KUrl getSoundURL() { return soundURL; }

        void setPattern(const QString& newPattern);
        void setAutoText(const QString& newAutoText);
        void setColor(QColor passed_itemColor) { itemColor = passed_itemColor; }
        void setID(int passed_itemID) { itemID = passed_itemID; }
        void setSoundURL(const KUrl& url);

        /** checks if the checkbox has been changed by the user
         *  stored internally by m_changed
         *
         * @return true when the checkbox has been changed
         */
        bool hasChanged();

        /** reset the change state of the listview item
         * call this when you have seen the change and acted upon it properly
         */
        void changeAcknowledged();

        HighlightViewItem* itemBelow();

    protected:
        QColor itemColor;
        QColorGroup itemColorGroup;
        int itemID;
        KUrl soundURL;
        QString autoText;

        bool m_changed;  // true if the checkbox has been changed

        void stateChange(bool newState);  // reimplemented to store changed value
        void paintCell(QPainter* p, const QColorGroup &cg, int column, int width, int alignment);
};
#endif
