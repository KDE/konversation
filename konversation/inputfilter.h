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
*/

#ifndef INPUTFILTER_H
#define INPUTFILTER_H

#include <qobject.h>
#include <qstringlist.h>

#include "ignore.h"

/*
  @author Dario Abatianni
*/

class Server;
class QWidget;

class InputFilter : public QObject
{
  Q_OBJECT

  public:
    InputFilter();
    ~InputFilter();

    void setServer(Server* newServer);
    void parseLine(const QString &line, QWidget *mainWindow);

    // use this when the client does automatics, like userhost for finding hostmasks
    void setAutomaticRequest(const QString& command, const QString& name, bool yes);
    int getAutomaticRequest(const QString& command, const QString& name);
    void addWhoRequest(const QString& name);  // called from Server::send()
    bool isWhoRequestUnderProcess(const QString& name);  // to avoid duplicate requests
    void setLagMeasuring(bool yes);
    bool getLagMeasuring();

  signals:
    void welcome(const QString& ownHost);
    void notifyResponse(const QString &nicksOnline);
    void addDccGet(const QString &sourceNick, const QStringList &dccArgument); // will be connected to Server::addDccGet()
    void resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArgument); // will be connected to Server::resumeDccGetTransfer()
    void resumeDccSendTransfer(const QString &sourceNick, const QStringList &dccArgument); // will be connected to Server::resumeDccSendTransfer()
    void userhost(const QString& nick,const QString& hostmask,bool away,bool ircOp); // will be connected to Server::userhost()
    void topicAuthor(const QString& channel,const QString& author); // will be connected to Server::setTopicAuthor()
    void endOfWho(const QString& target); // for scheduling auto /WHO
    void addChannelListPanel();
    void addToChannelList(const QString& channel,int users,const QString& topic);
    void invitation(const QString& nick,const QString& channel);
    void away();
    void unAway();
    // will be connected via Server to KonversationMainWindow::addDccChat()
    void addDccChat(const QString& myNick,const QString& nick,const QString& numericalIp,const QStringList& arguments,bool listen);

  protected:
    void parseClientCommand(const QString &prefix, const QString &command, const QStringList &parameterList, const QString &trailing, QWidget *mainWindow);
    void parseServerCommand(const QString &prefix, const QString &command, const QStringList &parameterList, const QString &trailing);
    void parseModes(const QString &sourceNick, const QStringList &parameterList);

    bool isAChannel(const QString &check);
    bool isIgnore(const QString &pattern, Ignore::Type type);

    Server* server;
    QMap< QString, QMap<QString,int> > automaticRequest; // automaticRequest[command][channel or nick]=count
    QStringList whoRequestList;
    int lagMeasuring;


    QStringList newNickList;
    int m_debugCount;
};

#endif
