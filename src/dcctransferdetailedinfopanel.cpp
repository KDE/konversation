/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#include <qlabel.h>
#include <qtimer.h>

#include <klineedit.h>
#include <klocale.h>
#include <kprogress.h>
#include <kurlrequester.h>

#include "dcctransfer.h"
#include "dcctransferpanelitem.h"

#include "dcctransferdetailedinfopanel.h"

DccTransferDetailedInfoPanel::DccTransferDetailedInfoPanel( QWidget* parent, const char* name )
    : DccTransferDetailedInfoPanelUI( parent, name )
{
    m_autoViewUpdateTimer = new QTimer( this );

    connect( m_urlreqLocation, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotLocationChanged( const QString& ) ) );
}

DccTransferDetailedInfoPanel::~DccTransferDetailedInfoPanel()
{
}

void DccTransferDetailedInfoPanel::setItem( DccTransferPanelItem* item )
{
    m_autoViewUpdateTimer->stop();

    // disconnect all slots once
    disconnect( this );
    // we can't do disconnect( m_item->transfer(), 0, this, 0 ) here
    // because m_item can have been deleted already.

    // set up the auto view-udpate timer
    connect( m_autoViewUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateView() ) );

    m_item = item;
    connect( m_item->transfer(), SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotTransferStatusChanged( DccTransfer*, int, int ) ) );

    updateView();
}

void DccTransferDetailedInfoPanel::updateView()
{
    DccTransfer* transfer = m_item->transfer();

    // Type:
    m_labelDccType->setText( transfer->getType() == DccTransfer::Send ? i18n( "DCC Send" ) : i18n( "DCC Receive" ) );

    // Filename:
    m_labelFilename->setText( transfer->getFileName() );

    // Location:
    m_urlreqLocation->setURL( transfer->getFileURL().prettyURL() );
    //m_urlreqLocation->lineEdit()->setFocusPolicy( transfer->getStatus() == DccTransfer::Queued ? StrongFocus : ClickFocus );
    m_urlreqLocation->lineEdit()->setReadOnly( transfer->getStatus() != DccTransfer::Queued );
    m_urlreqLocation->lineEdit()->setFrame( transfer->getStatus() == DccTransfer::Queued );
    m_urlreqLocation->button()->setEnabled( transfer->getStatus() == DccTransfer::Queued );

    // Partner:
    // FIXME: also display the server which the nick belongs to
    if ( transfer->getPartnerNick().isEmpty() )
        m_labelPartner->setText( "" );
    else
        m_labelPartner->setText( i18n( "%1 %2 (port %3)" ).arg( transfer->getPartnerNick(), transfer->getPartnerIp(), transfer->getPartnerPort() ) );

    // Self:
    if ( transfer->getOwnIp().isEmpty() )
        m_labelSelf->setText( "" );
    else
        m_labelSelf->setText( i18n( "%1 (port %2)" ).arg( transfer->getOwnIp() ).arg( transfer->getOwnPort() ) );

    // Status:
    if ( transfer->getStatus() == DccTransfer::Transferring )
        m_labelStatus->setText( m_item->getStatusText() + " ( " + m_item->getCurrentSpeedPrettyText() + " )" );
    else
        m_labelStatus->setText( transfer->getStatusDetail().isEmpty() ? m_item->getStatusText() : m_item->getStatusText() + " (" + transfer->getStatusDetail() + ')' );

    // Progress:
    m_progress->setProgress( transfer->getProgress() );

    // Current Position:
    m_labelCurrentPosition->setText( KGlobal::locale()->formatNumber( transfer->getTransferringPosition(), 0 ) );

    // File Size:
    m_labelFileSize->setText( KGlobal::locale()->formatNumber( transfer->getFileSize(), 0 ) );

    // Current Speed:
    m_labelCurrentSpeed->setText( m_item->getCurrentSpeedPrettyText() );

    // Average Speed:
    // FIXME

    // Resumed:
    if ( transfer->isResumed() )
        m_labelIsResumed->setText( i18n( "Yes, %1" ).arg( KGlobal::locale()->formatNumber( transfer->getTransferStartPosition(), 0 ) ) );
    else
        m_labelIsResumed->setText( i18n( "No" ) );

    // Transferring Time:
    if ( transfer->getTimeTransferStarted().isNull() )
        m_labelTransferringTime->setText( "" );
    else
    {
        int transferringTime = transfer->getTimeTransferStarted().secsTo( transfer->getTimeTransferFinished() );
        if ( transferringTime >= 1 )
            m_labelTransferringTime->setText( DccTransferPanelItem::secToHMS( transferringTime ) );
        else
            m_labelTransferringTime->setText( i18n( "< 1sec" ) );
    }

    // Estimated Time Left:
    m_labelTimeLeft->setText( m_item->getTimeLeftPrettyText() );

    // Offered at:
    m_labelTimeOffered->setText( transfer->getTimeOffer().toString( "hh:mm:ss" ) );

    // Started at:
    if ( !transfer->getTimeTransferStarted().isNull() )
        m_labelTimeStarted->setText( transfer->getTimeTransferStarted().toString( "hh:mm:ss" ) );
    else
        m_labelTimeStarted->setText( "" );

    // Finished at:
    if ( !transfer->getTimeTransferFinished().isNull() )
        m_labelTimeFinished->setText( transfer->getTimeTransferFinished().toString( "hh:mm:ss" ) );
    else
        m_labelTimeFinished->setText( "" );

}

void DccTransferDetailedInfoPanel::slotTransferStatusChanged( DccTransfer* /* transfer */, int newStatus, int oldStatus )
{
    updateView();
    if ( newStatus == DccTransfer::Transferring )
    {
        // start auto view-update timer
        m_autoViewUpdateTimer->start( 500, false );
    }
    else if ( oldStatus == DccTransfer::Transferring )
    {
        // stop auto view-update timer
        m_autoViewUpdateTimer->stop();
    }
}

void DccTransferDetailedInfoPanel::slotLocationChanged( const QString& url )
{
    m_item->transfer()->setFileURL( KURL::fromPathOrURL( url ) );
}

#include "dcctransferdetailedinfopanel.moc"
