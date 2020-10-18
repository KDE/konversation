/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2005 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2005 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2005-2009 Eike Hein <hein@kde.org>
*/

#ifndef OUTPUTFILTER_H
#define OUTPUTFILTER_H

#include "identity.h"
#include "common.h"

#include <KIO/Global>

#include <QObject>
#include <QString>
#include <QSet>
#include <QPointer>
#include <QUrl>

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

    struct OutputFilterInput
    {
        QString parameter;
        QString destination;
        QString myNick;
        QPointer<ChatWindow> context;
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
            ~OutputFilter() override;

            static const QSet<QString>& supportedCommands() { return m_commands; }

            QStringList splitForEncoding(const QString& destination, const QString& inputLine, int max, int segments = -1);
            OutputFilterResult parse(const QString& myNick, const QString& line, const QString& destination, ChatWindow* inputContext = nullptr);

            // dcc send
            OutputFilterResult sendRequest(const QString &recipient, const QString &fileName, const QString &address, quint16 port,quint64 size);
            OutputFilterResult passiveSendRequest(const QString& recipient, const QString &fileName, const QString &address, quint64 size, const QString &token);
            OutputFilterResult acceptResumeRequest(const QString &recipient, const QString &fileName, quint16 port, quint64 startAt);
            OutputFilterResult acceptPassiveResumeRequest(const QString &recipient, const QString &fileName, quint16 port, quint64 startAt, const QString &token);

            // dcc recv
            OutputFilterResult resumeRequest(const QString &sender, const QString &fileName, quint16 port, KIO::filesize_t startAt);
            OutputFilterResult resumePassiveRequest(const QString &sender, const QString &fileName, quint16 port, KIO::filesize_t startAt, const QString &token);
            OutputFilterResult acceptPassiveSendRequest(const QString& recipient, const QString &fileName, const QString &address, quint16 port, quint64 size, const QString &token);
            OutputFilterResult rejectDccSend(const QString& partnerNick, const QString& fileName);

            // dcc chat
            OutputFilterResult rejectDccChat(const QString& partnerNick, const QString& extension);
            OutputFilterResult requestDccChat(const QString& partnerNick, const QString& extension, const QString& numericalOwnIp, quint16 ownPort);
            OutputFilterResult passiveChatRequest(const QString& recipient, const QString& extension, const QString& address, const QString& token);
            OutputFilterResult acceptPassiveChatRequest(const QString& recipient, const QString& extension, const QString& numericalOwnIp, quint16 ownPort, const QString& token);

            static bool replaceAliases(QString& line, ChatWindow* context = nullptr);

        Q_SIGNALS:
            void openDccSend(const QString &recipient, const QUrl& url);
            void requestDccSend();                // Choose Recipient and File from requester
                                                  // Choose File from requester
            void requestDccSend(const QString& recipient);
            void openDccChat(const QString& nick);
            void openDccWBoard (const QString& nick);
            void addDccPanel();
            void closeDccPanel();
            void acceptDccGet(const QString& nick, const QString& file);
            void openRawLog(bool show);
            void closeRawLog();
            void openKonsolePanel();
            void openChannelList(const QString& parameter);
            void sendToAllChannels(const QString& text);
            void launchScript(int connectionId, const QString& target, const QString& parameter);
            void banUsers(const QStringList& userList,const QString& channel,const QString& option);
            void unbanUsers(const QString& mask,const QString& channel);
            void multiServerCommand(const QString& command, const QString& parameter);
            void reconnectServer(const QString& quitMessage);
            void disconnectServer(const QString& quitMessage);
            void quitServer(const QString& quitMessage);

            void connectTo(Konversation::ConnectionFlag flag,
                           const QString& hostName,
                           const QString& port = QString(),
                           const QString& password = QString(),
                           const QString& nick = QString(),
                           const QString& channel = QString(),
                           bool useSSL = false
            );

            void showView(ChatWindow* view);
            void encodingChanged ();

        public Q_SLOTS:
            OutputFilterResult execBan(const QString& mask,const QString& channels);
            OutputFilterResult execUnban(const QString& mask,const QString& channels);

        private Q_SLOTS:
            OutputFilterResult command_op(const OutputFilterInput& input);
            OutputFilterResult command_deop(const OutputFilterInput& input);
            OutputFilterResult command_hop(const OutputFilterInput& input);
            OutputFilterResult command_dehop(const OutputFilterInput& input);
            OutputFilterResult command_voice(const OutputFilterInput& input);
            OutputFilterResult command_unvoice(const OutputFilterInput& input);
            OutputFilterResult command_devoice(const OutputFilterInput& input);
            OutputFilterResult command_join(const OutputFilterInput& _input);
            OutputFilterResult command_j(const OutputFilterInput& input);
            OutputFilterResult command_kick(const OutputFilterInput& input);
            OutputFilterResult command_part(const OutputFilterInput& input);
            OutputFilterResult command_leave(const OutputFilterInput& input);
            OutputFilterResult command_topic(const OutputFilterInput& input);
            OutputFilterResult command_away(const OutputFilterInput& input);
            OutputFilterResult command_unaway(const OutputFilterInput& input);
            OutputFilterResult command_back(const OutputFilterInput& input);
            OutputFilterResult command_aaway(const OutputFilterInput& input);
            OutputFilterResult command_aunaway(const OutputFilterInput& input);
            OutputFilterResult command_aback(const OutputFilterInput& input);
            OutputFilterResult command_names(const OutputFilterInput& input);
            OutputFilterResult command_close(const OutputFilterInput& _input);
            OutputFilterResult command_reconnect(const OutputFilterInput& input);
            OutputFilterResult command_disconnect(const OutputFilterInput& input);
            OutputFilterResult command_quit(const OutputFilterInput& input);
            OutputFilterResult command_notice(const OutputFilterInput& input);
            OutputFilterResult command_me(const OutputFilterInput& input);
            OutputFilterResult command_msg(const OutputFilterInput& input);
            OutputFilterResult command_m(const OutputFilterInput& input);
            OutputFilterResult command_query(const OutputFilterInput& input);
            OutputFilterResult command_smsg(const OutputFilterInput& input);
            OutputFilterResult command_ping(const OutputFilterInput& input);
            OutputFilterResult command_ctcp(const OutputFilterInput& input);
            OutputFilterResult command_ame(const OutputFilterInput& input);
            OutputFilterResult command_amsg(const OutputFilterInput& input);
            OutputFilterResult command_omsg(const OutputFilterInput& input);
            OutputFilterResult command_onotice(const OutputFilterInput& input);
            OutputFilterResult command_quote(const OutputFilterInput& input);
            OutputFilterResult command_say(const OutputFilterInput& input);
            OutputFilterResult command_dcc(const OutputFilterInput& _input);
            OutputFilterResult command_invite(const OutputFilterInput& input);
            OutputFilterResult command_exec(const OutputFilterInput& input);
            OutputFilterResult command_raw(const OutputFilterInput& input);
            OutputFilterResult command_notify(const OutputFilterInput& input);
            OutputFilterResult command_oper(const OutputFilterInput& input);
            OutputFilterResult command_ban(const OutputFilterInput& input);
            OutputFilterResult command_kickban(const OutputFilterInput& input);
            OutputFilterResult command_unban(const OutputFilterInput& input);
            OutputFilterResult command_ignore(const OutputFilterInput& input);
            OutputFilterResult command_unignore(const OutputFilterInput& input);
            OutputFilterResult command_server(const OutputFilterInput& input);
            OutputFilterResult command_charset(const OutputFilterInput& input);
            OutputFilterResult command_encoding(const OutputFilterInput& input);
            OutputFilterResult command_setkey(const OutputFilterInput& input);
            OutputFilterResult command_keyx(const OutputFilterInput& input);
            OutputFilterResult command_delkey(const OutputFilterInput& _input);
            OutputFilterResult command_showkey(const OutputFilterInput& _input);
            OutputFilterResult command_kill(const OutputFilterInput& input);
            OutputFilterResult command_dns(const OutputFilterInput& input);
            OutputFilterResult command_list(const OutputFilterInput& input);
            OutputFilterResult command_konsole(const OutputFilterInput& input);
            OutputFilterResult command_queuetuner(const OutputFilterInput& input);
            OutputFilterResult command_sayversion(const OutputFilterInput& input);
            OutputFilterResult command_cycle(const OutputFilterInput& input);
            OutputFilterResult command_clear(const OutputFilterInput& input);
            OutputFilterResult command_umode(const OutputFilterInput& input);

        private:
            static void fillCommandList();

            OutputFilterResult handleMsg(const QString& parameter, bool commandIsQuery);
            OutputFilterResult handleCtcp(const QString& parameter);
            OutputFilterResult handleBan(const OutputFilterInput& input, bool kick);

            OutputFilterResult changeMode(const QString& parameter, const QString& destination, char mode, char giveTake);
            bool isAChannel(const QString& check) const;
            bool isParameter(const QString& parameter, const QString& string) const;
            static OutputFilterResult usage(const QString& check);
            static OutputFilterResult info(const QString& check);
            static OutputFilterResult error(const QString& check);
            QString addNickToEmptyNickList(const QString& nick, const QString& parameter);
            bool checkForEncodingConflict(QString *line, const QString& target);

        private:
            static QSet<QString> m_commands;

            Server* m_server;

            Q_DISABLE_COPY(OutputFilter)
    };
}

#endif
