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

#include "transferpanelitem.h"
#include "transferpanel.h"
#include "application.h"
#include "config/preferences.h"

#include <QHostAddress>
#include <QStyle>
#include <QTimer>

#include <Q3Header>

#include <KFileMetaInfo>
#include <KGlobal>
#include <KIconLoader>
#include <KMessageBox>
#include <KProgressDialog>
#include <KRun>
#include <KIO/Job>

namespace Konversation
{
    namespace DCC
    {
        TransferPanelItem::TransferPanelItem( TransferPanel* panel, Transfer* transfer )
            : K3ListViewItem( panel->getListView() )
            , m_panel( panel )
            , m_transfer( transfer )
            , m_isTransferInstanceBackup( false )
        {
            m_autoUpdateViewTimer = 0;

            m_progressBar = new QProgressBar( listView()->viewport() );
            m_progressBar->setMaximum(100);
            m_progressBar->setTextVisible( true );

            connect( m_transfer, SIGNAL( transferStarted( Konversation::DCC::Transfer* ) ), this, SLOT( startAutoViewUpdate() ) );
            connect( m_transfer, SIGNAL( done( Konversation::DCC::Transfer* ) ), this, SLOT( stopAutoViewUpdate() ) );
            connect( m_transfer, SIGNAL( done( Konversation::DCC::Transfer* ) ), this, SLOT( backupTransferInfo( Konversation::DCC::Transfer* ) ) );

            connect( m_transfer, SIGNAL( statusChanged( Konversation::DCC::Transfer*, int, int ) ), this, SLOT( slotStatusChanged( Konversation::DCC::Transfer*, int, int ) ) );

            updateView();
        }

        TransferPanelItem::~TransferPanelItem()
        {
            kDebug();
            stopAutoViewUpdate();
            delete m_progressBar;
            if ( m_isTransferInstanceBackup )
                delete m_transfer;
        }

        void TransferPanelItem::updateView()
        {
            setPixmap( TransferPanel::Column::TypeIcon, getTypeIcon() );
            setPixmap( TransferPanel::Column::Status,   getStatusIcon() );

            setText( TransferPanel::Column::OfferDate,     m_transfer->getTimeOffer().toString( "hh:mm:ss" ) );
            setText( TransferPanel::Column::Status,        getStatusText() );
            setText( TransferPanel::Column::FileName,      m_transfer->getFileName() );
            setText( TransferPanel::Column::PartnerNick,   m_transfer->getPartnerNick() );
            setText( TransferPanel::Column::Position,      getPositionPrettyText() );
            setText( TransferPanel::Column::TimeLeft,      getTimeLeftPrettyText() );
            setText( TransferPanel::Column::CurrentSpeed,  getCurrentSpeedPrettyText() );
            setText( TransferPanel::Column::SenderAddress, getSenderAddressPrettyText() );

            if ( m_transfer->getFileSize() )
                m_progressBar->setValue( m_transfer->getProgress() );
            else // filesize is unknown
            {
                m_progressBar->hide();
                setText( TransferPanel::Column::Progress, i18n( "unknown" ) );
            }
        }


        int TransferPanelItem::compare( Q3ListViewItem* i, int col, bool ascending ) const
        {
            TransferPanelItem* item = static_cast<TransferPanelItem*>( i );

            switch ( col )
            {
                case TransferPanel::Column::TypeIcon:
                    if ( m_transfer->getType() > item->transfer()->getType() ) return 1;
                    if ( m_transfer->getType() < item->transfer()->getType() ) return -1;
                    return 0;
                    break;
                case TransferPanel::Column::OfferDate:
                    if ( m_transfer->getTimeOffer() > item->transfer()->getTimeOffer() ) return 1;
                    if ( m_transfer->getTimeOffer() < item->transfer()->getTimeOffer() ) return -1;
                    return 0;
                    break;
                case TransferPanel::Column::Status:
                    if ( m_transfer->getStatus() > item->transfer()->getStatus() ) return 1;
                    if ( m_transfer->getStatus() < item->transfer()->getStatus() ) return -1;
                    return 0;
                    break;
                case TransferPanel::Column::Position:
                    if ( m_transfer->getTransferringPosition() > item->transfer()->getTransferringPosition() ) return 1;
                    if ( m_transfer->getTransferringPosition() < item->transfer()->getTransferringPosition() ) return -1;
                    return 0;
                    break;
                case TransferPanel::Column::TimeLeft:
                    if ( m_transfer->getTimeLeft() > item->transfer()->getTimeLeft() ) return 1;
                    if ( m_transfer->getTimeLeft() < item->transfer()->getTimeLeft() ) return -1;
                    return 0;
                    break;
                case TransferPanel::Column::CurrentSpeed:
                    if ( m_transfer->getCurrentSpeed() > item->transfer()->getCurrentSpeed() ) return 1;
                    if ( m_transfer->getCurrentSpeed() < item->transfer()->getCurrentSpeed() ) return -1;
                    return 0;
                    break;
                default:
                    return Q3ListViewItem::compare( i, col, ascending );
            }
            return Q3ListViewItem::compare( i, col, ascending );
        }

