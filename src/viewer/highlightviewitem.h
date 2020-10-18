/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Matthias Gierlings <gismore@users.sourceforge.net>
*/

#ifndef HIGHLIGHTVIEWITEM_H
#define HIGHLIGHTVIEWITEM_H

#include "highlight.h"

#include <QTreeWidget>


class QUrl;

class HighlightViewItem : public QTreeWidgetItem
{
    public:
        HighlightViewItem(QTreeWidget* parent, Highlight* highlight);
        ~HighlightViewItem() override;

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

    private:
        int m_itemID;

        QString m_pattern;
        QColor m_color;
        QUrl m_soundURL;
        QString m_autoText;
        QString m_chatWindows;
        bool m_notify;

        Q_DISABLE_COPY(HighlightViewItem)
};
#endif
