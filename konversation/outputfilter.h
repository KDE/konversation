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

    QString& parse(const QString& line,const QString& name);

    bool isAction();
    bool isCommand();

    QString& getOutput();
    QString& getServerOutput();
    QString& getType();

  protected:
    QString output;
    QString toServer;
    QString type;
    QString destination;

    bool action;
    bool command;
    bool server;

    void parseMsg(QString parameter);
    void parseQuery(QString parameter);
    void parseDescribe(QString parameter);
    void parseJoin(QString parameter);
    void parsePart(QString parameter);
    void parseQuit(QString parameter);
    void parseKick(QString parameter);
    void parseKickBan(QString parameter);
    void parseBan(QString parameter);
    void parseUnban(QString parameter);
    void parseNames(QString parameter);
    void parseList(QString parameter);
    void parseOp(QString parameter);
    void parseDeop(QString parameter);
    void parseVoice(QString parameter);
    void parseUnvoice(QString parameter);
    void parseTopic(QString parameter);
    void parseCtcp(QString parameter);
    void parsePing(QString parameter);
    void parseVersion(QString parameter);
    void parseServer(QString parameter);
    void parseConnect(QString parameter);

    void changeMode(QString parameter,char mode,char giveTake);
    bool isAChannel(QString check);

  signals:
    void openQuery(const QString& nick,const QString& hostmask); // hostmask currently unused
};

#endif
