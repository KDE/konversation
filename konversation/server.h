/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  server.h  -  Server Class to handle connection to the IRC server
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef SERVER_H
#define SERVER_H

#include <kapplication.h>

#include <qlist.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qlistview.h>

#include "inputfilter.h"
#include "outputfilter.h"
#include "ircserversocket.h"
#include "identity.h"
#include "ircresolver.h"

/*
  @author Dario Abatianni
*/

class Channel;
class Query;
class ServerWindow;

class Server : public QObject
{
  Q_OBJECT

  public:
    Server(int number);
    ~Server();

    QString getServerName();
    const Identity *getIdentity();
    int getPort();
    bool getAutoJoin();
    void setAutoJoin(bool on);

    bool isConnected();

    QString getAutoJoinChannel();
    void setAutoJoinChannel(const QString &channel);

    QString getAutoJoinChannelKey();
    void setAutoJoinChannelKey(const QString &key);

    void setDeliberateQuit(bool on);
    bool getDeliberateQuit();

    QString getNextNickname();

    void setIrcName(const QString &newIrcName);
    QString getIrcName();

    void addNickToChannel(const QString &channelName, const QString &nickname, const QString &hostmask, bool op, bool voice);
    void addHostmaskToNick(const QString &sourceNick, const QString &sourceHostmask);
    void nickJoinsChannel(const QString &channelName, const QString &nickname, const QString &hostmask);
    void renameNick(const QString &nickname,const QString &newNick);
    void removeNickFromChannel(const QString &channelName, const QString &nickname, const QString &reason, bool quit=false);
    void nickWasKickedFromChannel(const QString &channelName, const QString &nickname, const QString &kicker, const QString &reason);
    void removeNickFromServer(const QString &nickname, const QString &reason);

    bool isNickname(const QString& compare);
    QString getNickname();
    OutputFilter& getOutputFilter();

    void joinChannel(const QString& name, const QString& hostmask, const QString& key);
    void removeChannel(Channel* channel);
    void appendToChannel(const char* channel,const char* nickname, const char* message);
    void appendActionToChannel(const char* channel, const char* nickname, const char* message);
    void appendServerMessageToChannel(const char* channel, const char* type, const char* message);
    void appendCommandMessageToChannel(const char* channel, const char* command, const char* message);
    void appendStatusMessage(const char* type,const char* message);

    void dcopSay(const QString& target,const QString& command);
    void dcopInfo(const QString& string);
    void ctcpReply(const QString& receiver, const QString& text);

    void setChannelTopic(const QString& channel, const QString& topic);
    void setChannelTopic(const QString& nickname, const QString& channel, const QString& topic); // Overloaded
    void updateChannelMode(const QString& nick, const QString& channel, char mode, bool plus, const QString& parameter);
    void updateChannelModeWidgets(const QString& channel, char mode, const QString& parameter);
    void updateChannelQuickButtons(QStringList newButtons);
    void updateFonts();
    void setShowQuickButtons(bool state);
    void setShowModeButtons(bool state);

    QString getNextQueryName();
    ServerWindow* getServerWindow();
    void setServerWindow(ServerWindow* newWindow); // don't use this other than in ServerWindow!

    void appendToQuery(const char* queryName,const char* message);
    void appendActionToQuery(const char* queryName,const char* message);
    void appendServerMessageToQuery(const char* queryName, const char* type, const char* message);
    void appendCommandMessageToQuery(const char* queryName, const char* command, const char* message);

    Channel* getChannelByName(const char* name);
    Query* getQueryByName(const char* name);
    QString parseWildcards(const QString& toParse, const QString& nickname, const QString& channelName, const QString &channelKey, const QStringList& nickList, const QString& queryName, const QString& parameter);
    QString parseWildcards(const QString& toParse, const QString& nickname, const QString& channelName, const QString &channelKey, const QString& nick, const QString& queryName, const QString& parameter);

