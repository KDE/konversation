/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  inputfilter.h  -  description
  begin:     Fri Jan 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef INPUTFILTER_H
#define INPUTFILTER_H

#include <qobject.h>

#include "ignore.h"

/*
  @author Dario Abatianni
  test
*/

class Server;

class InputFilter : public QObject
{
  Q_OBJECT

  public:
    InputFilter();
    ~InputFilter();

    void setServer(Server* newServer);
    void parseLine(const QString &line);

    // use this when the client does automatics, like userhost for finding hostmasks
    void setAutomaticRequest(bool yes);
    int getAutomaticRequest();

  signals:
    void welcome();
    void notifyResponse(const QString &nicksOnline);
    void addDccGet(const QString &sourceNick, const QStringList &dccArgument); // will be connected to Server::addDccGet()
    void resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArgument); // will be connected to Server::resumeDccGetTransfer()
    void resumeDccSendTransfer(const QString &sourceNick, const QStringList &dccArgument); // will be connected to Server::resumeDccSendTransfer()
    void userhost(const QString& nick,const QString& hostmask,bool away,bool ircOp); // will be connected to Server::userhost()
    void topicAuthor(const QString& channel,const QString& author); // will be connected to Server::setTopicAuthor()
    void addChannelListPanel();
    void addToChannelList(const QString& channel,int users,const QString& topic);

  protected:
    void parseClientCommand(const QString &prefix, const QString &command, const QStringList &parameterList, const QString &trailing);
    void parseServerCommand(const QString &prefix, const QString &command, const QStringList &parameterList, const QString &trailing);
    void parseModes(const QString &sourceNick, const QStringList &parameterList);

    bool isAChannel(const QString &check);
    bool isIgnore(const QString &pattern, Ignore::Type type);

    Server* server;
    bool welcomeSent;
    int automaticRequest;
};

#endif
