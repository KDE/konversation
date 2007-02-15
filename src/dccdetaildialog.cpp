// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>

#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <kurlrequester.h>

#include "dccdetaildialog.h"
#include "dcctransfer.h"
#include "dcctransferpanelitem.h"
#include "dcctransferrecv.h"

DccDetailDialog::DccDetailDialog( DccTransferPanelItem* item )
    : KDialog( 0 )
    ,m_item( item )
    ,m_transfer( item->transfer() )
{
    QVBoxLayout* baseLayout = new QVBoxLayout( this );
    baseLayout->setMargin( marginHint() );
    baseLayout->setSpacing( spacingHint() );

    // information

    QFrame* infoFrame = new QFrame( this );
    QGridLayout* infoLayout = new QGridLayout( infoFrame, 1, 2 );
    infoLayout->setSpacing( spacingHint() );

    // Filename
    QLabel* fileNameHeader = new QLabel( i18n("File:"), infoFrame );
    fileNameHeader->setAlignment( AlignHCenter | AlignVCenter );
    KLineEdit* fileName = new KLineEdit( m_transfer->getFileName(), infoFrame );
    fileName->setFocusPolicy( ClickFocus );
    fileName->setReadOnly( true );
    fileName->setFrame( false );
    fileName->setAlignment( AlignHCenter );

    // Local File URL
    QLabel* localFileURLHeader = new QLabel( infoFrame );
    localFileURLHeader->setAlignment( AlignHCenter | AlignVCenter );
    if ( m_transfer->getType() == DccTransfer::Send )
        localFileURLHeader->setText( i18n("Local Path:") );
    else
        localFileURLHeader->setText( i18n("Saved to:") );
    QHBox* localFileURLBox = new QHBox( infoFrame );
    localFileURLBox->setSpacing( spacingHint() );
    m_localFileURL = new KURLRequester( m_transfer->getFileURL().prettyURL(), localFileURLBox );
    connect( m_localFileURL, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotLocalFileURLChanged( const QString& ) ) );
    m_buttonOpenFile = new KPushButton( KGlobal::iconLoader()->loadIconSet( "exec", KIcon::Small ), QString::null, localFileURLBox );
    m_buttonOpenFile->setFixedSize( m_localFileURL->button()->size() );
    connect( m_buttonOpenFile, SIGNAL( clicked() ), this, SLOT( slotOpenFile() ) );
    m_buttonRemoveFile = new KPushButton( KGlobal::iconLoader()->loadIconSet( "edittrash", KIcon::Small ), QString::null, localFileURLBox );
    m_buttonRemoveFile->setFixedSize( m_localFileURL->button()->size() );
    connect( m_buttonRemoveFile, SIGNAL( clicked() ), this, SLOT( slotRemoveFile() ) );
    
    //QToolTip::add( m_localFileURL, i18n( "Change the download destination" ) );
    QToolTip::add( m_buttonOpenFile,  i18n( "Open this file" ) );
    QToolTip::add( m_buttonRemoveFile,  i18n( "Delete this file" ) );
    
    // Partner
    QLabel* partnerHeader = new QLabel( infoFrame );
    partnerHeader->setAlignment( AlignHCenter | AlignVCenter );
    if ( m_transfer->getType() == DccTransfer::Send )
        partnerHeader->setText( i18n("Receiver:") );
    else
        partnerHeader->setText( i18n("Sender:") );
    m_partner = new KLineEdit( infoFrame );
    m_partner->setFocusPolicy( ClickFocus );
    m_partner->setReadOnly( true );
    m_partner->setFrame( false );
    m_partner->setAlignment( AlignHCenter );

    // Self
    QLabel* selfHeader = 0;
    m_self = 0;
    if ( m_transfer->getType() == DccTransfer::Send )
    {
        selfHeader = new QLabel( i18n("Self:"), infoFrame );
        selfHeader->setAlignment( AlignHCenter | AlignVCenter );
        m_self = new KLineEdit( infoFrame );
        m_self->setFocusPolicy( ClickFocus );
        m_self->setReadOnly( true );
        m_self->setFrame( false );
        m_self->setAlignment( AlignHCenter );
    }

    // Status
    QLabel* statusHeader = new QLabel( i18n("Status:"), infoFrame );
    statusHeader->setAlignment( AlignHCenter | AlignVCenter );
    m_status = new KLineEdit( infoFrame );
    m_status->setFocusPolicy( ClickFocus );
    m_status->setReadOnly( true );
    m_status->setFrame( false );
    m_status->setAlignment( AlignHCenter );

    // Progres
    QLabel* progressHeader = new QLabel( i18n("Progress:"), infoFrame );
    progressHeader->setAlignment( AlignHCenter | AlignVCenter );
    m_progress = new KProgress( 100, infoFrame );
    m_progress->setCenterIndicator( true );

    // Position
    QLabel* positionHeader = new QLabel( i18n("Position:"), infoFrame );
    positionHeader->setAlignment( AlignHCenter | AlignVCenter );
    m_position = new KLineEdit( infoFrame );
    m_position->setFocusPolicy( ClickFocus );
    m_position->setReadOnly( true );
    m_position->setFrame( false );
    m_position->setAlignment( AlignHCenter );

    // buttons

    QFrame* buttonFrame = new QFrame( this );
    QHBoxLayout* buttonLayout = new QHBoxLayout( buttonFrame );
    buttonLayout->setSpacing( spacingHint() );

    // Accept
    m_buttonAccept = 0;
    if ( m_transfer->getType() == DccTransfer::Receive )
    {
        m_buttonAccept = new KPushButton( KGlobal::iconLoader()->loadIconSet( "player_play", KIcon::Small ), i18n("&Accept"), buttonFrame );
        m_buttonAccept->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
        connect( m_buttonAccept, SIGNAL( clicked() ), this, SLOT( slotAccept() ) );
        QToolTip::add( m_buttonAccept, i18n( "Accept this transfer" ) );
    }

    // Abort
    m_buttonAbort = new KPushButton( KGlobal::iconLoader()->loadIconSet( "stop", KIcon::Small ), i18n("A&bort"), buttonFrame );
    m_buttonAbort->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    connect( m_buttonAbort, SIGNAL( clicked() ), this, SLOT( slotAbort() ) );
    QToolTip::add( m_buttonAbort, i18n( "Abort this transfer" ) );

    // Close
    KPushButton* buttonClose = new KPushButton( KGlobal::iconLoader()->loadIconSet( "button_ok", KIcon::Small ), i18n("&Close"), buttonFrame );
    buttonClose->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    connect( buttonClose, SIGNAL( clicked() ), this, SLOT( slotClose() ) );

    // layout

    // construct layout: base
    baseLayout->addWidget( infoFrame );
    baseLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding ) );
    baseLayout->addWidget( buttonFrame );

    // construct layout: info
    int row = 0;

    infoLayout->addWidget( fileNameHeader, row, 0 );
    infoLayout->addWidget( fileName, row, 1 );

    ++row;
    infoLayout->addWidget( localFileURLHeader, row, 0 );
    infoLayout->addWidget( localFileURLBox, row, 1 );

    ++row;
    infoLayout->addWidget( partnerHeader, row, 0 );
    infoLayout->addWidget( m_partner, row, 1 );

    if ( m_self )
    {
        ++row;
        infoLayout->addWidget( selfHeader, row, 0 );
        infoLayout->addWidget( m_self, row, 1 );
    }

    ++row;
    infoLayout->addWidget( statusHeader, row, 0 );
    infoLayout->addWidget( m_status, row, 1 );

    ++row;
    infoLayout->addWidget( progressHeader, row, 0 );
    infoLayout->addWidget( m_progress, row, 1 );

    ++row;
    infoLayout->addWidget( positionHeader, row, 0 );
    infoLayout->addWidget( m_position, row, 1 );

    // construct layout: buttons
    if ( m_buttonAccept )
        buttonLayout->addWidget( m_buttonAccept );
    buttonLayout->addWidget( m_buttonAbort );
    buttonLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding ) );
    buttonLayout->addWidget( buttonClose );

    // update
    updateView();

    // make up
    adjustSize();
    resize( 450, height() );
    buttonClose->setFocus();
}