        void TransferPanelItem::slotStatusChanged( Transfer* /* transfer */, int newStatus, int /* oldStatus */ )
        {
            updateView();

            if ( newStatus == Transfer::Transferring )
                startAutoViewUpdate();
        }

        void TransferPanelItem::startAutoViewUpdate()
        {
            stopAutoViewUpdate();
            m_autoUpdateViewTimer = new QTimer( this );
            connect( m_autoUpdateViewTimer, SIGNAL( timeout() ), this, SLOT( updateView()) );
            m_autoUpdateViewTimer->start( 500 );
        }

        void TransferPanelItem::stopAutoViewUpdate()
        {
            if ( m_autoUpdateViewTimer )
            {
                m_autoUpdateViewTimer->stop();
                delete m_autoUpdateViewTimer;
                m_autoUpdateViewTimer = 0;
            }
        }

        void TransferPanelItem::paintCell( QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment ) // virtual public
        {
            K3ListViewItem::paintCell( painter, colorgroup, column, width, alignment );
            if ( column == TransferPanel::Column::Progress )
                showProgressBar();
        }

        void TransferPanelItem::showProgressBar()
        {
            if ( m_transfer->getFileSize() )
            {
                QRect rect = listView()->itemRect( this );
                Q3Header *head = listView()->header();
                rect.setLeft( head->sectionPos( TransferPanel::Column::Progress ) - head->offset() );
                rect.setWidth( head->sectionSize( TransferPanel::Column::Progress ) );
                m_progressBar->setGeometry( rect );
                m_progressBar->show();
            }
        }

        void TransferPanelItem::runFile()
        {
            if ( m_transfer->getType() == Transfer::Send || m_transfer->getStatus() == Transfer::Done )
                new KRun( m_transfer->getFileURL(), listView() );
        }

        void TransferPanelItem::openLocation()
        {
            QString urlString = m_transfer->getFileURL().path();
            if ( !urlString.isEmpty() )
            {
                KUrl url( urlString );
                url.setFileName( QString() );
                new KRun( url, 0, 0, true, true );
            }
        }

