/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002-2004 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>
*/

#include "transfer.h" ////// header renamed
#include "preferences.h"

#include <qfileinfo.h>
#include <qhostaddress.h>

#include <kdebug.h>


DccTransfer::DccTransfer( DccType dccType, QObject* parent ) : QObject(parent)
{
    kDebug();

    m_type = dccType;

    m_status = Configuring;

    m_fileSize = 0;
    m_resumed = false;
    m_reverse = false;
    m_connectionId = -1;  // Not configured
    m_timeLeft = DccTransfer::NotInTransfer;
    m_transferringPosition = 0;
    m_transferStartPosition = 0;
    m_averageSpeed = 0.0;
    m_currentSpeed = 0.0;

    m_bufferSize = Preferences::self()->dccBufferSize();
    m_buffer = new char[ m_bufferSize ];

    connect( &m_loggerTimer, SIGNAL( timeout() ), this, SLOT( logTransfer() ) );

    m_timeOffer = QDateTime::currentDateTime();
}

DccTransfer::~DccTransfer()
{
    kDebug();
    delete[] m_buffer;
    m_loggerTimer.stop();
}

DccTransfer::DccTransfer( const DccTransfer& obj )
    : QObject()
{
    m_buffer = 0;
    m_bufferSize = 0;
    m_averageSpeed = obj.getAverageSpeed();
    m_currentSpeed = obj.getCurrentSpeed();
    m_status = obj.getStatus();
    m_statusDetail = obj.getStatusDetail();
    m_type = obj.getType();
    m_fileName = obj.getFileName();
    m_fileSize = obj.getFileSize();
    m_fileURL = obj.getFileURL();
    // m_loggerBaseTime
    // m_loggerTimer
    m_ownIp = obj.getOwnIp();
    m_ownPort = obj.getOwnPort();
    m_partnerIp = obj.getPartnerIp();
    m_partnerNick = obj.getPartnerNick();
    m_partnerPort = obj.getPartnerPort();
    m_resumed = obj.isResumed();
    m_reverse = obj.isReverse();
    m_connectionId = obj.getConnectionId();
    m_timeLeft = obj.getTimeLeft();
    m_timeOffer = obj.getTimeOffer();
    m_timeTransferFinished = obj.getTimeTransferFinished();
    m_timeTransferStarted = obj.getTimeTransferStarted();
    // m_transferLogPosition
    // m_transferLogTime
    m_transferringPosition = obj.getTransferringPosition();
    m_transferStartPosition = obj.getTransferStartPosition();
}

void DccTransfer::setConnectionId( int id )
{
    if ( getStatus() == Configuring || getStatus() == Queued )
        m_connectionId = id;
}

void DccTransfer::setPartnerNick( const QString& nick )
{
    if ( getStatus() == Configuring || getStatus() == Queued )
        m_partnerNick = nick;
}

bool DccTransfer::queue()
{
    kDebug();
    if ( getStatus() != Configuring )
        return false;

    if ( m_fileName.isEmpty() )
        return false;

    if ( m_connectionId == -1 || m_partnerNick.isEmpty() )
        return false;

    setStatus( Queued );
    return true;
}

void DccTransfer::startTransferLogger()
{
    m_timeTransferStarted = QDateTime::currentDateTime();
    m_loggerBaseTime.start();
    m_loggerTimer.start( 100 );
}

void DccTransfer::finishTransferLogger()
{
    if ( m_timeTransferFinished.isNull() )
        m_timeTransferFinished = QDateTime::currentDateTime();
    m_loggerTimer.stop();
    updateTransferMeters();
}

// called by m_loggerTimer
void DccTransfer::logTransfer()
{
    m_transferLogTime.append( m_loggerBaseTime.elapsed() );
    m_transferLogPosition.append( m_transferringPosition );
    updateTransferMeters();
}

void DccTransfer::setStatus( DccStatus status, const QString& statusDetail )
{
    bool changed = ( status != m_status );
    DccStatus oldStatus = m_status;
    m_status = status;
    m_statusDetail = statusDetail;
    if ( changed )
        emit statusChanged( this, m_status, oldStatus );
}

