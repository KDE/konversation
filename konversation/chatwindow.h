/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  chatwindow.h  -  Base class for all chat panels
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <qfile.h>
#include <qvbox.h>

#include "outputfilter.h"
#include "identity.h"
#include "scriptlauncher.h"

/*
  @author Dario Abatianni
*/

class QFile;

class IRCView;
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
      DccPanel,
      RawLog,
      Notice,
      SNotice,
      ChannelList,
      Konsole,
      UrlCatcher,
      NicksOnline
    };

    void setServer(Server* newServer);
    Server* getServer();
    void setIdentity(const Identity *newIdentity);
    void setTextView(IRCView* newView);
    IRCView* getTextView();
    void setLog(bool activate);

    void setName(const QString& newName);
    QString& getName();

    void setType(WindowType newType);
    WindowType getType();

    void append(const QString& nickname,const QString& message);
    void appendRaw(const QString& message, bool suppressTimestamps=false);
    void appendQuery(const QString& nickname,const QString& message);
    void appendAction(const QString& nickname,const QString& message);
    void appendServerMessage(const QString& type,const QString& message);
    void appendCommandMessage(const QString& command,const QString& message,bool important=true, bool parseURL = true);
    void appendBacklogMessage(const QString& firstColumn,const QString& message);

    QWidget* parentWidget;

    virtual QString getTextInLine();
    virtual void closeYourself();
    virtual bool frontView();
    virtual bool searchView();

  signals:
    void nameChanged(ChatWindow* view,const QString& newName);
    void online(ChatWindow* myself,bool state);

  public slots:
    void logText(const QString& text);
    void openLogfile();
    void serverOnline(bool state);
    
    virtual void adjustFocus()=0;
    virtual void appendInputText(const QString&);
    virtual void indicateAway(bool away);

  protected:
    bool log;
    bool firstLog;

    void setLogfileName(const QString& name);
    void cdIntoLogPath();
    int spacing();
    int margin();

    QString name;
    QString logName;

    QFont font;

    IRCView* textView;
    Server* server;
    Identity identity;
    QFile logfile;
    OutputFilter filter;
    WindowType type;
    ScriptLauncher scriptLauncher;
};

#endif
