/*
    linkaddressbookui.cpp - Kontact's Link IRC Nick to Addressbook contact Wizard

   This code was shamelessly stolen from kopete's add new contact wizard.

    Copyright (c) 2004 by John Tapsell           <john@geola.co.uk>

    Copyright (c) 2003 by Will Stephenson        <will@stevello.free-online.co.uk>
    Copyright (c) 2002 by Nick Betcher           <nbetcher@kde.org>
    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#include <qcheckbox.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kiconloader.h>

#include <kdeversion.h>
#include <kinputdialog.h>
#include <kinputdialog.h>

#include <kpushbutton.h>
#include <kdebug.h>
#include <klistview.h>
#include <qlabel.h>
// used for its AddresseeItem class
#include <kabc/addresseedialog.h>
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

#include "linkaddressbookui.h"
#include "addressbook.h"
#include <kapplication.h>

LinkAddressbookUI::LinkAddressbookUI( QWidget *parent, const char *name, const QString &ircnick, const QString &suggested_realname )
: LinkAddressbookUI_Base( parent, name )
{

	m_addressBook = Konversation::Addressbook::self()->getAddressBook();

	// Addressee validation connections
	connect( addAddresseeButton, SIGNAL( clicked() ), SLOT( slotAddAddresseeClicked() ) );
	connect( addresseeListView, SIGNAL( clicked(QListViewItem * ) ),
			SLOT( slotAddresseeListClicked( QListViewItem * ) ) );
	connect( addresseeListView, SIGNAL( selectionChanged( QListViewItem * ) ),
			SLOT( slotAddresseeListClicked( QListViewItem * ) ) );
	connect( addresseeListView, SIGNAL( spacePressed( QListViewItem * ) ),
			SLOT( slotAddresseeListClicked( QListViewItem * ) ) );

	connect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
    connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(slotLoadAddressees()));

	m_ircnick = ircnick;
	m_lower_ircnick = m_ircnick.lower();
	m_suggested_realname = suggested_realname;
	if(m_suggested_realname.isEmpty()) m_suggested_realname = suggested_realname;
	Q_ASSERT(!ircnick.isEmpty());
	slotLoadAddressees();
}


LinkAddressbookUI::~LinkAddressbookUI()
{
}

/**  Read in contacts from addressbook, and select the contact that is for our nick. */
void LinkAddressbookUI::slotLoadAddressees()
{
	addresseeListView->clear();

	QString realname;
	int num_contacts_with_nick=0;  //There shouldn't be more than 1 contact with this irc nick.  Warn the user if there is.

	KABC::AddressBook::Iterator it;
	for( it = m_addressBook->begin(); it != m_addressBook->end(); ++it )
		if(Konversation::Addressbook::self()->hasNick(*it, m_lower_ircnick)) {
			realname = (*it).realName();
			num_contacts_with_nick++;
			(new KABC::AddresseeItem( addresseeListView, (*it) ))->setSelected(true);
		} else
			/*KABC::AddresseeItem *item =*/ new KABC::AddresseeItem( addresseeListView, (*it) );
	if(num_contacts_with_nick == 0)
		lblHeader->setText(i18n("<qt><h1>Select Addressbook Entry</h1><p>From the list of contacts below, choose the person who '%2' is.</p></qt>").arg(m_ircnick));
	else if(num_contacts_with_nick == 1 && realname.isEmpty())
		lblHeader->setText(i18n("<qt><h1>Select Addressbook Entry</h1><p>'%2' is currently being listed as being a contact with no given name.</p></qt>").arg(m_ircnick));
	else if(num_contacts_with_nick == 1 && !realname.isEmpty())
		lblHeader->setText(i18n("<qt><h1>Select Addressbook Entry</h1><p>'%2' is currently being listed as belonging to the contact '%3'.</p></qt>").arg(m_ircnick).arg(realname));
	else
		lblHeader->setText(i18n("<qt><h1>Select Addressbook Entry</h1><p><b>Warning:</b> '%2' is currently being listed as belonging to multiple contacts.  Please select the correct contact.</p></qt>").arg(m_ircnick));
}

void LinkAddressbookUI::slotAddAddresseeClicked()
{
	// Pop up add addressee dialog
	if(!Konversation::Addressbook::self()->getAndCheckTicket()) return;
	QString addresseeName = KInputDialog::getText( i18n( "New Address Book Entry" ),
												   i18n( "Name the new entry:" ),
												   m_suggested_realname, 0, this );

	if ( !addresseeName.isEmpty() )
	{
		KABC::Addressee addr;
		addr.setNameFromString( addresseeName );
		m_addressBook->insertAddressee(addr);
		Konversation::Addressbook::self()->saveTicket();
		slotLoadAddressees();
	} else {
		Konversation::Addressbook::self()->releaseTicket();
	}
}

void LinkAddressbookUI::slotAddresseeListClicked( QListViewItem *addressee )
{
	// enable ok if a valid addressee is selected
	buttonOk->setEnabled(addressee ? addressee->isSelected() : false);
}

void LinkAddressbookUI::accept()
{
	//// set the KABC uid in the metacontact
	KABC::AddresseeItem *item = 0L;
	item = static_cast<KABC::AddresseeItem *>( addresseeListView->selectedItem() );

	KABC::Addressee addr;
	if ( item ) {

	    addr = item->addressee();
		if(!Konversation::Addressbook::self()->getAndCheckTicket()) {
			return;
		}
		Konversation::Addressbook::self()->associateNickAndUnassociateFromEveryoneElse(addr, m_ircnick);
		if(!Konversation::Addressbook::self()->saveTicket()) {
			return;
		}

	}
    disconnect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
	deleteLater();

}

void LinkAddressbookUI::reject()
{
	disconnect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
	deleteLater();
}

void LinkAddressbookUI::buttonHelp_clicked()
{
	kapp->invokeHelp("linkaddressbook");
}


#include "linkaddressbookui.moc"

// vim: set noet ts=4 sts=4 sw=4:

