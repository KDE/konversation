/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicksonline.h  -  shows a user tree of friends per server
  begin:     Sam Aug 31 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICKSONLINE_H
#define NICKSONLINE_H

#include <qvbox.h>

#include "chatwindow.h"

/*
  @author Dario Abatianni
*/

class KListView;

class NicksOnline : public ChatWindow
{
  Q_OBJECT

  public: 
    NicksOnline(QWidget* parent);
    ~NicksOnline();

  signals:
    void editClicked();
    void doubleClicked(const QString& server,const QString& nick);

  public slots:
    void setOnlineList(const QString& serverName,const QStringList& list,bool changed);
    virtual void adjustFocus();
    
  protected slots:
    void processDoubleClick(QListViewItem* item);
    void timerFired();

  protected:
    void updateServerOnlineList(Server* server, bool changed);
    void refreshAllServerOnlineLists();

    KListView* nickListView;
    QTimer* timer;
};

#endif
