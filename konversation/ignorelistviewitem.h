/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ignorelistviewitem.h  -  description
  begin:     Die Jun 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef IGNORELISTVIEWITEM_H
#define IGNORELISTVIEWITEM_H

/*
  @author Dario Abatianni
*/

#include <qstring.h>

#include <klistview.h>

class IgnoreListViewItem : public KListViewItem
{
  public:
    IgnoreListViewItem(QListView* parent,QString name,int flags);
    ~IgnoreListViewItem();

    void setFlag(int flag,bool active);
    bool getFlag(int flag) { return flags & flag; };
    int getFlags() { return flags; };
    IgnoreListViewItem* itemBelow();

  protected:
    void setFlags(int flags);
    QString yes;
    QString no;

    int flags;
};

#endif
