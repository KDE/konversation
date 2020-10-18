/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef INPUTFILTER_H
#define INPUTFILTER_H

#include "ignore.h"

#include <QObject>
#include <QStringList>
#include <QMap>

class Server;
class QDateTime;

class InputFilter : public QObject
{
    Q_OBJECT

    public:
        InputFilter();
        ~InputFilter() override;

        void setServer(Server* newServer);
        void parseLine(const QString &line);

        void reset();                             // reset AutomaticRequest, WhoRequestList

        // use this when the client does automatics, like userhost for finding hostmasks
        void setAutomaticRequest(const QString& command, const QString& name, bool yes);
        int getAutomaticRequest(const QString& command, const QString& name) const;
        void addWhoRequest(const QString& name);  // called from Server::send()
                                                  // to avoid duplicate requests
        bool isWhoRequestUnderProcess(const QString& name) const;
        void setLagMeasuring(bool yes);
        bool getLagMeasuring() const;

    Q_SIGNALS:
        void welcome(const QString& ownHost);
        void notifyResponse(const QString &nicksOnline);
                                                  // will be connected to Server::startReverseDccSendTransfer()
        void startReverseDccSendTransfer(const QString &sourceNick, const QStringList &dccArgument);
                                                  // will be connected to Server::startReverseDccChat()
        void startReverseDccChat(const QString &sourceNick, const QStringList &dccArgument);
                                                  // will be connected to Server::addDccGet()
        void addDccGet(const QString &sourceNick, const QStringList &dccArgument);
                                                  // will be connected to Server::resumeDccGetTransfer()
        void resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArgument);
                                                  // will be connected to Server::resumeDccSendTransfer()
        void resumeDccSendTransfer(const QString &sourceNick, const QStringList &dccArgument);
                                                  // will be connected to Server::rejectDccSendTransfer()
        void rejectDccSendTransfer(const QString &sourceNick, const QStringList &dccArgument);
                                                  // will be connected to Server::rejectDccChat()
        void rejectDccChat(const QString &sourceNick);
                                                  // will be connected to Server::userhost()
        void userhost(const QString& nick,const QString& hostmask,bool away,bool ircOp);
                                                  // will be connected to Server::setTopicAuthor()
        void topicAuthor(const QString& channel, const QString& author, QDateTime t);
        void endOfWho(const QString& target);
        void endOfNames(const QString& target);
        void addChannelListPanel();
        void addToChannelList(const QString& channel,int users,const QString& topic);
        void endOfChannelList();

        void invitation(const QString& nick,const QString& channel);

        void addDccChat(const QString& nick,const QStringList& arguments);

    private:
        void parseClientCommand(const QString &prefix, const QString &command, QStringList &parameterList, const QHash<QString, QString> &messageTags);
        void parseServerCommand(const QString &prefix, const QString &command, QStringList &parameterList, const QHash<QString, QString> &messageTags);
        void parseModes(const QString &sourceNick, const QStringList &parameterList, const QHash<QString, QString> &messageTags);
        void parsePrivMsg(const QString& prefix, QStringList& parameterList, const QHash<QString, QString> &messageTags);
        void parseNumeric(const QString &prefix, int command, QStringList &parameterList, const QHash<QString, QString> &messageTags);

        QHash<QString, QString> parseMessageTags(const QString &line, int *startOfMessage);

        bool isAChannel(const QString &check) const;
        bool isIgnore(const QString &pattern, Ignore::Type type) const;

    private:
        Server* m_server;
                                                  // automaticRequest[command][channel or nick]=count
        QMap< QString, QMap< QString, int > > m_automaticRequest;
        QStringList m_whoRequestList;
        bool m_lagMeasuring;

        /// Used when handling MOTD
        bool m_connecting;

        Q_DISABLE_COPY(InputFilter)
};

#endif
