/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Fri Jan 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICK_H
#define NICK_H

/*
  @author Dario Abatianni
*/

#include "channelnick.h"

class KListView;
class NickListViewItem;

class Nick
{
  public:
    Nick(KListView *listView,
	 ChannelNickPtr channelnick);
    ~Nick();

    bool isAdmin();
    bool isOwner();
    bool isOp();
    bool isHalfop();
    bool hasVoice();

    bool isSelected();

    QString getNickname();
    QString getHostmask();
    NickInfoPtr getNickInfo();
    ChannelNickPtr getChannelNick();
    
  protected:
    ChannelNickPtr channelnickptr;
    NickListViewItem* listViewItem;
};

#endif
