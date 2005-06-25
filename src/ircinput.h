/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The line input widget with chat enhanced functions
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
class QMouseEvent;

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
    QString lastCompletion() const { return m_lastCompletion; }

    virtual QSize sizeHint() const;
    QString text() const;

  signals:
    void nickCompletion();
    void endCompletion();   // tell channel that completion phase is over
    void history(bool up);
    void textPasted(const QString& text);
    void submit();
    void envelopeCommand();

  public slots:
    void paste();
    void paste(bool useSelection);
    void showCompletionList(const QStringList& nicks);
    void setText(const QString& text);
    void setLastCompletion(const QString& completion);

  protected slots:
    void getHistory(bool up);
    void insertCompletion(const QString& nick);

  protected:
    bool eventFilter(QObject *object,QEvent *event);
    void addHistory(const QString& text);
    bool checkPaste(QString& text);
    void contentsMouseReleaseEvent(QMouseEvent *);
    virtual void keyPressEvent(QKeyEvent* e);

    QStringList historyList;
    unsigned int lineNum;
    unsigned int oldPos;
    char completionMode;
    KCompletionBox* completionBox;
    QString m_lastCompletion;
    bool m_useSelection;
};

#endif
