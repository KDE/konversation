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

/*
  @author Dario Abatianni
*/

class Server;

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
      Copy,CopyUrl,SelectAll,
      Search,
      SendFile
    };

  signals:
    // Notify container of new text and highlight state
    void newText(const QString& highlightColor);
    void gotFocus();                  // So we can set focus to input line
    void textToLog(const QString& text);
    void sendFile();

  public slots:
    void append(const QString& nick,const QString& message);
    void appendRaw(const QString& message);
    void appendQuery(const QString& nick,const QString& message);
    void appendAction(const QString& nick,const QString& message);
    void appendServerMessage(const QString& type,const QString& message);
    void appendCommandMessage(const QString& command,const QString& message);
    void appendBacklogMessage(const QString& firstColumn,const QString& message);
    void search();

    void pageUp();
    void pageDown();

  protected slots:
    void highlightedSlot(const QString& link);

  protected:
    QString filter(const QString& line,const QString& who=NULL,bool doHilight=true);
    void doAppend(QString line,bool suppressTimestamps=false);
    void replaceDecoration(QString& line,char decoration,char replacement);

    void showEvent(QShowEvent* event);
    void focusInEvent(QFocusEvent* event);

    bool eventFilter(QObject* object,QEvent* event);

    bool contextMenu(QContextMenuEvent* ce);

    // used by search function
    int findParagraph;
    int findIndex;

    QString highlightColor;
    bool copyUrlMenu;
    QString urlToCopy;

    QString buffer;
    Server *server;
    QPopupMenu* popup;
};

#endif
