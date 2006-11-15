/*
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004,2005 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <qheader.h>
#include <qhostaddress.h>
#include <qstyle.h>
#include <qtimer.h>

#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprogress.h>
#include <krun.h>

#include "dccdetaildialog.h"
#include "dcctransferpanel.h"
#include "dcctransferpanelitem.h"
#include "konversationapplication.h"
#include "config/preferences.h"

DccTransferPanelItem::DccTransferPanelItem( DccTransferPanel* panel, DccTransfer* transfer )
    : KListViewItem( panel->getListView() )
    , m_panel( panel )
    , m_transfer( transfer )
{
    m_autoUpdateViewTimer = 0;

    m_progressBar = new KProgress( 100, listView()->viewport() );
    m_progressBar->setCenterIndicator( true );
    m_progressBar->setPercentageVisible( true );

    m_detailDialog = 0;

    m_fileRemoved = false;

    connect( m_transfer, SIGNAL( transferStarted( DccTransfer* ) ), this, SLOT( startAutoViewUpdate() ) );
    connect( m_transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( stopAutoViewUpdate() ) );

    connect( m_transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotStatusChanged( DccTransfer*, int, int ) ) );

    updateView();

    s_dccStatusText[ DccTransfer::Queued ]        = i18n("Queued");
    s_dccStatusText[ DccTransfer::Preparing ]     = i18n("Preparing");
    s_dccStatusText[ DccTransfer::WaitingRemote ] = i18n("Offering");
    s_dccStatusText[ DccTransfer::Connecting ]    = i18n("Connecting");
    s_dccStatusText[ DccTransfer::Sending ]       = i18n("Sending");
    s_dccStatusText[ DccTransfer::Receiving ]     = i18n("Receiving");
    s_dccStatusText[ DccTransfer::Done ]          = i18n("Done");
    s_dccStatusText[ DccTransfer::Failed ]        = i18n("Failed");
    s_dccStatusText[ DccTransfer::Aborted ]       = i18n("Aborted");
}

DccTransferPanelItem::~DccTransferPanelItem()
{
    kdDebug() << "DccTransferPanelItem::~DccTransferPanelItem()" << endl;
    stopAutoViewUpdate();
    closeDetailDialog();
    delete m_progressBar;

    //FIXME: konversion4: this object should be under the control of Konversation network core. 
    delete m_transfer;
}

void DccTransferPanelItem::updateView()
{
    setPixmap( DccTransferPanel::Column::TypeIcon, getTypeIcon() );
    setPixmap( DccTransferPanel::Column::Status,   getStatusIcon() );

    setText( DccTransferPanel::Column::OfferDate,     m_transfer->getTimeOffer().toString( "hh:mm:ss" ) );
    setText( DccTransferPanel::Column::Status,        getStatusText() );
    setText( DccTransferPanel::Column::FileName,      m_transfer->getFileURL().fileName() );
    setText( DccTransferPanel::Column::PartnerNick,   m_transfer->getPartnerNick() );
    setText( DccTransferPanel::Column::Position,      getPositionPrettyText() );
    setText( DccTransferPanel::Column::TimeRemaining, getTimeRemainingPrettyText() );
    setText( DccTransferPanel::Column::CPS,           getCPSPrettyText() );
    setText( DccTransferPanel::Column::SenderAddress, getSenderAddressPrettyText() );

    if ( m_transfer->getFileSize() )
        m_progressBar->setProgress( m_transfer->getProgress() );
    else // filesize is unknown
    {
        m_progressBar->hide();
        setText( DccTransferPanel::Column::Progress, i18n( "unknown" ) );
    }

    if ( m_detailDialog )
        m_detailDialog->updateView();
}


int DccTransferPanelItem::compare( QListViewItem* i, int col, bool ascending ) const
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
        case DccTransferPanel::Column::TimeRemaining:
            if ( m_transfer->getTimeRemaining() > item->transfer()->getTimeRemaining() ) return 1;
            if ( m_transfer->getTimeRemaining() < item->transfer()->getTimeRemaining() ) return -1;
            return 0;
            break;
        case DccTransferPanel::Column::CPS:
            if ( m_transfer->getCPS() > item->transfer()->getCPS() ) return 1;
            if ( m_transfer->getCPS() < item->transfer()->getCPS() ) return -1;
            return 0;
            break;
        default:
            return QListViewItem::compare( i, col, ascending );
    }
}

void DccTransferPanelItem::slotStatusChanged( DccTransfer* /* transfer */, int newStatus, int oldStatus )
{
    updateView();

    if ( newStatus == DccTransfer::Sending || newStatus == DccTransfer::Receiving )
        startAutoViewUpdate();

    if ( newStatus == DccTransfer::Failed && oldStatus != DccTransfer::Queued )
        openDetailDialog();
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
        QHeader *head = listView()->header();
        rect.setLeft( head->sectionPos( DccTransferPanel::Column::Progress ) - head->offset() );
        rect.setWidth( head->sectionSize( DccTransferPanel::Column::Progress ) );
        m_progressBar->setGeometry( rect );
        m_progressBar->show();
    }
}

