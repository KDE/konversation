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
    void sendRequest(QString recipient,QString fileName,QString address,QString port,unsigned long size);
    void resumeRequest(QString sender,QString fileName,QString port,int startAt);
    void acceptRequest(QString recipient,QString fileName,QString port,int startAt);

    bool isAction();
    bool isCommand();
    bool isProgram();
    bool isQuery();

    QString& getOutput();
    QString& getServerOutput();
    QString& getType();

  signals:
    void openQuery(const QString& nick,const QString& hostmask); // hostmask currently unused
    void openDccSend(QString recipient,QString fileName);
    void requestDccSend(QString recipient);       // Choose Recipient / File from requester
    void openDccPanel();
    void closeDccPanel();

  public slots:
    void setCommandChar();

  protected:
    QString output;
    QString toServer;
    QString type;
    QString destination;

    QString commandChar;

    // message types
    bool action;
    bool command;
    bool program;
    bool query;

    void parseMsg(QString myNick,QString parameter);      // works
    void parseQuery(QString parameter);    // works
    void parseDescribe(QString parameter);
    void parseNotice(QString parameter);   // works
    void parseJoin(QString parameter);     // works
    void parsePart(QString parameter);     // works
    void parseQuit(QString parameter);     // works
    void parseKick(QString parameter);     // works
    void parseKickBan(QString parameter);
    void parseBan(QString parameter);
    void parseUnban(QString parameter);
    void parseNames(QString parameter);
    void parseList(QString parameter);
    void parseOp(QString parameter);       // works
    void parseDeop(QString parameter);     // works
    void parseVoice(QString parameter);    // works
    void parseUnvoice(QString parameter);  // works
    void parseTopic(QString parameter);    // works
    void parseAway(QString parameter);     // works
    void parseCtcp(QString parameter);     // works
    void parsePing(QString parameter);
    void parseVersion(QString parameter);
    void parseServer(QString parameter);
    void parseConnect(QString parameter);
    void parseDcc(QString parameter);

    void changeMode(QString parameter,char mode,char giveTake);
    bool isAChannel(QString check);
};

#endif
