/*
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "dcctransferpanelitem.h"
#include "dcctransferpanel.h"
#include "konversationapplication.h"
#include "config/preferences.h"

#include <q3header.h>
#include <qhostaddress.h>
#include <qstyle.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QPixmap>

#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprogress.h>
#include <krun.h>


DccTransferPanelItem::DccTransferPanelItem( DccTransferPanel* panel, DccTransfer* transfer )
    : KListViewItem( panel->getListView() )
    , m_panel( panel )
    , m_transfer( transfer )
    , m_isTransferInstanceBackup( false )
{
    m_autoUpdateViewTimer = 0;

    m_progressBar = new KProgress( 100, listView()->viewport() );
    m_progressBar->setCenterIndicator( true );
    m_progressBar->setPercentageVisible( true );

    connect( m_transfer, SIGNAL( transferStarted( DccTransfer* ) ), this, SLOT( startAutoViewUpdate() ) );
    connect( m_transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( stopAutoViewUpdate() ) );
    connect( m_transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( backupTransferInfo( DccTransfer* ) ) );

    connect( m_transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotStatusChanged( DccTransfer*, int, int ) ) );

    updateView();
}

DccTransferPanelItem::~DccTransferPanelItem()
{
    kdDebug() << "DccTransferPanelItem::~DccTransferPanelItem()" << endl;
    stopAutoViewUpdate();
    delete m_progressBar;
    if ( m_isTransferInstanceBackup )
        delete m_transfer;
}

void DccTransferPanelItem::updateView()
{
    setPixmap( DccTransferPanel::Column::TypeIcon, getTypeIcon() );
    setPixmap( DccTransferPanel::Column::Status,   getStatusIcon() );

    setText( DccTransferPanel::Column::OfferDate,     m_transfer->getTimeOffer().toString( "hh:mm:ss" ) );
    setText( DccTransferPanel::Column::Status,        getStatusText() );
    setText( DccTransferPanel::Column::FileName,      m_transfer->getFileName() );
    setText( DccTransferPanel::Column::PartnerNick,   m_transfer->getPartnerNick() );
    setText( DccTransferPanel::Column::Position,      getPositionPrettyText() );
    setText( DccTransferPanel::Column::TimeLeft,      getTimeLeftPrettyText() );
    setText( DccTransferPanel::Column::CurrentSpeed,  getCurrentSpeedPrettyText() );
    setText( DccTransferPanel::Column::SenderAddress, getSenderAddressPrettyText() );

    if ( m_transfer->getFileSize() )
        m_progressBar->setProgress( m_transfer->getProgress() );
    else // filesize is unknown
    {
        m_progressBar->hide();
        setText( DccTransferPanel::Column::Progress, i18n( "unknown" ) );
    }
}


int DccTransferPanelItem::compare( Q3ListViewItem* i, int col, bool ascending ) const
{
    DccTransferPanelItem* item = static_cast<DccTransferPanelItem*>( i );

    switch ( col )
    {
        case DccTransferPanel::Column::TypeIcon:
            if ( m_transfer->getType() > item->transfer()->getType() ) return 1;
            if ( m_transfer->getType() < item->transfer()->getType() ) return -1;
            return 0;
            break;
        case DccTransferPanel::Column::OfferDate:
            if ( m_transfer->getTimeOffer() > item->transfer()->getTimeOffer() ) return 1;
            if ( m_transfer->getTimeOffer() < item->transfer()->getTimeOffer() ) return -1;
            return 0;
            break;
        case DccTransferPanel::Column::Status:
            if ( m_transfer->getStatus() > item->transfer()->getStatus() ) return 1;
            if ( m_transfer->getStatus() < item->transfer()->getStatus() ) return -1;
            return 0;
            break;
        case DccTransferPanel::Column::Position:
            if ( m_transfer->getTransferringPosition() > item->transfer()->getTransferringPosition() ) return 1;
            if ( m_transfer->getTransferringPosition() < item->transfer()->getTransferringPosition() ) return -1;
            return 0;
            break;
        case DccTransferPanel::Column::TimeLeft:
            if ( m_transfer->getTimeLeft() > item->transfer()->getTimeLeft() ) return 1;
            if ( m_transfer->getTimeLeft() < item->transfer()->getTimeLeft() ) return -1;
            return 0;
            break;
        case DccTransferPanel::Column::CurrentSpeed:
            if ( m_transfer->getCurrentSpeed() > item->transfer()->getCurrentSpeed() ) return 1;
            if ( m_transfer->getCurrentSpeed() < item->transfer()->getCurrentSpeed() ) return -1;
            return 0;
            break;
        default:
            return Q3ListViewItem::compare( i, col, ascending );
    }
    return Q3ListViewItem::compare( i, col, ascending );
}

void DccTransferPanelItem::slotStatusChanged( DccTransfer* /* transfer */, int newStatus, int /* oldStatus */ )
{
    updateView();

    if ( newStatus == DccTransfer::Transferring )
        startAutoViewUpdate();
}

