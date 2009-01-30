/*
   Kontact's Link IRC Nick to Addressbook contact Wizard

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

#include "linkaddressbookui.h"
#include "addressbook.h"
#include "linkaddressbookui_base.h"
#include "addresseeitem.h"

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3Frame>

#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <kinputdialog.h>
#include <kpushbutton.h>
#include <k3activelabel.h>
#include <kdebug.h>
#include <k3listview.h>
#include <k3listviewsearchline.h>
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>


LinkAddressbookUI::LinkAddressbookUI( QWidget *parent, const char *name, const QString &ircnick, const QString &servername, const QString &servergroup, const QString &suggested_realname )
: KDialog(parent)
{
    setCaption( i18n("Link IRC Nick to Addressbook Contact") );
    setButtons( KDialog::Ok|KDialog::Cancel|KDialog::Help );
    setDefaultButton( KDialog::Ok );

    QFrame* page = new QFrame(this);
    setMainWidget( page );
    Q3GridLayout* pageLayout = new Q3GridLayout(page, 1, 1, 0, 0);
    m_mainWidget = new LinkAddressbookUI_Base(page);
    pageLayout->addWidget(m_mainWidget, 0, 0);

    enableButtonOk(false);
    setHelp("linkaddressbook");
    m_addressBook = Konversation::Addressbook::self()->getAddressBook();

    // Addressee validation connections
    connect( m_mainWidget->addAddresseeButton, SIGNAL( clicked() ), SLOT( slotAddAddresseeClicked() ) );
    connect( m_mainWidget->addresseeListView, SIGNAL( clicked(Q3ListViewItem * ) ),
        SLOT( slotAddresseeListClicked( Q3ListViewItem * ) ) );
    connect( m_mainWidget->addresseeListView, SIGNAL( selectionChanged( Q3ListViewItem * ) ),
        SLOT( slotAddresseeListClicked( Q3ListViewItem * ) ) );
    connect( m_mainWidget->addresseeListView, SIGNAL( spacePressed( Q3ListViewItem * ) ),
        SLOT( slotAddresseeListClicked( Q3ListViewItem * ) ) );

    connect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
    connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(slotLoadAddressees()));

    //We should add a clear KAction here.  But we can't really do that with a designer file :\  this sucks

    m_ircnick = ircnick;
    m_lower_ircnick = m_ircnick.toLower();
    m_servername = servername;
    m_servergroup = servergroup;
    m_suggested_realname = suggested_realname;

    m_mainWidget->addresseeListView->setColumnText(2, SmallIconSet("email"), i18n("Email") );

    if(m_suggested_realname.isEmpty()) m_suggested_realname = suggested_realname;
    Q_ASSERT(!ircnick.isEmpty());
    m_mainWidget->kListViewSearchLine->setListView(m_mainWidget->addresseeListView);
    slotLoadAddressees();

    m_mainWidget->addresseeListView->setColumnWidthMode(0, Q3ListView::Manual);
                                                  //Photo is 60, and it's nice to have a small gap, imho
    m_mainWidget->addresseeListView->setColumnWidth(0, 63);
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
    connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancel() ) );
}

LinkAddressbookUI::~LinkAddressbookUI()
{
}

/**  Read in contacts from addressbook, and select the contact that is for our nick. */
void LinkAddressbookUI::slotLoadAddressees()
{
    m_mainWidget->addresseeListView->clear();

    QString realname;
    int num_contacts_with_nick=0;                 //There shouldn't be more than 1 contact with this irc nick.  Warn the user if there is.

    KABC::AddressBook::Iterator it;
    for( it = m_addressBook->begin(); it != m_addressBook->end(); ++it )
        if(Konversation::Addressbook::self()->hasNick(*it, m_lower_ircnick, m_servername, m_servergroup))
    {
        realname = (*it).realName();
        num_contacts_with_nick++;
        (new AddresseeItem( m_mainWidget->addresseeListView, (*it) ))->setSelected(true);
    } else
    /*AddresseeItem *item =*/ new AddresseeItem( m_mainWidget->addresseeListView, (*it));

    if(num_contacts_with_nick == 0)
        m_mainWidget->lblHeader->setText(i18n("Choose the person who '%1' is.",m_ircnick));
    else if(num_contacts_with_nick == 1 && realname.isEmpty())
        m_mainWidget->lblHeader->setText(i18n("Currently '%1' is associated with a contact.",m_ircnick));
    else if(num_contacts_with_nick == 1 && !realname.isEmpty())
        m_mainWidget->lblHeader->setText(i18n("Currently '%1' is associated with contact '%2'.",m_ircnick,realname));
    else
        m_mainWidget->lblHeader->setText(i18n("<qt><b>Warning:</b> '%1' is currently being listed as belonging to multiple contacts.  Please select the correct contact.</qt>",m_ircnick));

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
    }
    else
    {
        Konversation::Addressbook::self()->releaseTicket();
    }
}

void LinkAddressbookUI::slotAddresseeListClicked( Q3ListViewItem *addressee )
{
    // enable ok if a valid addressee is selected
    enableButtonOk(addressee ? addressee->isSelected() : false);
}

void LinkAddressbookUI::slotOk()
{
    //// set the KABC uid in the metacontact
    AddresseeItem *item = 0L;
    item = static_cast<AddresseeItem *>( m_mainWidget->addresseeListView->selectedItem() );

    KABC::Addressee addr;
    if ( item )
    {

        addr = item->addressee();
        if(!Konversation::Addressbook::self()->getAndCheckTicket())
        {
            return;
        }
        Konversation::Addressbook::self()->associateNickAndUnassociateFromEveryoneElse(addr, m_ircnick, m_servername, m_servergroup);
        if(!Konversation::Addressbook::self()->saveTicket())
        {
            return;
        }
    }
    disconnect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
    deleteLater();
    accept();
}

void LinkAddressbookUI::slotCancel()
{
    disconnect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
    deleteLater();
    reject();
}

#include "linkaddressbookui.moc"

// vim: set noet ts=4 sts=4 sw=4:
