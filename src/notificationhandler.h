/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONNOTIFICATIONHANDLER_H
#define KONVERSATIONNOTIFICATIONHANDLER_H

#include <QObject>


class ChatWindow;
class Application;
class MainWindow;

namespace Konversation
{

    class NotificationHandler : public QObject
    {
        Q_OBJECT

        public:
            explicit NotificationHandler(Application* parent = 0);
            ~NotificationHandler();

        public slots:
            void message(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void nick(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void join(ChatWindow* chatWin, const QString& nick);
            void part(ChatWindow* chatWin, const QString& nick);
            void quit(ChatWindow* chatWin, const QString& nick);
            void nickChange(ChatWindow* chatWin, const QString& oldNick, const QString& newNick);
            void dccIncoming(ChatWindow* chatWin, const QString& fromNick);
            void dccError(ChatWindow* chatWin, const QString& error);
            void dccTransferDone(ChatWindow* chatWin, const QString& file);
            void mode(ChatWindow* chatWin, const QString& nick);
            void query(ChatWindow* chatWin, const QString& fromNick);
            void queryMessage(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void nickOnline(ChatWindow* chatWin, const QString& nick);
            void nickOffline(ChatWindow* chatWin, const QString& nick);
            void kick(ChatWindow* chatWin, const QString& channel,const QString& nick);
            void dccChat(ChatWindow* chatWin, const QString& nick);
            void highlight(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void connectionFailure(ChatWindow* chatWin, const QString& server);
            void channelJoin(ChatWindow* chatWin, const QString& channel);

        protected:
            void startTrayNotification(ChatWindow* chatWin);

        private:
            MainWindow* m_mainWindow;
    };

}
#endif
