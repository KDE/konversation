/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  channellistpanel.cpp  -  Shows the list of channels
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include "channellistpanel.h"

ChannelListPanel::ChannelListPanel(QWidget* parent) :
                  ChatWindow(parent)
{
  setType(ChatWindow::ChannelList);
}

ChannelListPanel::~ChannelListPanel()
{
}