    QString getAutoJoinCommand();

  signals:
    void nicknameChanged(const QString&);
    void serverLag(int msec);
    void tooLongLag(int msec); // waiting too long for 303 response
    void resetLag();
    void nicksNowOnline(const QStringList& list); // Will be emitted when new 303 came in
    void addDccPanel(); // will be connected to ServerWindow::addDccPanel()
    void closeDccPanel(); // will be connected to ServerWindow::closeDccPanel()
    void deleted(Server* myself); // will be connected to KonversationApplication::removeServer()
    void repaintTabs(); // will be connected to LedTabBar::repaintTabs();

  public slots:
    void connectToIRCServer();
    void queue(const QString &buffer);
    void setNickname(const QString &newNickname);
    void addQuery(const QString &nickname, const QString &hostmask);
    void closeQuery(const QString &name);
    void closeChannel(const QString &name);
    void requestDccPanel();
    void requestCloseDccPanel();
    void requestBan(const QStringList& users,const QString& channel,const QString& option);
    void addDccSend(const QString &recipient,const QString &file);
    void removeQuery(Query *query);
    void startNotifyTimer(int msec=0);

  protected slots:
    void addRawLog();
    void closeRawLog();
    void ircServerConnectionSuccess();
    void incoming();
    void processIncomingData();
    void send();
    void broken(int state);
    void notifyTimeout();
    void notifyCheckTimeout();
    void connectionEstablished();
    void notifyAction(QListViewItem* item);
    void notifyResponse(const QString &nicksOnline);
    void addDccGet(const QString &sourceNick, const QStringList &dccArguments);
    void requestDccSend();                                               // -> to outputFilter, dccPanel
    void requestDccSend(const QString &recipient);                                  // -> to outputFilter
    void resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArguments);  // -> to inputFilter
    void resumeDccSendTransfer(const QString &sourceNick, const QStringList &dccArguments); // -> to inputFilter
    void dccSendRequest(const QString &recipient, const QString &fileName, const QString &address, const QString &port, unsigned long size);
    void dccResumeGetRequest(const QString &sender, const QString &fileName, const QString &port, int startAt);
    void dccGetDone(const QString &fileName);
    void dccSendDone(const QString &fileName);
    void away();
    void unAway();
    void sendToAllChannels(const QString &text);
    void scriptNotFound(const QString& name);
    void scriptExecutionError(const QString& name);

  protected:
    // constants
    static const int BUFFER_LEN=513;

    bool eventFilter(QObject* parent, QEvent *event);

    void lookupFinished();
    void startNotifyCheckTimer();
    bool isAChannel(const QString &check);
    void setIdentity(const Identity *newIdentity);

    unsigned int completeQueryPosition;
    unsigned int tryNickNumber;
    unsigned int reconnectCounter;

    QString serverName;
    QString bot;
    QString botPassword;
    int serverPort;

    IRCResolver resolver;
    const Identity *identity;

    bool autoJoin;
    bool autoRejoin;
    bool autoReconnect;
    bool deliberateQuit;

    QString autoJoinChannel;
    QString autoJoinChannelKey;

    ServerWindow* serverWindow;
    IRCServerSocket serverSocket;

    QTimer reconnectTimer;
    QTimer incomingTimer;

    QTimer notifyTimer;
    QTimer notifyCheckTimer; // Checks if the ISON reply needs too long
    QTime notifySent;
    QStringList notifyCache; // List of users found with ISON
    int checkTime;           // Time elapsed while waiting for server 303 response

    QString ircName;
    QString inputBuffer;
    QString outputBuffer;
    QString nickname;
    QString serverKey;
    QString lastDccDir;

    QPtrList<Channel> channelList;
    QPtrList<Query> queryList;

    InputFilter inputFilter;
    OutputFilter outputFilter;

    QDateTime awayTime;
    bool isAway;
};

#endif
