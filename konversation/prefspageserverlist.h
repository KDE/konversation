/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageserverlist.h  -  description
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qpushbutton.h>
#include <qframe.h>

#include <klistview.h>

#include "prefspage.h"
#include "preferences.h"

#ifndef PREFSPAGESERVERLIST_H
#define PREFSPAGESERVERLIST_H

/*
  @author Dario Abatianni
*/

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
    void updateServer(const QString&,const QString&,const QString&,const QString&,const QString&,const QString&);
    void updateServerProperty(QListViewItem*,const QString&,int);
    void connectClicked();
    void serverDoubleClicked(QListViewItem* item);

  protected:
    QPushButton* connectButton;
    QPushButton* newServerButton;
    QPushButton* editServerButton;
    QPushButton* removeServerButton;

    KListView* serverListView;

		int itemId;
};

#endif
