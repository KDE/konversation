/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicksonline.h  -  description
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
    NicksOnline(QSize& newSize);
    ~NicksOnline();

  signals:
    void editClicked();
    void doubleClicked(QListViewItem* item);
    void closeClicked(QSize size);

  public slots:
    void setOnlineList(QStringList list);
    void closeButton();
    
  protected:
    void closeEvent(QCloseEvent* ce);

  protected slots:
    void processDoubleClick(QListViewItem* item);

    KListView* nickListView;
};

#endif
