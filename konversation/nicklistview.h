/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicklistview.h  -  Channel Nick List, including context menu
  begin:     Fre Jun 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef NICKLISTVIEW_H
#define NICKLISTVIEW_H

#include <klistview.h>
#include "channel.h"
/*
  @author Dario Abatianni
*/

class QPopupMenu;
class QContextMenuEvent;

class NickListView : public KListView
{
  Q_OBJECT

  public:
    NickListView(QWidget* parent, Channel *chan);
    ~NickListView();

    enum PopupIDs
    {
      ModesSub,GiveOp,TakeOp,GiveVoice,TakeVoice,
      KickBanSub,Ignore,
      Kick,KickBan,BanNick,BanHost,BanDomain,BanUserHost,BanUserDomain,
      KickBanHost,KickBanDomain,KickBanUserHost,KickBanUserDomain,
      Whois,Version,Ping,Query,DccSend,
      CustomID, AddressbookSub, AddressbookChange, AddressbookNew, AddressbookDelete
    };

  signals:
    /* Will be connected to Channel::popupCommand(int) */
    void popupCommand(int id);

  protected:
    void contextMenuEvent(QContextMenuEvent* ce);
    void insertAssociationSubMenu();
	
    QPopupMenu* popup;
    QPopupMenu* modes;
    QPopupMenu* kickban;
    QPopupMenu* addressbook;
    Channel *channel;
};

#endif
