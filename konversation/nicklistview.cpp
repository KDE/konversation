/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This is the class that shows the channel nick list
  begin:     Fre Jun 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kpopupmenu.h>
#include <klocale.h>
#include <kdebug.h>
#include <qtooltip.h>
#include <qtimer.h>
#include <kiconloader.h>
#include <qwhatsthis.h>

#include "images.h"

#include "konversationapplication.h"
#include "nicklistview.h"
#include "nicklistviewitem.h"
#include "linkaddressbook/addressbook.h"

NickListView::NickListView(QWidget* parent, Channel *chan) :
              KListView(parent)
{
  setWhatsThis(); 
  channel=chan;
  popup=new KPopupMenu(this,"nicklist_context_menu");
  modes=new KPopupMenu(this,"nicklist_modes_context_submenu");
  kickban=new KPopupMenu(this,"nicklist_kick_ban_context_submenu");
  addressbook= new KPopupMenu(this,"nicklist_addressbook_context_submenu");

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
    int newitem;
    newitem = popup->insertItem(i18n("Open Query"),Query);
    popup->setWhatsThis(newitem, "<qt>Start a private chat between you and this person.<p/><em>Technical note:</em><br>The conversation between you and this person will be sent via the server.  This means that the conversation will be affected by server lag, server stability, and will be terminated when you disconnect from the server.</qt>");
    newitem = popup->insertItem(i18n("Open DCC Chat"),DccChat);
    popup->setWhatsThis(newitem, "<qt>Start a private <em>D</em>irect <em>C</em>lient <em>C</em>onnection chat between you and this person.<p/><em>Technical note:</em><br />The conversation between you and this person will be sent directly.  This means it is independent from the server - so if the server connection fails, or use disconnect, your DCC Chat will be unaffected.  It also means that no irc server admin can view or spy on this chat.</qt>");
    newitem = popup->insertItem(SmallIcon("2rightarrow"),i18n("Send File..."),DccSend);
    popup->setWhatsThis(newitem, "<qt>Send a file to this person.  If you are having problem sending files, or they are sending slowly, see the Konversation Handbook and DCC preferences page.</qt>");
    popup->insertItem(SmallIconSet("mail_generic"),i18n("Send Email..."), SendEmail);
    if(addressbook) {
      popup->insertSeparator();
      popup->insertItem(i18n("Addressbook Associations"), addressbook, AddressbookSub);
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

    connect (popup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    connect (modes, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    connect (kickban, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    connect (addressbook, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));

  }
  else
  {
    kdWarning() << "NickListView::NickListView(): Could not create popup!" << endl;
  }

  // We have our own tooltips, don't use the default QListView ones
  setShowToolTips(false);
  m_tooltip = new Konversation::KonversationNickListViewToolTip(viewport(), this);

  m_resortTimer = new QTimer( this);
  connect(m_resortTimer, SIGNAL( timeout()), SLOT(resort()));
}

NickListView::~NickListView()
{
}

void NickListView::setWhatsThis()
{
  Images* images = KonversationApplication::instance()->images();
  
  QMimeSourceFactory::defaultFactory()->setImage( "admin", images->getNickIcon( Images::Admin, false ).convertToImage() );
  QMimeSourceFactory::defaultFactory()->setImage( "owner", images->getNickIcon( Images::Owner, false ).convertToImage());
  QMimeSourceFactory::defaultFactory()->setImage( "op", images->getNickIcon( Images::Op, false ).convertToImage() );
  QMimeSourceFactory::defaultFactory()->setImage( "halfop", images->getNickIcon( Images::HalfOp, false ).convertToImage() );
  QMimeSourceFactory::defaultFactory()->setImage( "voice", images->getNickIcon( Images::Voice, false ).convertToImage() );
  QMimeSourceFactory::defaultFactory()->setImage( "normal", images->getNickIcon( Images::Normal, false ).convertToImage() );
  QMimeSourceFactory::defaultFactory()->setImage( "normalaway", images->getNickIcon( Images::Normal, true).convertToImage() );

  if(images->getNickIcon( Images::Normal, false).isNull()) {
    QWhatsThis::add(this, i18n("<qt>This shows all the people in the channel.  The nick for each person is shown.<br>Usually an icon is shown showing the status of each person, but you do not seem to have any icon theme installed.  See the Konversation settings - <i>Configure Konversation</i> under the <i>Settings</i> menu.  Then view the page for <i>Themes</i> under <i>Appearence</i>.</qt>"));
  } else {
	  
    QWhatsThis::add(this, i18n("<qt>This shows all the people in the channel.  The nick for each person is shown, with a picture showing their status.<p>"
			  "<table>"
			  
			  "<tr><th><img src=\"admin\"></th><td>This person has administrator privileges.</td></tr>"
			  "<tr><th><img src=\"owner\"></th><td>This person is a channel owner.</td></tr>"
			  "<tr><th><img src=\"op\"></th><td>This person is a channel operator.</td></tr>"
			  "<tr><th><img src=\"halfop\"></th><td>This person is a channel half-operator.</td></tr>" 
			  "<tr><th><img src=\"voice\"></th><td>This person has voice, and can therefore talk in a moderated channel.</td></tr>"
			  "<tr><th><img src=\"normal\"></th><td>This person does not have any special privileges.</td></tr>"
			  "<tr><th><img src=\"normalaway\"></th><td>This indicates that this person is currently away.</td></tr>"
			  "</table><p>"
			  "The meaning of admin, owner and halfop varies between different irc servers.<p>"
			  "Hovering over any nick shows their current status, as well as any information in the addressbook for this person.  See the Konversation Handbook for more information."
			  "</qt>"
			  ));
  }

}

void NickListView::refresh()
{
  QPtrList<QListViewItem> nicklist;
  QListViewItemIterator it(this);

  while (it.current()) 
  {
    static_cast<NickListViewItem*>(it.current())->refresh();
    ++it;
  }
  setWhatsThis();
}

void NickListView::startResortTimer() {
  if(!m_resortTimer->isActive());
    m_resortTimer->start(3000, TRUE /*single shot*/); 
}

void NickListView::resort()
{
  sort();
}

void NickListView::contextMenuEvent(QContextMenuEvent* ce)
{
  ce->accept();

  if(selectedItems().count())
  {
    insertAssociationSubMenu();
    popup->popup(ce->globalPos());
  }
}

void NickListView::insertAssociationSubMenu() {

  bool existingAssociation = false;
  bool noAssociation = false;
  bool emailAddress = false;

  addressbook->clear();

  ChannelNickList nickList=channel->getSelectedChannelNicks();
  for(ChannelNickList::Iterator it=nickList.begin();it!=nickList.end();++it)
  {
    KABC::Addressee addr = (*it)->getNickInfo()->getAddressee();
    if(addr.isEmpty()) {
        noAssociation=true;
        if(existingAssociation && emailAddress) break;
    } else {
        if(!emailAddress && !addr.preferredEmail().isEmpty())
            emailAddress = true;
        existingAssociation=true;
        if(noAssociation && emailAddress) break;
    }
  }

  if(!noAssociation && existingAssociation) {
      addressbook->insertItem(SmallIcon("contents"), i18n("Edit Contact..."), AddressbookEdit);
      addressbook->insertSeparator();
  }

  if(noAssociation && existingAssociation)
      addressbook->insertItem(i18n("Choose/Change Associations..."), AddressbookChange);
  else if(noAssociation)
      addressbook->insertItem(i18n("Choose Contact..."), AddressbookChange);
  else
      addressbook->insertItem(i18n("Change Association..."), AddressbookChange);

  if(noAssociation && !existingAssociation)
      addressbook->insertItem(i18n("Create New Contact..."), AddressbookNew);

  if(existingAssociation)
      addressbook->insertItem(SmallIcon("editdelete"), i18n("Delete Association"), AddressbookDelete);

  if(!emailAddress)
      popup->setItemEnabled(SendEmail, false);
  else
      popup->setItemEnabled(SendEmail, true);

}
#include "nicklistview.moc"