        void TransferPanelItem::openFileInfoDialog()
        {
            if ( m_transfer->getType() == Transfer::Send || m_transfer->getStatus() == Transfer::Done )
            {
                QStringList infoList;

                QString path=m_transfer->getFileURL().path();

                // get meta info object
                KFileMetaInfo fileMetaInfo(path,QString(),KFileMetaInfo::Everything);

                // is there any info for this file?
                if (fileMetaInfo.isValid())
                {
                    const QHash<QString, KFileMetaInfoItem>& items = fileMetaInfo.items();
                    QHash<QString, KFileMetaInfoItem>::const_iterator it = items.constBegin();
                    const QHash<QString, KFileMetaInfoItem>::const_iterator end = items.constEnd();
                    while (it != end)
                    {
                        const KFileMetaInfoItem& metaInfoItem = it.value();
                        const QVariant& value = metaInfoItem.value();
                        if (value.isValid())
                        {
                            // append item information to list
                            infoList.append("- "+metaInfoItem.name()+' '+value.toString());
                        }
                        ++it;
                    }

                    // display information list if any available
                    if(infoList.count())
                    {
                        #ifdef USE_INFOLIST
                        KMessageBox::informationList(
                            listView(),
                            i18n("Available information for file %1:", path),
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

        void TransferPanelItem::backupTransferInfo( Transfer* transfer )
        {
            kDebug();
            // instances of Transfer are deleted immediately after the transfer is done
            // so we need to make a backup of Transfer.

            m_transfer = new Transfer( *transfer );
            m_isTransferInstanceBackup = true;
        }

        QString TransferPanelItem::getTypeText() const
        {
            if ( m_transfer->getType() == Transfer::Send )
                return i18n( "Send" );
            else
                return i18n( "Receive" );
        }

        QPixmap TransferPanelItem::getTypeIcon() const
        {
            if ( m_transfer->getType() == Transfer::Send )
                return KIconLoader::global()->loadIcon( "arrow-up", KIconLoader::Small );
            else
                return KIconLoader::global()->loadIcon( "arrow-down", KIconLoader::Small );
        }

        QPixmap TransferPanelItem::getStatusIcon() const
        {
            QString icon;
            switch ( m_transfer->getStatus() )
            {
                case Transfer::Queued:
                    icon = "media-playback-stop";
                    break;
                case Transfer::Preparing:
                case Transfer::WaitingRemote:
                case Transfer::Connecting:
                    icon = "network-disconnect";
                    break;
                case Transfer::Transferring:
                    icon = "media-playback-start";
                    break;
                case Transfer::Done:
                    icon = "dialog-ok";
                    break;
                case Transfer::Aborted:
                case Transfer::Failed:
                    icon = "process-stop";
                    break;
                default:
                break;
            }
            return KIconLoader::global()->loadIcon( icon, KIconLoader::Small );
        }

        QString TransferPanelItem::getStatusText() const
        {
            Transfer::Status status = m_transfer->getStatus();
            Transfer::Type type = m_transfer->getType();

            if ( status == Transfer::Queued )
                return i18n( "Queued" );
            else if ( status == Transfer::Preparing )
                return i18n( "Preparing" );
            else if ( status == Transfer::WaitingRemote )
                return i18n( "Awaiting" );
            else if ( status == Transfer::Connecting )
                return i18n( "Connecting" );
            else if ( status == Transfer::Transferring && type == Transfer::Receive )
                return i18n( "Receiving" );
            else if ( status == Transfer::Transferring && type == Transfer::Send )
                return i18n( "Sending" );
            else if ( status == Transfer::Done )
                return i18n( "Done" );
            else if ( status == Transfer::Failed )
                return i18n( "Failed" );
            else if ( status == Transfer::Aborted )
                return i18n( "Aborted" );

            return QString();
        }

        QString TransferPanelItem::getFileSizePrettyText() const
        {
            return KIO::convertSize( m_transfer->getFileSize() );
        }

        QString TransferPanelItem::getPositionPrettyText( bool detailed ) const
        {
            if ( detailed )
                return KGlobal::locale()->formatNumber( m_transfer->getTransferringPosition(), 0 )  + " / " +
                    KGlobal::locale()->formatNumber( m_transfer->getFileSize(), 0 );
            else
                return KIO::convertSize( m_transfer->getTransferringPosition() ) + " / " + KIO::convertSize( m_transfer->getFileSize() );
        }

        QString TransferPanelItem::getTimeLeftPrettyText() const
        {
            QString text;

            if ( m_transfer->getTimeLeft() == Transfer::NotInTransfer )
                ;
            else if ( m_transfer->getTimeLeft() == Transfer::InfiniteValue )
                text = '?';
            else
                text = secToHMS( m_transfer->getTimeLeft() );

            return text;
        }

        QString TransferPanelItem::getAverageSpeedPrettyText() const
        {
            return getSpeedPrettyText( m_transfer->getAverageSpeed() );
        }

        QString TransferPanelItem::getCurrentSpeedPrettyText() const
        {
            return getSpeedPrettyText( m_transfer->getCurrentSpeed() );
        }

        QString TransferPanelItem::getSenderAddressPrettyText() const
        {
            if ( m_transfer->getType() == Transfer::Send )
                return QString( "%1:%2" ).arg( m_transfer->getOwnIp() ).arg( m_transfer->getOwnPort() );
            else
                return QString( "%1:%2" ).arg( m_transfer->getPartnerIp() ).arg( m_transfer->getPartnerPort() );
        }

        QString TransferPanelItem::getSpeedPrettyText( transferspeed_t speed )
        {
            if ( speed == Transfer::Calculating || speed == Transfer::InfiniteValue )
                return QString( "?" );
            else if ( speed == Transfer::NotInTransfer )
                return QString();
            else
                return i18n("%1/sec", KIO::convertSize( (KIO::fileoffset_t)speed ) );
        }

        QString TransferPanelItem::secToHMS( long sec )
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

    }
}

#include "transferpanelitem.moc"
