/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONTOPICLABEL_H
#define KONVERSATIONTOPICLABEL_H

#include <kactivelabel.h>
#include <klocale.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <Q3PopupMenu>


class QFontMetrics;
class Server;
class KPopupMenu;

namespace Konversation
{

    class TopicLabel : public KActiveLabel
    {
        Q_OBJECT

        public:
            explicit TopicLabel(QWidget *parent = 0, const char *name = 0);
            ~TopicLabel();

            QSize minimumSizeHint() const;
            QSize sizeHint() const;
            void setServer(Server* server);

            enum PopupIDs { Copy,CopyUrl,SelectAll,Bookmark };

        public slots:
            virtual void openLink(const QString& link);
            void setText(const QString& text);

        signals:
            void setStatusBarTempText(const QString&);
            void clearStatusBarTempText();
            void popupCommand(int);
            void currentChannelChanged(const QString&);

        protected:
            void updateSqueezedText();
            QString rPixelSqueeze(const QString& text, uint maxPixels);
            uint textWidth(const QString& text, const QFontMetrics& fm);
            virtual void contentsMousePressEvent(QMouseEvent *e);
            virtual void contentsMouseReleaseEvent(QMouseEvent *e);
            virtual void contentsMouseMoveEvent(QMouseEvent *e);
            virtual void leaveEvent (QEvent*);
            virtual void contentsContextMenuEvent(QContextMenuEvent* ev);
            void resizeEvent(QResizeEvent*);
            bool contextMenu(QContextMenuEvent* ce);

            void setupChannelPopupMenu();

        protected slots:
            void highlightedSlot(const QString&);

        private:
            Server* m_server;

            Q3PopupMenu* m_popup;
            KPopupMenu* m_channelPopup;

            QString m_fullText;
            bool mousePressed;
            QString urlToDrag;
            QPoint pressPosition;
            QString m_lastStatusText;
            QString m_highlightedURL;
            QString m_currentChannel;
            bool m_isOnChannel;
            int m_nickPopupId;
            int m_channelPopupId;
            bool m_copyUrlMenu;
            QString m_urlToCopy;

    };

}
#endif
