/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicklistview.cpp  -  This is the class that shows the channel nick list
  begin:     Fre Jun 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qpopupmenu.h>

#include <klocale.h>
#include <kdebug.h>

#include "nicklistview.h"

NickListView::NickListView(QWidget* parent) :
              KListView(parent)
{
  popup=new QPopupMenu(this,"nicklist_context_menu");
  modes=new QPopupMenu(this,"nicklist_modes_context_submenu");

  if(popup)
  {
    if(modes)
    {
      modes->insertItem(i18n("Give Op"),GiveOp);
      modes->insertItem(i18n("Take Op"),TakeOp);
      modes->insertItem(i18n("Give Voice"),GiveVoice);
      modes->insertItem(i18n("Take Voice"),TakeVoice);
      popup->insertItem(i18n("Modes"),modes,Modes);
    }
    else
    {
      kdWarning() << "NickListView::NickListView(): Could not create modes popup!" << endl;
    }
    popup->insertSeparator();
    popup->insertItem(i18n("Whois"),Whois);
    popup->insertItem(i18n("Version"),Version);
    popup->insertItem(i18n("Ping"),Ping);
    popup->insertSeparator();
    popup->insertItem(i18n("Open Query"),Query);
    popup->insertItem(i18n("Send File"),DccSend);
    popup->insertSeparator();
    popup->insertItem(i18n("Kick"),Kick);
    popup->insertItem(i18n("Kickban"),KickBan);
    popup->insertItem(i18n("Ban Nickname"),BanNick);
    popup->insertItem(i18n("Ban Hostmask"),BanHostmask);
  }
  else
  {
    kdWarning() << "NickListView::NickListView(): Could not create popup!" << endl;
  }
}

NickListView::~NickListView()
{
  delete modes;
  delete popup;
}

void NickListView::contextMenuEvent(QContextMenuEvent* ce)
{
  ce->accept();

  if(selectedItems().count())
  {
    int r=popup->exec(ce->globalPos());
    // Will be connected to Channel::popupCommand(int)
    emit popupCommand(r);
  }
}

#include "nicklistview.moc"
