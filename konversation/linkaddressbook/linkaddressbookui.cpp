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
// used for its AddresseeItem class
#include <kabc/addresseedialog.h>
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

#include "linkaddressbookui.h"

LinkAddressbookUI::LinkAddressbookUI( QWidget *parent, const char *name, const QString &ircnick )
: LinkAddressbookUI_Base( parent, name )
{
	m_addressBook = KABC::StdAddressBook::self( true );
	KABC::StdAddressBook::setAutomaticSave( false );
	
	// Addressee validation connections
	connect( addAddresseeButton, SIGNAL( clicked() ), SLOT( slotAddAddresseeClicked() ) );
	connect( addresseeListView, SIGNAL( clicked(QListViewItem * ) ),
			SLOT( slotAddresseeListClicked( QListViewItem * ) ) );
	connect( addresseeListView, SIGNAL( selectionChanged( QListViewItem * ) ),
			SLOT( slotAddresseeListClicked( QListViewItem * ) ) );
	connect( addresseeListView, SIGNAL( spacePressed( QListViewItem * ) ),
			SLOT( slotAddresseeListClicked( QListViewItem * ) ) );

	connect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
	slotLoadAddressees();
	m_ircnick = ircnick;
	Q_ASSERT(!ircnick.isEmpty());
	
}


LinkAddressbookUI::~LinkAddressbookUI()
{
}

void LinkAddressbookUI::slotLoadAddressees()
{
	addresseeListView->clear();
	KABC::AddressBook::Iterator it;
	for( it = m_addressBook->begin(); it != m_addressBook->end(); ++it )
		/*KABC::AddresseeItem *item =*/ new KABC::AddresseeItem( addresseeListView, (*it) );
}

void LinkAddressbookUI::slotAddAddresseeClicked()
{
	// Pop up add addressee dialog
	QString addresseeName = KInputDialog::getText( i18n( "New Address Book Entry" ),
												   i18n( "Name the new entry:" ),
												   m_ircnick, 0, this );

	if ( !addresseeName.isEmpty() )
	{
		KABC::Addressee addr;
		addr.setNameFromString( addresseeName );
		m_addressBook->insertAddressee( addr );
		KABC::Ticket *ticket = m_addressBook->requestSaveTicket();
		if ( !ticket )
		{
			kdError() << "Resource is locked by other application!" << endl;
		}
		else
		{
			if ( !m_addressBook->save( ticket ) )
			{
				kdError() << "Saving failed!" << endl;
#if KDE_IS_VERSION (3,1,90)
				m_addressBook->releaseSaveTicket( ticket );
#endif
			}
		}
	}
//this shouldn't be needed - why is it?
	slotLoadAddressees();
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
	if ( item ) {
		KABC::Addressee addressee(item->addressee());
		addressee.insertCustom("messaging/irc", "All", m_ircnick);
	    m_addressBook->insertAddressee(addressee);
		
		KABC::Ticket *ticket = m_addressBook->requestSaveTicket();
        if ( !ticket )
        {
			//TODO:  tell the user
            kdError() << "Resource is locked by other application!" << endl;
	    }
	    else
	    {
	        if ( !m_addressBook->save( ticket ) )
	        {
		        kdError() << "Saving failed!" << endl;
#if KDE_IS_VERSION (3,1,90)
			    m_addressBook->releaseSaveTicket( ticket );
#endif
			}
        }
    } else {
		//??
	}
	
    disconnect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
	deleteLater();
	
}

void LinkAddressbookUI::reject()
{
	disconnect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
	deleteLater();
}

#include "linkaddressbookui.moc"

// vim: set noet ts=4 sts=4 sw=4:

