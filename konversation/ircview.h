/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircview.h  -  the text widget used for all text based panels
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef IRCVIEW_H
#define IRCVIEW_H

#include <qevent.h>
#include <qpopupmenu.h>

#include <ktextbrowser.h>

#include "server.h"

/*
  @author Dario Abatianni
*/

class IRCView : public KTextBrowser
{
  Q_OBJECT

  public:
    IRCView(QWidget* parent,Server* newServer);
    ~IRCView();

    void clear();
    void setServer(Server* server);

    enum PopupIDs
    {
      Copy,SelectAll,
      SendFile
    };

  signals:
    void newText();      // Notify container of new text
    void newURL(QString url);
    void gotFocus();     // So we can set focus to input line
    void textToLog(const QString& text);
    void sendFile();

  public slots:
    void append(const char* nick,const char* message);
    void appendQuery(const char* nick,const char* message);
    void appendAction(const char* nick,const char* message);
    void appendServerMessage(const char* type,const char* message);
    void appendCommandMessage(const char* command,const char* message);
    void appendBacklogMessage(const char* firstColumn,const char* message);

  protected:
    QString filter(const QString& line,const QString& who=NULL,bool doHilight=true);
    void doAppend(QString line,bool suppressTimestamps=false);
    void replaceDecoration(QString& line,char decoration,char replacement);

    void showEvent(QShowEvent* event);
    void focusInEvent(QFocusEvent* event);

    bool eventFilter(QObject* object,QEvent* event);

    bool contextMenu(QContextMenuEvent* ce);
    
    QString buffer;
    Server *server;
    QPopupMenu* popup;
};

#endif
