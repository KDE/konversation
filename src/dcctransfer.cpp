/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002-2004 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004-2006 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>
*/

#include <qhostaddress.h>

#include "preferences.h"

#include "dcctransfer.h"

DccTransfer::DccTransfer( DccType dccType, const QString& partnerNick )
{
    m_dccType = dccType;
    m_partnerNick = partnerNick;

    m_dccStatus = Queued;
    m_resumed = false;
    m_transferringPosition = 0;
    m_transferStartPosition = 0;
    m_timeOffer = QDateTime::currentDateTime();

    m_bufferSize = Preferences::dccBufferSize();
    m_buffer = new char[ m_bufferSize ];

    connect( &m_loggerTimer, SIGNAL( timeout() ), this, SLOT( logTransfer() ) );
}

DccTransfer::~DccTransfer()
{
    kdDebug() << "DccTransfer::~DccTransfer()" << endl;
    delete[] m_buffer;
    m_loggerTimer.stop();
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
}

// called by m_loggerTimer
void DccTransfer::logTransfer()
{
    m_transferLogTime.append( m_loggerBaseTime.elapsed() );
    m_transferLogPosition.append( m_transferringPosition );
}

void DccTransfer::setStatus( DccStatus status, const QString& statusDetail )
{
    bool changed = ( status != m_dccStatus );
    DccStatus oldStatus = m_dccStatus;
    m_dccStatus = status;
    m_dccStatusDetail = statusDetail;
    if ( changed )
        emit statusChanged( this, m_dccStatus, oldStatus );
}

void DccTransfer::updateTransferMeters()
{
    const int timeToCalc = 5;

    if ( m_dccStatus == Sending || m_dccStatus == Receiving )
    {
        // update CPS

        // remove too old data
        QValueList<int>::iterator itTime = m_transferLogTime.begin();
        QValueList<KIO::fileoffset_t>::iterator itPos = m_transferLogPosition.begin();
        while ( itTime != m_transferLogTime.end() && ( m_transferLogTime.last() - (*itTime) > timeToCalc * 1000 ) )
        {
            itTime = m_transferLogTime.remove( itTime );
            itPos = m_transferLogPosition.remove( itPos );
        }

        // shift the base of the time (m_transferLoggerBaseTime)
        // reason: QTime can't handle a time longer than 24 hours
        int shiftOffset = m_loggerBaseTime.restart();
        itTime = m_transferLogTime.begin();
        for ( ; itTime != m_transferLogTime.end() ; ++itTime )
            (*itTime) = (*itTime) - shiftOffset;

        if ( m_transferLogTime.count() >= 2 )
            m_cps = (double)( m_transferLogPosition.last() - m_transferLogPosition.front() ) / (double)( m_transferLogTime.last() - m_transferLogTime.front() ) * 1000;
        else // avoid zero devision
            m_cps = CPS_CALCULATING;

        // update the remaining time
        if ( m_cps <= 0 )
            m_timeRemaining = TIME_REMAINING_INFINITE;
        else
            m_timeRemaining = (int)( (double)( m_fileSize - m_transferringPosition ) / m_cps );
    }
    else if ( m_dccStatus >= Done )
    {
        // avoid zero devision
        if ( m_timeTransferStarted.secsTo( m_timeTransferFinished ) <= 0 )
            m_cps = CPS_NOT_IN_TRANSFER;
        else
            m_cps = (double)( m_transferringPosition - m_transferStartPosition ) / (double)m_timeTransferStarted.secsTo( m_timeTransferFinished );
        m_timeRemaining = TIME_REMAINING_NOT_AVAILABLE;
    }
    else
    {
        m_cps = 0;
        m_timeRemaining = TIME_REMAINING_NOT_AVAILABLE;
    }
}

//FIXME: IPv6 support
QString DccTransfer::getNumericalIpText( const QString& ipString )
{
    QHostAddress ip;
    ip.setAddress( ipString );

    return QString::number( ip.ip4Addr() );
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
  return m_dccType; 
}

DccTransfer::DccStatus DccTransfer::getStatus() const 
{ 
  return m_dccStatus; 
}

const QString& DccTransfer::getStatusDetail() const
{
    return m_dccStatusDetail;
}

QDateTime DccTransfer::getTimeOffer() const 
{
  return m_timeOffer; 
}

QString DccTransfer::getOwnIp() const 
{ 
  return m_ownIp; 
}

QString DccTransfer::getOwnPort() const 
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

QString DccTransfer::getPartnerPort() const 
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

KURL DccTransfer::getFileURL() const 
{ 
  return m_fileURL;
}

bool DccTransfer::isResumed() const 
{ 
  return m_resumed; 
}

long DccTransfer::getCPS()
{ 
    //FIXME
    updateTransferMeters();
  return (unsigned long)m_cps; 
}

int DccTransfer::getTimeRemaining()
{ 
    //FIXME
    updateTransferMeters();
  return m_timeRemaining; 
}

int DccTransfer::getProgress() const
{
    return (int)( ( (double)getTransferringPosition() / (double)getFileSize() ) * 100 );
}

#include "dcctransfer.moc"
