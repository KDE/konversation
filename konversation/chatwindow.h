/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  chatwindow.h  -  description
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qfile.h>
#include <qvbox.h>

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "ircview.h"
#include "outputfilter.h"

/*
  @author Dario Abatianni
*/

class Server;

class ChatWindow : public QVBox
{
  Q_OBJECT

  public:
    ChatWindow(QWidget* parent);
    ~ChatWindow();

    enum WindowType
    {
      Status=0,
      Channel,
      Query,
      DccChat,
      DccPanel
    };

    void setServer(Server* newServer);
    void setTextView(IRCView* newView);
    IRCView* getTextView() { return textView; };
    void setLog(bool activate) { log=activate; };

    void setName(QString newName);
    QString& getName();

    void setType(WindowType newType);
    WindowType getType();

    void append(const char* nickname,const char* message);
    void appendQuery(const char* nickname,const char* message);
    void appendAction(const char* nickname,const char* message);
    void appendServerMessage(const char* type,const char* message);
    void appendCommandMessage(const char* command,const char* message);
    void appendBacklogMessage(const char* firstColumn,const char* message);

    QWidget* parentWidget;

  public slots:
    void logText(const QString& text);

  protected:
    bool log;
    bool firstLog;
    void setLogfileName(const QString& name);
    void cdIntoLogPath();

    QString name;
    QString logName;

    IRCView* textView;
    Server* server;
    QFile logfile;
    OutputFilter filter;
    WindowType type;
};

#endif