void DccTransfer::updateTransferMeters()
{
    const int timeToCalc = 5;

    if ( getStatus() == Transferring )
    {
        // update CurrentSpeed

        // remove too old data
        QList<int>::iterator itTime = m_transferLogTime.begin();
        QList<KIO::fileoffset_t>::iterator itPos = m_transferLogPosition.begin();
        while ( itTime != m_transferLogTime.end() && ( m_transferLogTime.last() - (*itTime) > timeToCalc * 1000 ) )
        {
            itTime = m_transferLogTime.erase( itTime );
            itPos = m_transferLogPosition.erase( itPos );
        }

        // shift the base of the time (m_transferLoggerBaseTime)
        // reason: QTime can't handle a time longer than 24 hours
        int shiftOffset = m_loggerBaseTime.restart();
        itTime = m_transferLogTime.begin();
        for ( ; itTime != m_transferLogTime.end() ; ++itTime )
            (*itTime) = (*itTime) - shiftOffset;

        if ( m_transferLogTime.count() >= 2 )
        {
            // FIXME: precision of average speed is too bad
            m_averageSpeed = (double)( m_transferringPosition - m_transferStartPosition ) / (double)m_timeTransferStarted.secsTo( QDateTime::currentDateTime() );
            m_currentSpeed = (double)( m_transferLogPosition.last() - m_transferLogPosition.front() ) / (double)( m_transferLogTime.last() - m_transferLogTime.front() ) * 1000;
        }
        else // avoid zero devision
        {
            m_averageSpeed = DccTransfer::Calculating;
            m_currentSpeed = DccTransfer::Calculating;
        }

        // update the remaining time
        if ( m_currentSpeed <= 0 )
            m_timeLeft = DccTransfer::InfiniteValue;
        else
            m_timeLeft = (int)( (double)( m_fileSize - m_transferringPosition ) / m_currentSpeed );
    }
    else if ( m_status >= Done )
    {
        if ( m_timeTransferStarted.secsTo( m_timeTransferFinished ) > 1 )
            m_averageSpeed = (double)( m_transferringPosition - m_transferStartPosition ) / (double)m_timeTransferStarted.secsTo( m_timeTransferFinished );
        else
            m_averageSpeed = DccTransfer::InfiniteValue;
        m_currentSpeed = 0;
        m_timeLeft = DccTransfer::NotInTransfer;
    }
    else
    {
        m_averageSpeed = 0;
        m_currentSpeed = 0;
        m_timeLeft = DccTransfer::NotInTransfer;
    }
}

QString DccTransfer::sanitizeFileName( const QString& fileName )
{
    QString fileNameTmp = QFileInfo( fileName ).fileName();
    if ( fileNameTmp.startsWith( '.' ) )
        fileNameTmp.replace( 0, 1, '_' );         // Don't create hidden files
    if ( fileNameTmp.isEmpty() )
        fileNameTmp = "unnamed";
    return fileNameTmp;
}

unsigned long DccTransfer::intel( unsigned long value )
{
    value = ( (value & 0xff000000) >> 24 ) +
        ( (value & 0xff0000) >> 8 ) +
        ( (value & 0xff00) << 8 ) +
        ( (value & 0xff) << 24 );

    return value;
}

DccTransfer::DccType DccTransfer::getType() const
{
    return m_type;
}

DccTransfer::DccStatus DccTransfer::getStatus() const
{
    return m_status;
}

const QString& DccTransfer::getStatusDetail() const
{
    return m_statusDetail;
}

QDateTime DccTransfer::getTimeOffer() const
{
    return m_timeOffer;
}

int DccTransfer::getConnectionId() const
{
    return m_connectionId;
}

QString DccTransfer::getOwnIp() const
{
    return m_ownIp;
}

uint DccTransfer::getOwnPort() const
{
    return m_ownPort;
}

QString DccTransfer::getPartnerNick() const
{
    return m_partnerNick;
}

QString DccTransfer::getPartnerIp() const
{
    return m_partnerIp;
}

uint DccTransfer::getPartnerPort() const
{
    return m_partnerPort;
}

QString DccTransfer::getFileName() const
{
    return m_fileName;
}

KIO::filesize_t DccTransfer::getFileSize() const
{
    return m_fileSize;
}

KIO::fileoffset_t DccTransfer::getTransferringPosition() const
{
    return m_transferringPosition;
}

KIO::fileoffset_t DccTransfer::getTransferStartPosition() const
{
    return m_transferStartPosition;
}

KUrl DccTransfer::getFileURL() const
{
    return m_fileURL;
}

bool DccTransfer::isResumed() const
{
    return m_resumed;
}

bool DccTransfer::isReverse() const
{
    return m_reverse;
}

QString DccTransfer::getReverseToken() const
{
    return m_reverseToken;
}

transferspeed_t DccTransfer::getAverageSpeed() const
{
    return m_averageSpeed;
}

transferspeed_t DccTransfer::getCurrentSpeed() const
{
    return m_currentSpeed;
}

int DccTransfer::getTimeLeft() const
{
    return m_timeLeft;
}

int DccTransfer::getProgress() const
{
    return (int)( ( (double)getTransferringPosition() / (double)getFileSize() ) * 100 );
}

QDateTime DccTransfer::getTimeTransferStarted() const
{
    return m_timeTransferStarted;
}

QDateTime DccTransfer::getTimeTransferFinished() const
{
    return m_timeTransferFinished;
}

QString DccTransfer::transferFileName(const QString & fileName)
{
    if (fileName.contains(' ') && !(fileName.startsWith('\"') && fileName.endsWith('\"')))
        return '\"'+fileName+'\"';

    return fileName;
}

#include "transfer.moc"
