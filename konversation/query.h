/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  query.h  -  description
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef QUERY_H
#define QUERY_H

#include <qobject.h>
#include <qvbox.h>
#include <qiconset.h>
#include <qcheckbox.h>
#include <qlineedit.h>

#include "chatwindow.h"

/*
  @author Dario Abatianni
*/

/* TODO: Idle counter to close query after XXX minutes of inactivity */
/* TODO: Use /USERHOST to check if queries are still valid */

class IRCInput;

class Query : public ChatWindow
{
  Q_OBJECT

  public:
    Query(QWidget* parent);
    ~Query();

    void setName(const QString& newName);
    void setHostmask(const QString& newHostmask);
    void updateFonts();
    void closeYourself();

  signals:
    void newText(QWidget* query);
    void closed(Query* query);
    void sendFile(const QString& recipient);

  public slots:
    void adjustFocus();
    void sendQueryText(const QString& text);

  protected slots:
    void queryTextEntered();
    void sendFileMenu();
    void newTextInView();
    void close();
    // connected to IRCInput::textPasted() - used to handle large/multiline pastes
    void textPasted(QString text);

  protected:
    QString queryName;
    QString hostmask;
    QString buffer;

    QLineEdit* queryHostmask;
    IRCInput* queryInput;
    QCheckBox* logCheckBox;
};

#endif