void DccTransferPanelItem::startAutoViewUpdate()
{
    stopAutoViewUpdate();
    m_autoUpdateViewTimer = new QTimer( this );
    connect( m_autoUpdateViewTimer, SIGNAL( timeout() ), this, SLOT( updateView()) );
    m_autoUpdateViewTimer->start( 500 );
}

void DccTransferPanelItem::stopAutoViewUpdate()
{
    if ( m_autoUpdateViewTimer )
    {
        m_autoUpdateViewTimer->stop();
        delete m_autoUpdateViewTimer;
        m_autoUpdateViewTimer = 0;
    }
}

void DccTransferPanelItem::paintCell( QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment ) // virtual public
{
    KListViewItem::paintCell( painter, colorgroup, column, width, alignment );
    if ( column == DccTransferPanel::Column::Progress )
        showProgressBar();
}

void DccTransferPanelItem::showProgressBar()
{
    if ( m_transfer->getFileSize() )
    {
        QRect rect = listView()->itemRect( this );
        Q3Header *head = listView()->header();
        rect.setLeft( head->sectionPos( DccTransferPanel::Column::Progress ) - head->offset() );
        rect.setWidth( head->sectionSize( DccTransferPanel::Column::Progress ) );
        m_progressBar->setGeometry( rect );
        m_progressBar->show();
    }
}

void DccTransferPanelItem::runFile()
{
    if ( m_transfer->getType() == DccTransfer::Send || m_transfer->getStatus() == DccTransfer::Done )
        new KRun( m_transfer->getFileURL(), listView() );
}

void DccTransferPanelItem::openFileInfoDialog()
{
    if ( m_transfer->getType() == DccTransfer::Send || m_transfer->getStatus() == DccTransfer::Done )
    {
        QStringList infoList;

        QString path=m_transfer->getFileURL().path();

        // get meta info object
        KFileMetaInfo fileInfo(path,QString(),KFileMetaInfo::Everything);

        // is there any info for this file?
        if(!fileInfo.isEmpty())
        {
            // get list of meta information groups
            QStringList groupList=fileInfo.groups();
            // look inside for keys
            for(unsigned int index=0;index<groupList.count();index++)
            {
                // get next group
                KFileMetaInfoGroup group=fileInfo.group(groupList[index]);
                // check if there are keys in this group at all
                if(!group.isEmpty())
                {
                    // append group name to list
                    infoList.append(groupList[index]);
                    // get list of keys in this group
                    QStringList keys=group.keys();
                    for(unsigned keyIndex=0;keyIndex<keys.count();keyIndex++)
                    {
                        // get meta information item for this key
                        KFileMetaInfoItem item=group.item(keys[keyIndex]);
                        if(item.isValid())
                        {
                            // append item information to list
                            infoList.append("- "+item.translatedKey()+' '+item.string());
                        }
                    } // endfor
                }
            } // endfor

            // display information list if any available
            if(infoList.count())
            {
                #ifdef USE_INFOLIST
                KMessageBox::informationList(
                    listView(),
                    i18n("Available information for file %1:").arg(path),
                    infoList,
                    i18n("File Information")
                    );
                #else
                KMessageBox::information(
                    listView(),
                    "<qt>"+infoList.join("<br>")+"</qt>",
                    i18n("File Information")
                    );
                #endif
            }
        }
        else
        {
            KMessageBox::sorry(listView(),i18n("No detailed information for this file found."),i18n("File Information"));
        }
    }
}

void DccTransferPanelItem::backupTransferInfo( DccTransfer* transfer )
{
    kdDebug() << "DccTransferPanelItem::backupTransferInfo()" << endl;
    // instances of DccTransfer are deleted immediately after the transfer is done
    // so we need to make a backup of DccTransfer.

    m_transfer = new DccTransfer( *transfer );
    m_isTransferInstanceBackup = true;
}

QString DccTransferPanelItem::getTypeText() const
{
    if ( m_transfer->getType() == DccTransfer::Send )
        return i18n( "Send" );
    else
        return i18n( "Receive" );
}

QPixmap DccTransferPanelItem::getTypeIcon() const
{
    if ( m_transfer->getType() == DccTransfer::Send )
        return KGlobal::iconLoader()->loadIcon( "up", KIcon::Small );
    else
        return KGlobal::iconLoader()->loadIcon( "down", KIcon::Small );
}

