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

#include "chatwindow.h"
#include "ircinput.h"

/*
  @author Dario Abatianni
*/

/* TODO: Idle counter to close query after XXX minutes of inactivity */
/* TODO: Use /USERHOST to check if queries are still valid */
class Query : public ChatWindow
{
  Q_OBJECT

  public:
    Query(QWidget* parent);
    ~Query();

    void setName(const QString& newName);
    void setHostmask(const QString& newHostmask);
    void updateFonts();

  signals:
    void newText(QWidget* query);
    void closed(Query* query);
    void sendFile(QString recipient);

  public slots:
    void adjustFocus();

  protected slots:
    void sendFileMenu();
    void queryTextEntered();
    void newTextInView();
    void close();
    // connected to IRCInput::textPasted() - used to handle large/multiline pastes
    void textPasted(QString text);

  protected:
    void sendQueryText(QString line);

    QString queryName;
    QString hostmask;
    QString buffer;

    QLineEdit* queryHostmask;
    IRCInput* queryInput;
    QCheckBox* logCheckBox;
};

#endif
