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
*/

#include <qpopupmenu.h>

#include <klocale.h>
#include <kdebug.h>
#include <qtooltip.h>

#include "nicklistview.h"
#include "linkaddressbook/addressbook.h"

NickListView::NickListView(QWidget* parent, Channel *chan) :
              KListView(parent)
{
  channel=chan;
  popup=new QPopupMenu(this,"nicklist_context_menu");
  modes=new QPopupMenu(this,"nicklist_modes_context_submenu");
  kickban=new QPopupMenu(this,"nicklist_kick_ban_context_submenu");
  addressbook= new QPopupMenu(this,"nicklist_addressbook_context_submenu");
  
  if(popup)
  {
    if(modes)
    {
      modes->insertItem(i18n("Give Op"),GiveOp);
      modes->insertItem(i18n("Take Op"),TakeOp);
      modes->insertItem(i18n("Give Voice"),GiveVoice);
      modes->insertItem(i18n("Take Voice"),TakeVoice);
      popup->insertItem(i18n("Modes"),modes,ModesSub);
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
    popup->insertItem(i18n("Send File..."),DccSend);

    if(addressbook) {
      popup->insertSeparator();
      popup->insertItem(i18n("Addressbook associations"), addressbook, AddressbookSub);
    }
    popup->insertSeparator();
    if(kickban)
    {
      kickban->insertItem(i18n("Kick"),Kick);
      kickban->insertItem(i18n("Kickban"),KickBan);
      kickban->insertItem(i18n("Ban Nickname"),BanNick);
      kickban->insertSeparator();
      kickban->insertItem(i18n("Ban *!*@*.host"),BanHost);
      kickban->insertItem(i18n("Ban *!*@domain"),BanDomain);
      kickban->insertItem(i18n("Ban *!user@*.host"),BanUserHost);
      kickban->insertItem(i18n("Ban *!user@domain"),BanUserDomain);
      kickban->insertSeparator();
      kickban->insertItem(i18n("Kickban *!*@*.host"),KickBanHost);
      kickban->insertItem(i18n("Kickban *!*@domain"),KickBanDomain);
      kickban->insertItem(i18n("Kickban *!user@*.host"),KickBanUserHost);
      kickban->insertItem(i18n("Kickban *!user@domain"),KickBanUserDomain);
      popup->insertItem(i18n("Kick / Ban"),kickban,KickBanSub);
    }
    popup->insertItem(i18n("Ignore"),Ignore);
  }
  else
  {
    kdWarning() << "NickListView::NickListView(): Could not create popup!" << endl;
  }

  // We have our own tooltips, don't use the default QListView ones
  setShowToolTips(false);
  m_tooltip = new Konversation::KonversationNickListViewToolTip(viewport(), this);
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
    insertAssociationSubMenu();
    int r=popup->exec(ce->globalPos());
    // Will be connected to Channel::popupCommand(int)
    emit popupCommand(r);
  }
}
void NickListView::insertAssociationSubMenu() {
	
  bool any_existing_associations=false;
  bool any_not_having_associations=false;
  addressbook->clear();
  QStringList nickList=channel->getSelectedNicksList();
  for(QStringList::Iterator nickIterator=nickList.begin();nickIterator!=nickList.end();++nickIterator)
  {
    if(Konversation::Addressbook::self()->getKABCAddresseeFromNick(*nickIterator).isEmpty()) {
      any_not_having_associations=true;
      if(any_existing_associations) break;
    } else {
      any_existing_associations=true;
      if(any_not_having_associations) break;
    }
  }

  if(!any_not_having_associations && any_existing_associations) {
    addressbook->insertItem(i18n("Edit contact..."), AddressbookEdit);
    addressbook->insertSeparator();	  
  }
  
  if(any_not_having_associations && any_existing_associations)
    addressbook->insertItem(i18n("Chose/change associations..."), AddressbookChange);
  else if(any_not_having_associations)
    addressbook->insertItem(i18n("Chose contact..."), AddressbookChange);
  else
    addressbook->insertItem(i18n("Change association..."), AddressbookChange);
  if(any_not_having_associations && !any_existing_associations)
    addressbook->insertItem(i18n("Create new contact..."), AddressbookNew);
  
    if(any_existing_associations)
    addressbook->insertItem(i18n("Delete association"), AddressbookDelete);
}
#include "nicklistview.moc"
