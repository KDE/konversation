/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageserverlist.h  -  Provides a user interface to edit and select servers
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGESERVERLIST_H
#define PREFSPAGESERVERLIST_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class QListViewItem;
class QPushButton;
class QCheckBox;

class KListView;

class ServerListItem;

class PrefsPageServerList : public PrefsPage
{
  Q_OBJECT

  public: 
    PrefsPageServerList(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageServerList();

  signals:
    void connectToServer(int id);
    
  protected slots:
    void newServer();
    void editServer();
    void removeServer();
    void serverSelected(QListViewItem* item);
    void updateServer(const QString&,const QString&,const QString&,const QString&,const QString&,const QString&,const QString&);
    void updateServerProperty(QListViewItem*,const QString&,int);
    void updateAutoState(ServerListItem* item,bool state);
    void connectClicked();
    void serverDoubleClicked(QListViewItem* item);
    void showServerListChanged(int state);

  protected:
    QPushButton* connectButton;
    QPushButton* newServerButton;
    QPushButton* editServerButton;
    QPushButton* removeServerButton;

    KListView* serverListView;
    QCheckBox* showServerList;

    int itemId;
};

#endif
