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

  $Id$
*/

#ifndef NICKSONLINE_H
#define NICKSONLINE_H

#include <qvbox.h>

#include <klistview.h>

/*
  @author Dario Abatianni
*/

class NicksOnline : public QVBox
{
  Q_OBJECT

  public: 
    NicksOnline(const QSize& newSize);
    ~NicksOnline();

  signals:
    void editClicked();
    void doubleClicked(const QString& server,const QString& nick);
    void closeClicked(QSize size);

  public slots:
    void setOnlineList(const QString& serverName,const QStringList& list);
    void closeButton();
    
  protected slots:
    void processDoubleClick(QListViewItem* item);

  protected:
    void closeEvent(QCloseEvent* ce);

    KListView* nickListView;
};

#endif