QPixmap DccTransferPanelItem::getStatusIcon() const
{
    QString icon;
    switch ( m_transfer->getStatus() )
    {
        case DccTransfer::Queued:
            icon = "player_stop";
            break;
        case DccTransfer::Preparing:
        case DccTransfer::WaitingRemote:
        case DccTransfer::Connecting:
            icon = "goto";
            break;
        case DccTransfer::Transferring:
            icon = "player_play";
            break;
        case DccTransfer::Done:
            icon = "ok";
            break;
        case DccTransfer::Aborted:
        case DccTransfer::Failed:
            icon = "stop";
            break;
        default:
	    break;
    }
    return KGlobal::iconLoader()->loadIcon( icon, KIcon::Small );
}

QString DccTransferPanelItem::getStatusText() const
{
    DccTransfer::DccStatus status = m_transfer->getStatus();
    DccTransfer::DccType type = m_transfer->getType();

    if ( status == DccTransfer::Queued )
        return i18n( "Queued" );
    else if ( status == DccTransfer::Preparing )
        return i18n( "Preparing" );
    else if ( status == DccTransfer::WaitingRemote )
        return i18n( "Awaiting" );
    else if ( status == DccTransfer::Connecting )
        return i18n( "Connecting" );
    else if ( status == DccTransfer::Transferring && type == DccTransfer::Receive )
        return i18n( "Receiving" );
    else if ( status == DccTransfer::Transferring && type == DccTransfer::Send )
        return i18n( "Sending" );
    else if ( status == DccTransfer::Done )
        return i18n( "Done" );
    else if ( status == DccTransfer::Failed )
        return i18n( "Failed" );
    else if ( status == DccTransfer::Aborted )
        return i18n( "Aborted" );

    return QString();
}

QString DccTransferPanelItem::getFileSizePrettyText() const
{
    return KIO::convertSize( m_transfer->getFileSize() );
}

QString DccTransferPanelItem::getPositionPrettyText( bool detailed ) const
{
    if ( detailed )
        return KGlobal::locale()->formatNumber( m_transfer->getTransferringPosition(), 0 )  + " / " +
            KGlobal::locale()->formatNumber( m_transfer->getFileSize(), 0 );
    else
        return KIO::convertSize( m_transfer->getTransferringPosition() ) + " / " + KIO::convertSize( m_transfer->getFileSize() );
}

QString DccTransferPanelItem::getTimeLeftPrettyText() const
{
    QString text;

    if ( m_transfer->getTimeLeft() == DccTransfer::NotInTransfer )
        ;
    else if ( m_transfer->getTimeLeft() == DccTransfer::InfiniteValue )
        text = "?";
    else
        text = secToHMS( m_transfer->getTimeLeft() );

    return text;
}

QString DccTransferPanelItem::getAverageSpeedPrettyText() const
{
    return getSpeedPrettyText( m_transfer->getAverageSpeed() );
}

QString DccTransferPanelItem::getCurrentSpeedPrettyText() const
{
    return getSpeedPrettyText( m_transfer->getCurrentSpeed() );
}

QString DccTransferPanelItem::getSenderAddressPrettyText() const
{
    if ( m_transfer->getType() == DccTransfer::Send )
        return QString( "%1:%2" ).arg( m_transfer->getOwnIp() ).arg( m_transfer->getOwnPort() );
    else
        return QString( "%1:%2" ).arg( m_transfer->getPartnerIp() ).arg( m_transfer->getPartnerPort() );
}

QString DccTransferPanelItem::getSpeedPrettyText( transferspeed_t speed )
{
    if ( speed == DccTransfer::Calculating || speed == DccTransfer::InfiniteValue )
        return QString( "?" );
    else if ( speed == DccTransfer::NotInTransfer )
        return QString();
    else
        return i18n("%1/sec").arg( KIO::convertSize( (KIO::fileoffset_t)speed ) );
}

QString DccTransferPanelItem::secToHMS( long sec )
{
        int remSec = sec;
        int remHour = remSec / 3600; remSec -= remHour * 3600; 
        int remMin = remSec / 60; remSec -= remMin * 60; 

        // remHour can be more than 25, so we can't use QTime here.
        return QString( "%1:%2:%3" )
            .arg( QString::number( remHour ).rightJustified( 2, '0', false ) )
            .arg( QString::number( remMin ).rightJustified( 2, '0' ) )
            .arg( QString::number( remSec ).rightJustified( 2, '0' ) );
}

#include "dcctransferpanelitem.moc"
