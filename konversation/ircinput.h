/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircinput.h  -  The line input widget with chat enhanced functions
  begin:     Tue Mar 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IRCINPUT_H
#define IRCINPUT_H

#include <qstringlist.h>

#include <ktextedit.h>

/*
  @author Dario Abatianni
*/

class KCompletionBox;

class IRCInput : public KTextEdit
{
  Q_OBJECT

  public:
    IRCInput(QWidget* parent);
    ~IRCInput();

    void setCompletionMode(char mode);
    char getCompletionMode();
    void setOldCursorPosition(int pos);
    int getOldCursorPosition();

    virtual QSize sizeHint() const;

  signals:
    void nickCompletion();
    void endCompletion();   // tell channel that completion phase is over
    void history(bool up);
    void pageUp();
    void pageDown();
    void textPasted(QString text);
    void submit();

  public slots:
    void paste();
    void insert(const QString& text);
    void showCompletionList(const QStringList& nicks);

  protected slots:
    void getHistory(bool up);
    void insertCompletion(const QString& nick);

  protected:
    bool eventFilter(QObject *object,QEvent *event);
    void addHistory(const QString& text);
    bool checkPaste(QString& text);

    QStringList historyList;
    unsigned int lineNum;
    unsigned int oldPos;
    char completionMode;
    KCompletionBox* completionBox;
};

#endif
