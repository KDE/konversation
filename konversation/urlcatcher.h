/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  urlcatcher.h  -  shows all URLs found by the client
  begin:     Die Mai 27 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef URLCATCHER_H
#define URLCATCHER_H

#include "chatwindow.h"

/*
  @author Dario Abatianni
*/

class KListView;
class QListViewItem;
class QPushButton;

class UrlCatcher : public ChatWindow
{
  Q_OBJECT

  public:
    UrlCatcher(QWidget* parent);
    ~UrlCatcher();

  signals:
    void deleteUrl(const QString& who,const QString& url);
    void clearUrlList();

  public slots:
    virtual void adjustFocus();
    void addUrl(const QString& who,const QString& url);

  protected slots:
    void urlSelected();
    void openUrl(QListViewItem* item);

    void openUrlClicked();
    void deleteUrlClicked();
    void saveListClicked();
    void clearListClicked();

  protected:
    KListView* urlListView;

    QPushButton* openUrlButton;
    QPushButton* deleteUrlButton;
    QPushButton* saveListButton;
    QPushButton* clearListButton;
};

#endif
