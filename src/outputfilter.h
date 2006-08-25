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
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#ifndef OUTPUTFILTER_H
#define OUTPUTFILTER_H

#include <qobject.h>
#include <qstring.h>
#include <kurl.h>
#include <kio/global.h>

#include "identity.h"

/*
  @author Dario Abatianni
*/

class Server;
class ChatWindow;

namespace Konversation
{
    typedef enum MessageType
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
            OutputFilter(Server* server);
            ~OutputFilter();

            QStringList splitForEncoding(QString inputLine, int MAX);
            OutputFilterResult parse(const QString& myNick,const QString& line,const QString& name);
            OutputFilterResult sendRequest(const QString &recipient,const QString &fileName,const QString &address,
                const QString &port,unsigned long size);
            OutputFilterResult resumeRequest(const QString &sender,const QString &fileName,const QString &port,KIO::filesize_t startAt);
            OutputFilterResult acceptRequest(const QString &recipient,const QString &fileName,const QString &port,int startAt);
            bool replaceAliases(QString& line);

        signals:
            void openDccSend(const QString &recipient, KURL kurl);
            void requestDccSend();                // Choose Recipient and File from requester
                                                  // Choose File from requester
            void requestDccSend(const QString& recipient);
            void requestDccChat(const QString& nick);
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
            void connectToServerGroup(const QString& serverGroup);
            void connectToServer(const QString& server, const QString& port, const QString& password);

            void showView(ChatWindow* view);


        public slots:
            void setCommandChar();
            OutputFilterResult execBan(const QString& mask,const QString& channels);
            OutputFilterResult execUnban(const QString& mask,const QString& channels);

        protected:
            OutputFilterResult parseMsg(const QString& myNick,const QString& parameter, bool focusQueryWindow);
            OutputFilterResult parseSMsg(const QString& parameter);
            OutputFilterResult parseDescribe(const QString& parameter);
            OutputFilterResult parseNotice(const QString& parameter);
            OutputFilterResult parseJoin(QString parameter);
            OutputFilterResult parsePart(const QString& parameter);
            OutputFilterResult parseQuit(const QString& parameter);
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
            OutputFilterResult parseAway(QString& parameter);
            OutputFilterResult parseBack();
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
            void parseAaway(const QString& parameter);
            OutputFilterResult parseAme(const QString& parameter);
            OutputFilterResult parseAmsg(const QString& parameter);
            OutputFilterResult parsePrefs(const QString& parameter);
            OutputFilterResult parseOmsg(const QString& parameter);
            OutputFilterResult parseOnotice(const QString& parameter);
            void parseCharset(const QString& charset);
            void parseCycle();
            OutputFilterResult parseSetKey(const QString& parameter);
            OutputFilterResult parseDelKey(const QString& parameter);
            OutputFilterResult parseDNS(const QString& parameter);

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
