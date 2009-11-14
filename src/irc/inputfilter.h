/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef INPUTFILTER_H
#define INPUTFILTER_H

#include "ignore.h"

#include <QObject>
#include <QStringList>
#include <QMap>

class Server;
class Query;
class QDateTime;

class InputFilter : public QObject
{
    Q_OBJECT

    public:
        InputFilter();
        ~InputFilter();

        void setServer(Server* newServer);
        void parseLine(const QString &line);

        void reset();                             // reset AutomaticRequest, WhoRequestList

        // use this when the client does automatics, like userhost for finding hostmasks
        void setAutomaticRequest(const QString& command, const QString& name, bool yes);
        int getAutomaticRequest(const QString& command, const QString& name);
        void addWhoRequest(const QString& name);  // called from Server::send()
                                                  // to avoid duplicate requests
        bool isWhoRequestUnderProcess(const QString& name);
        void setLagMeasuring(bool yes);
        bool getLagMeasuring();

    signals:
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
        void endOfWho(const QString& target);     // for scheduling auto /WHO
        void addChannelListPanel();
        void addToChannelList(const QString& channel,int users,const QString& topic);
        void endOfChannelList();

        void invitation(const QString& nick,const QString& channel);

        void addDccChat(const QString& nick,const QStringList& arguments);

    protected:
        void parseClientCommand(const QString &prefix, const QString &command, QStringList &parameterList);
        void parseServerCommand(const QString &prefix, const QString &command, QStringList &parameterList);
        void parseModes(const QString &sourceNick, const QStringList &parameterList);
        void parsePrivMsg(const QString& prefix, QStringList& parameterList);

        bool isAChannel(const QString &check);
        bool isIgnore(const QString &pattern, Ignore::Type type);

        Server* server;
                                                  // automaticRequest[command][channel or nick]=count
        QMap< QString, QMap< QString, int > > automaticRequest;
        QStringList whoRequestList;
        int lagMeasuring;

        Query* query;

        int m_debugCount;

        /// Used when handling MOTD
        bool m_connecting;
};
#endif
