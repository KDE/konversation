/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverlistitem.cpp  -  description
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <kdebug.h>

#include "serverlistitem.h"

ServerListItem::ServerListItem(QListView* parent,int newId,
                               QString arg0,
                               QString arg1,
                               QString arg2,
                               QString arg3,
                               QString arg4,
                               QString arg5) :
                 KListViewItem(parent,arg0,arg1,arg2,arg3,arg4,arg5)
{
  id=newId;
}

ServerListItem::~ServerListItem()
{
  kdDebug() << "ServerListItem::~ServerListItem(" << text(0) << ")" << endl;
}
