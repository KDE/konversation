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
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#ifndef OUTPUTFILTER_H
#define OUTPUTFILTER_H

#include "identity.h"
#include "common.h"

#include <qobject.h>
#include <qstring.h>
#include <kurl.h>
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

            QStringList splitForEncoding(const QString& inputLine, int max);
            OutputFilterResult parse(const QString& myNick,const QString& line,const QString& name);

            // dcc send
            OutputFilterResult sendRequest(const QString &recipient,const QString &fileName,const QString &address,uint port,unsigned long size);
            OutputFilterResult passiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,unsigned long size,const QString &token);
            OutputFilterResult acceptResumeRequest(const QString &recipient,const QString &fileName,uint port,int startAt);
            OutputFilterResult acceptPassiveResumeRequest(const QString &recipient,const QString &fileName,uint port,int startAt,const QString &token);

            // dcc recv
            OutputFilterResult resumeRequest(const QString &sender,const QString &fileName,uint port,KIO::filesize_t startAt);
            OutputFilterResult resumePassiveRequest(const QString &sender,const QString &fileName,uint port,KIO::filesize_t startAt,const QString &token);
            OutputFilterResult acceptPassiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,uint port,unsigned long size,const QString &token);
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
                           uint port = 0,
                           const QString& password = "",
                           const QString& nick = "",
                           const QString& channel = "",
                           bool useSSL = false
            );

            void showView(ChatWindow* view);
            void encodingChanged ();


        public slots:
            void setCommandChar();
            OutputFilterResult execBan(const QString& mask,const QString& channels);
            OutputFilterResult execUnban(const QString& mask,const QString& channels);

        protected:
            OutputFilterResult parseMsg(const QString& myNick,const QString& parameter, bool focusQueryWindow);
            OutputFilterResult parseSMsg(const QString& parameter);
            OutputFilterResult parseMe(const QString &parameter, const QString &destination);
            OutputFilterResult parseDescribe(const QString& parameter);
            OutputFilterResult parseNotice(const QString& parameter);
            OutputFilterResult parseJoin(QString& parameter);
            OutputFilterResult parsePart(const QString& parameter);
            OutputFilterResult parseQuit(const QString& parameter);
            OutputFilterResult parseClose(QString parameter);
            OutputFilterResult parseKick(const QString& parameter);
            OutputFilterResult parseKickBan(const QString& parameter);
            OutputFilterResult parseBan(const QString& parameter, bool kick = false);
            OutputFilterResult parseUnban(const QString& parameter);
            OutputFilterResult parseNames(const QString& parameter);
            OutputFilterResult parseList(const QString& parameter);
            OutputFilterResult parseOp(const QString& parameter);
            OutputFilterResult parseDeop(const QString& ownNick, const QString& parameter);
            OutputFilterResult parseHop(const QString& parameter);
            OutputFilterResult parseDehop(const QString& ownNick, const QString& parameter);
            OutputFilterResult parseVoice(const QString& parameter);
            OutputFilterResult parseUnvoice(const QString& ownNick, const QString& parameter);
            OutputFilterResult parseTopic(const QString& parameter);
            void parseAway(QString& parameter);
            void parseBack();
            OutputFilterResult parseCtcp(const QString& parameter);
            OutputFilterResult parsePing(const QString& parameter);
            OutputFilterResult parseVersion(const QString& parameter);
            void parseServer(const QString& parameter);
            void parseReconnect();
            OutputFilterResult parseConnect(const QString& parameter);
            OutputFilterResult parseInvite(const QString& parameter);
            OutputFilterResult parseExec(const QString& parameter);
            OutputFilterResult parseNotify(const QString& parameter);
            OutputFilterResult parseOper(const QString& myNick,const QString& parameter);
            OutputFilterResult parseDcc(const QString& parameter);
            OutputFilterResult parseRaw(const QString& parameter);
            OutputFilterResult parseIgnore(const QString& parameter);
            OutputFilterResult parseUnignore(const QString& parameter);
            OutputFilterResult parseQuote(const QString& parameter);
            OutputFilterResult parseSay(const QString& parameter);
            void parseKonsole();
            OutputFilterResult parseAme(const QString& parameter);
            OutputFilterResult parseAmsg(const QString& parameter);
            OutputFilterResult parseOmsg(const QString& parameter);
            OutputFilterResult parseOnotice(const QString& parameter);
            OutputFilterResult parseCharset(const QString& charset);
            void parseCycle();
            OutputFilterResult parseSetKey(const QString& parameter);
            OutputFilterResult parseDelKey(const QString& parameter);
            OutputFilterResult parseShowKey(const QString& parameter);
            OutputFilterResult parseDNS(const QString& parameter);
            OutputFilterResult parseKill(const QString& parameter);
            OutputFilterResult parseShowTuner(const QString &p);

            OutputFilterResult changeMode(const QString& parameter,char mode,char giveTake);
            bool isAChannel(const QString& check);
            OutputFilterResult usage(const QString& check);
            OutputFilterResult info(const QString& check);
            OutputFilterResult error(const QString& check);

            QString addNickToEmptyNickList(const QString& nick, const QString& parameter);

        private:
            QString destination;
            QString commandChar;

            Server* m_server;
    };
}
#endif
