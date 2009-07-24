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

            QSize minimumSizeHint() const;
            QSize sizeHint() const;
            void setServer(Server* server);
            void setChannelName(const QString& channel);

        public slots:
            virtual void openLink(const QString& link);
            void setText(const QString& text);

        signals:
            void setStatusBarTempText(const QString&);
            void clearStatusBarTempText();

        protected:
            void updateSqueezedText();
            QString rPixelSqueeze(const QString& text, int maxPixels);
            int textWidth(const QString& text);
            virtual void leaveEvent (QEvent*);
            virtual void contextMenuEvent(QContextMenuEvent* ev);
            void resizeEvent(QResizeEvent*);
            bool contextMenu(QContextMenuEvent* ce);

        protected slots:
            void highlightedSlot(const QString&);

            void joinChannel();
            void getChannelUserList();
            void getChannelTopic();

            void copyUrl();
            void bookmarkUrl();
            void saveLinkAs();

        private:
            Server* m_server;
            QString m_channelName;

            QString m_fullText;
            QString m_lastStatusText;
            QString m_highlightedURL;
            QString m_currentChannel;
            bool m_isOnChannel;
            int m_nickPopupId;
            bool m_copyUrlMenu;
            QString m_urlToCopy;
    };

}
#endif
