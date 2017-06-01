/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONTOPICLABEL_H
#define KONVERSATIONTOPICLABEL_H

#include "irccontextmenus.h"

#include <QLabel>


class Server;

namespace Konversation
{
    class TopicLabel : public QLabel
    {
        Q_OBJECT

        public:
            explicit TopicLabel(QWidget *parent = 0, const char *name = 0);
            ~TopicLabel();

            QSize minimumSizeHint() const Q_DECL_OVERRIDE;
            QSize sizeHint() const Q_DECL_OVERRIDE;
            void setServer(Server* server);
            void setChannelName(const QString& channel);

            void setContextMenuOptions(IrcContextMenus::MenuOptions options, bool on);

        public Q_SLOTS:
            virtual void openLink(const QString& link);
            void setText(const QString& text);

        Q_SIGNALS:
            void setStatusBarTempText(const QString&);
            void clearStatusBarTempText();

        protected:
            void updateSqueezedText();
            QString rPixelSqueeze(const QString& text, int maxPixels);
            int textWidth(const QString& text);
            void leaveEvent (QEvent*) Q_DECL_OVERRIDE;
            void contextMenuEvent(QContextMenuEvent* ev) Q_DECL_OVERRIDE;
            void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;
            void mouseReleaseEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
            void mousePressEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
            void mouseMoveEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;

        protected Q_SLOTS:
            void highlightedSlot(const QString&);

        private:
            void resetLinkHighlightState();

            inline QString tagUrls(const QString& text, const QString& sender);

            Server* m_server;
            QString m_channelName;

            QString m_fullText;
            QString m_lastStatusText;

            IrcContextMenus::MenuOptions m_contextMenuOptions;
            QString m_currentChannel;
            bool m_isOnChannel;
            QString m_currentUrl;
            QString m_dragUrl;
            bool m_mousePressedOnUrl;
            QPoint m_mousePressPosition;
    };

}
#endif
