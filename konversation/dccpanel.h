/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dccpanel.h  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef DCCPANEL_H
#define DCCPANEL_H

#include <klistview.h>

#include "chatwindow.h"

/*
  @author Dario Abatianni
*/

class DccPanel : public ChatWindow
{
  public:
    DccPanel(QWidget* parent);
    ~DccPanel();

  protected:
    KListView* dccListView;

    int spacing();
    int margin();

};

#endif
