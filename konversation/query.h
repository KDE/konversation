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
*/

#include <qobject.h>
#include <qvbox.h>
#include <qiconset.h>

#include <kdialog.h>

#ifndef QUERY_H
#define QUERY_H

#include "server.h"
#include "chatwindow.h"
#include "ircinput.h"

/*
  @author Dario Abatianni
*/

/* TODO: Idle counter to close query after XXX minutes of inactivity */

class Server;

class Query : public ChatWindow
{
  Q_OBJECT

  public:
    Query(QWidget* parent);
    ~Query();

    QWidget* getQueryPane() { return queryPane; };
    QString& getQueryName() { return queryName; };

    void setQueryName(const QString& newName);
    void setHostmask(const QString& newHostmask);

  signals:
    void newText(QWidget* query);
    void closed(Query* query);

  protected slots:
    void queryTextEntered();
    void newTextInView();
    void close();

  protected:
    int spacing() {  return KDialog::spacingHint(); };
    int margin() {  return KDialog::marginHint(); };

    QString queryName;
    QString hostmask;
    QString buffer;
    QVBox* queryPane;
    QLineEdit* queryHostmask;
    IRCInput* queryInput;
    QCheckBox* logCheckBox;
};

#endif
