/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
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
            explicit TopicLabel(QWidget *parent = nullptr);
            ~TopicLabel() override;

            QSize minimumSizeHint() const override;
            QSize sizeHint() const override;
            void setServer(Server* server);
            void setChannelName(const QString& channel);

            void setContextMenuOptions(IrcContextMenus::MenuOptions options, bool on);

        public Q_SLOTS:
            virtual void openLink(const QString& link);
            void setText(const QString& text);
            void clear();

        Q_SIGNALS:
            void setStatusBarTempText(const QString&);
            void clearStatusBarTempText();

        protected:
            void leaveEvent (QEvent*) override;
            void contextMenuEvent(QContextMenuEvent* ev) override;
            void resizeEvent(QResizeEvent*) override;
            void mouseReleaseEvent(QMouseEvent* ev) override;
            void mousePressEvent(QMouseEvent* ev) override;
            void mouseMoveEvent(QMouseEvent* ev) override;

        private Q_SLOTS:
            void highlightedSlot(const QString&);

        private:
            QString rPixelSqueeze(const QString& text, int maxPixels) const;
            int textWidth(const QString& text) const;

            void updateSqueezedText();
            void resetLinkHighlightState();

            inline QString tagUrls(const QString& text, const QString& sender);

        private:
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

            Q_DISABLE_COPY(TopicLabel)
    };
}

#endif
