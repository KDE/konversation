/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircinput.h  -  description
  begin:     Tue Mar 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IRCINPUT_H
#define IRCINPUT_H

#include <qlineedit.h>
#include <qstringlist.h>

/*
  @author Dario Abatianni
*/

class IRCInput : public QLineEdit
{
  Q_OBJECT

  public:
    IRCInput(QWidget* parent);
    ~IRCInput();

    void setCompletionMode(char mode) { completionMode=mode; };
    char getCompletionMode() { return completionMode; };
    void setOldCursorPosition(int pos) { oldPos=pos; };
    int getOldCursorPosition() { return oldPos; };

  signals:
    void nickCompletion();
    void history(bool up);
    void pageUp();
    void pageDown();

  protected slots:
    void getHistory(bool up);

  protected:
    bool eventFilter(QObject *object,QEvent *event);
    void addHistory(const QString& text);

    QStringList historyList;
    unsigned int lineNum;
    unsigned int oldPos;
    char completionMode;
};

#endif