DccDetailDialog::~DccDetailDialog()
{
}

void DccDetailDialog::updateView() // public
{
    // caption
    if ( m_transfer->getType() == DccTransfer::Send )
        setCaption( i18n( "DCC Send: %1" ).arg( m_transfer->getFileName() ) );
    else
        setCaption( i18n( "DCC Receive: %1" ).arg( m_transfer->getFileName() ) );

    // information

    // Local path
    m_localFileURL->setURL( m_transfer->getFileURL().prettyURL() );
    m_localFileURL->lineEdit()->setFocusPolicy( m_transfer->getStatus() == DccTransfer::Queued ? StrongFocus : ClickFocus );
    m_localFileURL->lineEdit()->setReadOnly( m_transfer->getStatus() != DccTransfer::Queued );
    m_localFileURL->lineEdit()->setFrame( m_transfer->getStatus() == DccTransfer::Queued );
    m_localFileURL->lineEdit()->setAlignment( m_transfer->getStatus() == DccTransfer::Queued ? AlignLeft : AlignHCenter );
    m_localFileURL->button()->setEnabled( m_transfer->getStatus() == DccTransfer::Queued );
    m_buttonOpenFile->setEnabled( m_transfer->getType() == DccTransfer::Send || m_transfer->getStatus() == DccTransfer::Done );
    m_buttonRemoveFile->setEnabled( m_transfer->getType() == DccTransfer::Receive && m_transfer->getStatus() == DccTransfer::Done );

    // Partner
    if ( !m_transfer->getPartnerIp().isEmpty() || !m_transfer->getPartnerPort().isEmpty() )
        m_partner->setText( QString( "%1 (%2:%3)" )
            .arg( m_transfer->getPartnerNick() )
            .arg( !m_transfer->getPartnerIp().isEmpty() ? m_transfer->getPartnerIp() : i18n("unknown") )
        .arg( !m_transfer->getPartnerPort().isEmpty() ? m_transfer->getPartnerPort() : i18n("unknown") )
        );
    else
        m_partner->setText( m_transfer->getPartnerNick() );

    // Self
    if ( m_self )
    {
        QString self;
        if ( !m_transfer->getOwnIp().isEmpty() || !m_transfer->getOwnPort().isEmpty() )
            m_self->setText( QString( "%1:%2" )
                .arg( !m_transfer->getOwnIp().isEmpty() ? m_transfer->getOwnIp() : QString("* ") )
            .arg( !m_transfer->getOwnPort().isEmpty() ? m_transfer->getOwnPort() : i18n("unknown") )
            );
        else
            m_self->setText( i18n("unknown") );
    }

    // Status
    if ( m_transfer->getStatus() == DccTransfer::Sending || m_transfer->getStatus() == DccTransfer::Receiving )
        m_status->setText( m_item->getStatusText() + " ( " + m_item->getCurrentSpeedPrettyText() + " )" );
    else
        m_status->setText( m_transfer->getStatusDetail().isEmpty() ? m_item->getStatusText() : m_item->getStatusText() + " (" + m_transfer->getStatusDetail() + ')' );

    // Progress
    // FIXME: in case filesize is unknown
    m_progress->setProgress( m_transfer->getProgress() );

    // Position
    m_position->setText( m_item->getPositionPrettyText( true ) );

    // buttons

    // Accept
    if ( m_buttonAccept )
        m_buttonAccept->setEnabled( m_transfer->getStatus() == DccTransfer::Queued );

    // Abort
    m_buttonAbort->setEnabled( m_transfer->getStatus() < DccTransfer::Done );
}

void DccDetailDialog::slotLocalFileURLChanged( const QString& newURL )
{
    DccTransferRecv* transfer = static_cast<DccTransferRecv*>( m_transfer );
    if ( transfer )
        transfer->m_fileURL = KURL::fromPathOrURL( newURL );
}

void DccDetailDialog::slotOpenFile()
{
    m_item->runFile();
}

void DccDetailDialog::slotRemoveFile()
{
    int ret = KMessageBox::warningContinueCancel( this,
        i18n( "Do you really want to remove the received file?" ),
        i18n( "Delete Confirmation" ),
        i18n( "&Delete" ),
        "RemoveDCCReceivedFile",
        KMessageBox::Dangerous
        );
    if ( ret == KMessageBox::Continue )
        m_item->removeFile();
}

void DccDetailDialog::slotAccept()
{
    m_transfer->start();
}

void DccDetailDialog::slotAbort()
{
    m_transfer->abort();
}

void DccDetailDialog::slotClose()
{
    accept(); // *not* mean accepting DCC, but closing this dialog
}

#include "dccdetaildialog.moc"
