/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  channellistpanel.h  -  Shows the list of channels
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef _CHANNELLISTPANEL_H_
#define _CHANNELLISTPANEL_H_

#include "klistview.h"

#include <chatwindow.h>

/*
  Dario Abatianni
*/

class ChannelListPanel : public ChatWindow
{
  Q_OBJECT
  
  public:
    ChannelListPanel(QWidget* parent);
    ~ChannelListPanel();

  public slots:
    void adjustFocus();
    
  protected:
    KListView* channelListView;
};

#endif
