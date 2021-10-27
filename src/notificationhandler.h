/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#ifndef KONVERSATIONNOTIFICATIONHANDLER_H
#define KONVERSATIONNOTIFICATIONHANDLER_H

#include <QObject>

class ChatWindow;
class Application;
class MainWindow;

namespace Konversation
{
    namespace DCC {
        class Transfer;
    }

    class NotificationHandler : public QObject
    {
        Q_OBJECT

        public:
            explicit NotificationHandler(Application* parent = nullptr);
            ~NotificationHandler() override;

        public Q_SLOTS:
            void message(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void nick(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void join(ChatWindow* chatWin, const QString& nick);
            void part(ChatWindow* chatWin, const QString& nick);
            void quit(ChatWindow* chatWin, const QString& nick);
            void nickChange(ChatWindow* chatWin, const QString& oldNick, const QString& newNick);
            void dccIncoming(ChatWindow* chatWin, const QString& fromNick);
            void dccError(ChatWindow* chatWin, const QString& error);
            void dccTransferDone(ChatWindow* chatWin, const QString& file, Konversation::DCC::Transfer* transfer);
            void mode(ChatWindow* chatWin, const QString& nick, const QString& subject, const QString& change);
            void query(ChatWindow* chatWin, const QString& fromNick);
            void queryMessage(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void nickOnline(ChatWindow* chatWin, const QString& nick);
            void nickOffline(ChatWindow* chatWin, const QString& nick);
            void kick(ChatWindow* chatWin, const QString& channel,const QString& nick);
            void dccChat(ChatWindow* chatWin, const QString& nick);
            void highlight(ChatWindow* chatWin, const QString& fromNick, const QString& message);
            void connectionFailure(ChatWindow* chatWin, const QString& server);
            void channelJoin(ChatWindow* chatWin, const QString& channel);

        private:
            void startTrayNotification(ChatWindow* chatWin);

        private:
            MainWindow* m_mainWindow;

            Q_DISABLE_COPY(NotificationHandler)
    };

}
#endif