void DccTransferPanelItem::runFile()
{
    if ( m_fileRemoved )
        return;

    if ( m_transfer->getType() == DccTransfer::Send || m_transfer->getStatus() == DccTransfer::Done )
        new KRun( m_transfer->getFileURL(), listView() );
}

void DccTransferPanelItem::removeFile()
{
    if ( m_fileRemoved )
        return;

    if ( m_transfer->getType() != DccTransfer::Receive || m_transfer->getStatus() != DccTransfer::Done )
        return;
    // is it better to show the progress dialog?
    KIO::SimpleJob* deleteJob = KIO::file_delete( m_transfer->getFileURL(), false );
    connect( deleteJob, SIGNAL( result( KIO::Job* ) ), this, SLOT( slotRemoveFileDone( KIO::Job* ) ) );
}

void DccTransferPanelItem::slotRemoveFileDone( KIO::Job* job )
{
    if ( job->error() )
        KMessageBox::sorry( listView(), i18n("Cannot remove file '%1'.").arg( m_transfer->getFileURL().url() ), i18n("DCC Error") );
    else
    {
        m_fileRemoved = true;
        updateView();
    }
}

void DccTransferPanelItem::openFileInfoDialog()
{
    if ( m_fileRemoved )
        return;

    if ( m_transfer->getType() == DccTransfer::Send || m_transfer->getStatus() == DccTransfer::Done )
    {
        QStringList infoList;

        QString path=m_transfer->getFileURL().path();

        // get meta info object
        KFileMetaInfo fileInfo(path,QString::null,KFileMetaInfo::Everything);

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

void DccTransferPanelItem::openDetailDialog()
{
    if ( !m_detailDialog )
        m_detailDialog = new DccDetailDialog( this );
    m_detailDialog->show();
}

void DccTransferPanelItem::closeDetailDialog()
{
    if ( m_detailDialog )
    {
        delete m_detailDialog;
        m_detailDialog = 0;
    }
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
        case DccTransfer::Sending:
        case DccTransfer::Receiving:
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
    return s_dccStatusText[ m_transfer->getStatus() ];
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

QString DccTransferPanelItem::getTimeRemainingPrettyText() const
{
    QString text;

    if ( m_transfer->getTimeRemaining() == TIME_REMAINING_NOT_AVAILABLE )
        ;
    else if ( m_transfer->getTimeRemaining() == TIME_REMAINING_INFINITE )
        text = "?";
    else
    {
        int remSec = m_transfer->getTimeRemaining(); 
        int remHour = remSec / 3600; remSec -= remHour * 3600; 
        int remMin = remSec / 60; remSec -= remMin * 60; 

        // remHour can be more than 25, so we can't use QTime here.
        text = QString( "%1:%2:%3" )
	  .arg( QString::number( remHour ).rightJustify( 2, '0', false ) )
	  .arg( QString::number( remMin ).rightJustify( 2, '0' ) )
	  .arg( QString::number( remSec ).rightJustify( 2, '0' ) );
    }

    return text;
}

QString DccTransferPanelItem::getCPSPrettyText() const
{
    if ( m_transfer->getCPS() == CPS_CALCULATING )
        return QString( "?" );
    else if ( m_transfer->getCPS() == CPS_NOT_IN_TRANSFER )
        return QString();
    else
        return i18n("%1/sec").arg( KIO::convertSize( (KIO::fileoffset_t)m_transfer->getCPS() ) );
}

QString DccTransferPanelItem::getSenderAddressPrettyText() const
{
    if ( m_transfer->getType() == DccTransfer::Send )
        return QString( "%1:%2" ).arg( m_transfer->getOwnIp() ).arg( m_transfer->getOwnPort() );
    else
        return QString( "%1:%2" ).arg( m_transfer->getPartnerIp() ).arg( m_transfer->getPartnerPort() );
}


QString DccTransferPanelItem::s_dccStatusText[ DccTransfer::DccStatusCount ];

#include "dcctransferpanelitem.moc"
