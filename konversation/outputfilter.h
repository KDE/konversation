/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  outputfilter.h  -  description
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef OUTPUTFILTER_H
#define OUTPUTFILTER_H

#include <qobject.h>
#include <qstring.h>

#include "identity.h"

/*
  @author Dario Abatianni
*/

class OutputFilter : public QObject
{
  Q_OBJECT

  public:
    OutputFilter();
    ~OutputFilter();

    QString& parse(const QString& myNick,const QString& line,const QString& name);
    void sendRequest(const QString &recipient,const QString &fileName,const QString &address,const QString &port,unsigned long size);
    void resumeRequest(const QString &sender,const QString &fileName,const QString &port,int startAt);
    void acceptRequest(const QString &recipient,const QString &fileName,const QString &port,int startAt);

    bool isAction();
    bool isCommand();
    bool isProgram();
    bool isQuery();

    QString& getOutput();
    QString& getServerOutput();
    QString& getType();

  signals:
    void openQuery(const QString& nick,const QString& hostmask); // hostmask currently unused
    void openDccSend(const QString &recipient, const QString &fileName);
    void requestDccSend();                        // Choose Recipient and File from requester
    void requestDccSend(const QString &recipient);       // Choose File from requester
    void openDccPanel();
    void closeDccPanel();
    void away();
    void unAway();
    void sendToAllChannels(const QString& text);
    void launchScript(const QString& parameter);

  public slots:
    void setCommandChar();
    void setIdentity(const Identity *newIdentity);

  protected:
    QString output;
    QString toServer;
    QString type;
    QString destination;

    QString commandChar;
    const Identity *identity;

    // message types
    bool action;
    bool command;
    bool program;
    bool query;

    void parseMsg(const QString &myNick,const QString &parameter);      // works
    void parseSMsg(const QString &parameter);     // works
    void parseQuery(const QString &parameter);    // works
    void parseDescribe(const QString &parameter);
    void parseNotice(const QString &parameter);   // works
    void parseJoin(const QString &parameter);     // works
    void parsePart(const QString &parameter);     // works
    void parseQuit(const QString &parameter);     // works
    void parseKick(const QString &parameter);     // works
    void parseKickBan(const QString &parameter);
    void parseBan(const QString &parameter);
    void parseUnban(const QString &parameter);
    void parseNames(const QString &parameter);
    void parseList(const QString &parameter);
    void parseOp(const QString &parameter);       // works
    void parseDeop(const QString &parameter);     // works
    void parseVoice(const QString &parameter);    // works
    void parseUnvoice(const QString &parameter);  // works
    void parseTopic(const QString &parameter);    // works
    void parseAway(const QString &parameter);     // works
    void parseCtcp(const QString &parameter);     // works
    void parsePing(const QString &parameter);
    void parseVersion(const QString &parameter);
    void parseServer(const QString &parameter);
    void parseConnect(const QString &parameter);
    void parseDcc(const QString &parameter);
    void parseInvite(const QString &parameter);
    void parseExec(const QString &parameter);

    void changeMode(const QString &parameter,char mode,char giveTake);
    bool isAChannel(const QString &check);
};

#endif
