/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircview.h  -  description
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

// #include <qlistview.h>
// #include <qlabel.h>
// #include <qscrollview.h>
// #include <qgrid.h>
#include <qevent.h>

#include <ktextbrowser.h>

#ifndef IRCVIEW_H
#define IRCVIEW_H

/*
  @author Dario Abatianni
*/

// class IRCView : public QWidget
// class IRCView : public QScrollView
class IRCView : public KTextBrowser
// class IRCView : public QListView, public QLabel
{
  Q_OBJECT

  public:
    IRCView(QWidget* parent);
    ~IRCView();

    void clear();

  signals:
    void newText();      /* Notify container of new text */
    void newURL(QString url);
    void gotFocus();     /* So we can set focus to input line */
    void textToLog(const QString& text);

  public slots:
    void append(const char* nick,const char* message);
    void appendQuery(const char* nick,const char* message);
    void appendAction(const char* nick,const char* message);
    void appendServerMessage(const char* type,const char* message);
    void appendCommandMessage(const char* command,const char* message);
    void appendBacklogMessage(const char* firstColumn,const char* message);

  protected:
    QString filter(const QString& line,bool doHilight=true);
    void doAppend(QString line);
    void replaceDecoration(QString& line,char decoration,char replacement);

    void showEvent(QShowEvent* event);
    void focusInEvent(QFocusEvent* event);

    bool eventFilter(QObject* object,QEvent* event);

    QString buffer;
// QScrollView stuff
//    QGrid* grid;
};

#endif
