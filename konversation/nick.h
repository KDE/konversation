/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nick.h  -  description
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
class LedListViewItem;

class Nick
{
  public:
#ifdef USE_NICKINFO
    Nick(KListView *listView,
	 ChannelNickPtr channelnick);
#else
    Nick(KListView* listView,
         const QString &nickname,
         const QString &hostmask,
         bool admin,
         bool owner,
         bool op,
         bool halfop,
         bool voice);
#endif
    ~Nick();

    bool isAdmin();
    bool isOwner();
    bool isOp();
    bool isHalfop();
    bool hasVoice();

    bool isSelected();

    QString getNickname();
    QString getHostmask();
#ifdef USE_NICKINFO
    NickInfoPtr getNickInfo();
    ChannelNickPtr getChannelNick();
#else
    void setHostmask(const QString& newMask);
    void setNickname(const QString& newName);
    void setAdmin(bool state);
    void setOwner(bool state);
    void setOp(bool state);
    void setHalfop(bool state);
    void setVoice(bool state);    

#endif
    
  protected:
#ifdef USE_NICKINFO
    ChannelNickPtr channelnickptr;
#else
    bool admin;
    bool owner;
    bool op;
    bool halfop;
    bool voice;

    QString nickname;
    QString hostmask;   
#endif
    LedListViewItem* listViewItem;
};

#endif
