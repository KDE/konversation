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
#include "nicklisttooltip.h"
#include "images.h"

/*
  @author Dario Abatianni
*/

class QPopupMenu;
class QContextMenuEvent;
class QTimer;

class NickListView : public KListView
{
  Q_OBJECT

  public:
    NickListView(QWidget* parent, Channel *chan);
    ~NickListView();

    void refresh();

    enum PopupIDs
    {
      ModesSub,GiveOp,TakeOp,GiveVoice,TakeVoice,
      KickBanSub,Ignore,
      Kick,KickBan,BanNick,BanHost,BanDomain,BanUserHost,BanUserDomain,
      KickBanHost,KickBanDomain,KickBanUserHost,KickBanUserDomain,
      Whois,Version,Ping,Query,DccSend,
      CustomID, AddressbookSub, AddressbookChange, AddressbookNew, AddressbookDelete,
      AddressbookEdit, SendEmail
    };

  public slots:
    /** When this is called, resort is guaranteed to be called within a hard-coded time (a few seconds).
     *  This prevents lots of calls to resort.
     */
    void startResortTimer();
  protected slots:
    /** Resort the listview.
     *  It is better to call startResortTimer() which will resort with a minimum of a 
     *  1 second delay.  If this isn't good enough, you can make the function public
     *  again and use this, but best not to.
     */
    void resort();
  signals:
    /* Will be connected to Channel::popupCommand(int) */
    void popupCommand(int id);

  protected:
    void contextMenuEvent(QContextMenuEvent* ce);
    void insertAssociationSubMenu();
    Konversation::KonversationNickListViewToolTip *m_tooltip;
    QPopupMenu* popup;
    QPopupMenu* modes;
    QPopupMenu* kickban;
    QPopupMenu* addressbook;
    Channel *channel;
    QTimer *m_resortTimer;
};

#endif
