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
#ifdef USE_MDI
    UrlCatcher(QString caption);
#else
    UrlCatcher(QWidget* parent);
#endif
    ~UrlCatcher();

  signals:
    void deleteUrl(const QString& who,const QString& url);
    void clearUrlList();

  public slots:
    void addUrl(const QString& who,const QString& url);

  protected slots:
    void urlSelected();
    void openUrl(QListViewItem* item);

    void openUrlClicked();
    void copyUrlClicked();
    void deleteUrlClicked();
    void saveListClicked();
    void clearListClicked();

  protected:
#ifdef USE_MDI
    virtual void closeYourself(ChatWindow*);
#endif
    KListView* urlListView;

    /** Called from ChatWindow adjustFocus */
    virtual void childAdjustFocus();
    QPushButton* openUrlButton;
    QPushButton* copyUrlButton;
    QPushButton* deleteUrlButton;
    QPushButton* saveListButton;
    QPushButton* clearListButton;
};

#endif
