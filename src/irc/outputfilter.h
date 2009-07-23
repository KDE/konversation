/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005-2009 Eike Hein <hein@kde.org>
*/

#ifndef OUTPUTFILTER_H
#define OUTPUTFILTER_H

#include "identity.h"
#include "common.h"

#include <QObject>
#include <QString>
#include <QSet>

#include <KUrl>
#include <kio/global.h>

class Server;
class ChatWindow;


namespace Konversation
{
    enum MessageType
    {
        Message,
        Action,
        Command,
        Program,
        PrivateMessage
    };

    struct OutputFilterResult
    {
        QString output;
        QStringList outputList;
        QString toServer;
        QStringList toServerList;
        QString typeString;
        MessageType type;
    };

    class OutputFilter : public QObject
    {
        Q_OBJECT

        public:
            explicit OutputFilter(Server* server);
            ~OutputFilter();

            static const QSet<QString>& supportedCommands() { return m_commands; }

            QStringList splitForEncoding(const QString& inputLine, int max, int segments = -1);
            OutputFilterResult parse(const QString& myNick,const QString& line,const QString& name);

            // dcc send
            OutputFilterResult sendRequest(const QString &recipient,const QString &fileName,const QString &address,uint port,quint64 size);
            OutputFilterResult passiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,quint64 size,const QString &token);
            OutputFilterResult acceptResumeRequest(const QString &recipient,const QString &fileName,uint port,quint64 startAt);
            OutputFilterResult acceptPassiveResumeRequest(const QString &recipient,const QString &fileName,uint port,quint64 startAt,const QString &token);

            // dcc recv
            OutputFilterResult resumeRequest(const QString &sender,const QString &fileName,uint port,KIO::filesize_t startAt);
            OutputFilterResult resumePassiveRequest(const QString &sender,const QString &fileName,uint port,KIO::filesize_t startAt,const QString &token);
            OutputFilterResult acceptPassiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,uint port,quint64 size,const QString &token);
            OutputFilterResult rejectDccSend(const QString& partnerNick, const QString& fileName);

            // dcc chat
            OutputFilterResult rejectDccChat(const QString& partnerNick);

            bool replaceAliases(QString& line);

        signals:
            void openDccSend(const QString &recipient, KUrl kurl);
            void requestDccSend();                // Choose Recipient and File from requester
                                                  // Choose File from requester
            void requestDccSend(const QString& recipient);
            void openDccChat(const QString& nick);
            void addDccPanel();
            void closeDccPanel();
            void acceptDccGet(const QString& nick, const QString& file);
            void openRawLog(bool show);
            void closeRawLog();
            void openKonsolePanel();
            void openChannelList(const QString& parameter, bool getList);
            void sendToAllChannels(const QString& text);
            void launchScript(const QString& target, const QString& parameter);
            void banUsers(const QStringList& userList,const QString& channel,const QString& option);
            void unbanUsers(const QString& mask,const QString& channel);
            void multiServerCommand(const QString& command, const QString& parameter);
            void reconnectServer();
            void disconnectServer();

            void connectTo(Konversation::ConnectionFlag flag,
                           const QString& hostName,
                           const QString& port = "",
                           const QString& password = "",
                           const QString& nick = "",
                           const QString& channel = "",
                           bool useSSL = false
            );

            void showView(ChatWindow* view);
            void encodingChanged ();

        public slots:
            OutputFilterResult execBan(const QString& mask,const QString& channels);
            OutputFilterResult execUnban(const QString& mask,const QString& channels);

        private slots:
            void command_join();
            void command_part();
            void command_leave();
            void command_quit();
            void command_close();
            void command_notice();
            void command_j();
            void command_me();
            void command_msg();
            void command_m();
            void command_smsg();
            void command_query();
            void command_ame();
            void command_amsg();
            void command_omsg();
            void command_onotice();
            void command_quote();
            void command_say();
            void command_op();
            void command_deop();
            void command_hop();
            void command_dehop();
            void command_voice();
            void command_devoice();
            void command_unvoice();
            void command_ctcp();
            void command_ping();
            void command_kick();
            void command_topic();
            void command_away();
            void command_unaway();
            void command_back();
            void command_invite();
            void command_exec();
            void command_notify();
            void command_oper();
            void command_ban();
            void command_unban();
            void command_kickban();
            void command_ignore();
            void command_unignore();
            void command_list();
            void command_names();
            void command_raw();
            void command_dcc();
            void command_konsole();
            void command_aaway();
            void command_aunaway();
            void command_aback();
            void command_server();
            void command_reconnect();
            void command_disconnect();
            void command_charset();
            void command_encoding();
            void command_setkey();
            void command_keyx();
            void command_delkey();
            void command_showkey();
            void command_dns();
            void command_kill();
            void command_queuetuner();

        private:
            static void fillCommandList();
            static QSet<QString> m_commands;

            void handleMsg(bool commandIsQuery);
            void handleCtcp(const QString& parameter);
            void handleBan(bool kick);

            OutputFilterResult changeMode(const QString& parameter,char mode,char giveTake);
            bool isAChannel(const QString& check);
            OutputFilterResult usage(const QString& check);
            OutputFilterResult info(const QString& check);
            OutputFilterResult error(const QString& check);
            QString addNickToEmptyNickList(const QString& nick, const QString& parameter);
            bool checkForEncodingConflict(QString *line, const QString& target);

            QString m_commandChar;
            QString m_myNick;
            QString m_destination;
            QString m_parameter;

            OutputFilterResult m_result;

            Server* m_server;
    };
}
#endif
