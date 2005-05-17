/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
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
#include "dccpanel.h"
#include "dcctransfer.h"
#include "konversationapplication.h"


#define TIME_REMAINING_NOT_AVAILABLE -1
#define TIME_REMAINING_INFINITE      -2

#define CPS_UNKNOWN -1


DccTransfer::DccTransfer( DccPanel* panel, DccType dccType, const QString& partnerNick )
  : KListViewItem( panel->getListView() )
{
  m_dccType = dccType;
  m_partnerNick = partnerNick;
  
  m_dccStatus = Queued;
  m_resumed = false;
  m_transferringPosition = 0;
  m_transferStartPosition = 0;
  m_partnerIp = QString::null;
  m_partnerPort = QString::null;
  m_ownIp = QString::null;
  m_ownPort = QString::null;
  m_timeOffer = QDateTime::currentDateTime();
  
  m_bufferSize = KonversationApplication::preferences.getDccBufferSize();
  m_buffer = new char[ m_bufferSize ];
  
  m_autoUpdateViewTimer = 0;
  m_transferLoggerTimer = 0;
  
  m_progressBar = new KProgress( 100, listView()->viewport() );
  m_progressBar->setCenterIndicator( true );
  m_progressBar->setPercentageVisible( true );
  
  m_detailDialog = 0;
  
  // FIXME: we shouldn't do these init in the instance constructer
  s_dccTypeText[ Send ]    = i18n("Send");
  s_dccTypeText[ Receive ] = i18n("Receive");
  
  s_dccStatusText[ Queued ]        = i18n("Queued");
  s_dccStatusText[ Preparing ]     = i18n("Preparing");
  s_dccStatusText[ WaitingRemote ] = i18n("Offering");
  s_dccStatusText[ Connecting ]    = i18n("Connecting");
  s_dccStatusText[ Sending ]       = i18n("Sending");
  s_dccStatusText[ Receiving ]     = i18n("Receiving");
  s_dccStatusText[ Done ]          = i18n("Done");
  s_dccStatusText[ Failed ]        = i18n("Failed");
  s_dccStatusText[ Aborted ]       = i18n("Aborted");
  s_dccStatusText[ Removed ]       = i18n("Removed");
}

DccTransfer::~DccTransfer()
{
  stopAutoUpdateView();
  closeDetailDialog();
  delete[] m_buffer;
  delete m_progressBar;
}

void DccTransfer::updateView()  // slot, protected
{
  updateTransferMeters();
  
  setPixmap( DccPanel::Column::TypeIcon, getTypeIcon() );
  setPixmap( DccPanel::Column::Status,   getStatusIcon() );
  
  setText( DccPanel::Column::OfferDate,     m_timeOffer.toString( "hh:mm:ss" ) );
  setText( DccPanel::Column::Status,        getStatusText() );
  setText( DccPanel::Column::FileName,      m_fileName );
  setText( DccPanel::Column::PartnerNick,   m_partnerNick );
  setText( DccPanel::Column::Position,      getPositionPrettyText() );
  setText( DccPanel::Column::TimeRemaining, getTimeRemainingPrettyText() );
  setText( DccPanel::Column::CPS,           getCPSPrettyText() );
  setText( DccPanel::Column::SenderAddress, getSenderAddressPrettyText() );
  
  if ( m_fileSize )
    m_progressBar->setProgress( getProgress() );
  else  // filesize is unknown
    setText( DccPanel::Column::Progress, i18n( "unknown" ) );
  
  if ( m_detailDialog )
    m_detailDialog->updateView();
}

int DccTransfer::compare( QListViewItem* i, int col, bool ascending ) const  // virtual
{
  DccTransfer* item = static_cast<DccTransfer*>( i );
  
  switch ( col )
  {
    case DccPanel::Column::TypeIcon:
      if ( m_dccType > item->getType() ) return 1;
      if ( m_dccType < item->getType() ) return -1;
      return 0;
      break;
    case DccPanel::Column::OfferDate:
      if ( m_timeOffer > item->getTimeOffer() ) return 1;
      if ( m_timeOffer < item->getTimeOffer() ) return -1;
      return 0;
      break;
    case DccPanel::Column::Status:
      if ( m_dccStatus > item->getStatus() ) return 1;
      if ( m_dccStatus < item->getStatus() ) return -1;
      return 0;
      break;
    case DccPanel::Column::Position:
      if ( m_transferringPosition > item->getTransferringPosition() ) return 1;
      if ( m_transferringPosition < item->getTransferringPosition() ) return -1;
      return 0;
      break;
    case DccPanel::Column::TimeRemaining:
      if ( getTimeRemaining() > item->getTimeRemaining() ) return 1;
      if ( getTimeRemaining() < item->getTimeRemaining() ) return -1;
      return 0;
      break;
    case DccPanel::Column::CPS:
      if ( getCPS() > item->getCPS() ) return 1;
      if ( getCPS() < item->getCPS() ) return -1;
      return 0;
      break;
    default:
      return QListViewItem::compare( i, col, ascending );
  }
}

void DccTransfer::initTransferMeter()  // protected
{
  kdDebug() << "DccTransfer::initTransferMeter()" << endl;
  m_transferLoggerTimer = new QTimer( this );
  connect( m_transferLoggerTimer, SIGNAL( timeout() ), this, SLOT( slotLogTransfer() ) );
  m_timeTransferStarted = QDateTime::currentDateTime();
  m_transferTime.start();
  m_transferLoggerTimer->start( 100 );
  startAutoUpdateView();
}

void DccTransfer::finishTransferMeter()  // protected
{
  if ( m_timeTransferFinished.isNull() )
    m_timeTransferFinished = QDateTime::currentDateTime();
  if ( m_transferLoggerTimer )
  {
    m_transferLoggerTimer->stop();
    delete m_transferLoggerTimer;
    m_transferLoggerTimer = 0;
  }
  stopAutoUpdateView();
}

void DccTransfer::startAutoUpdateView()
{
  stopAutoUpdateView();
  m_autoUpdateViewTimer = new QTimer( this );
  connect( m_autoUpdateViewTimer, SIGNAL( timeout() ), this, SLOT( updateView()) );
  m_autoUpdateViewTimer->start( 500 );
}

void DccTransfer::stopAutoUpdateView()
{
  if ( m_autoUpdateViewTimer )
  {
    m_autoUpdateViewTimer->stop();
    delete m_autoUpdateViewTimer;
    m_autoUpdateViewTimer = 0;
  }
}

void DccTransfer::slotLogTransfer()  // private
{
  // *************************************************************
  // TODO: URGENT:
  // QTime can't handle a longer time more than a day
  // *************************************************************
  kdDebug() << "transfer logger: " <<  m_transferTime.elapsed() << " " << m_transferringPosition << endl;
  m_transferLogTime.append( m_transferTime.elapsed() );
  m_transferLogPosition.append( m_transferringPosition );
}

void DccTransfer::paintCell( QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment )  // public virtual
{
  KListViewItem::paintCell( painter, colorgroup, column, width, alignment );
  if ( column == DccPanel::Column::Progress )
    showProgressBar();
}

void DccTransfer::showProgressBar()
{
  if ( m_fileSize )
  {
    QRect rect = listView()->itemRect( this );
    QHeader *head = listView()->header();
    rect.setLeft( head->sectionPos( DccPanel::Column::Progress ) - head->offset() );
    rect.setWidth( head->sectionSize( DccPanel::Column::Progress ) );
    m_progressBar->setGeometry( rect );
    m_progressBar->show();
  }
}

void DccTransfer::runFile()  // public
{
  if ( m_dccType == Send || m_dccStatus == Done )
    new KRun( m_fileURL, listView() );
}

void DccTransfer::removeFile()  // public
{
  if ( m_dccType != Receive || m_dccStatus != Done )
    return;
  KIO::SimpleJob* deleteJob = KIO::file_delete( m_fileURL, false );  // is it better to show the progress dialog?
  connect( deleteJob, SIGNAL( result( KIO::Job* ) ), this, SLOT( slotRemoveFileDone( KIO::Job* ) ) );
}

void DccTransfer::slotRemoveFileDone( KIO::Job* job )
{
  if ( job->error() )
    KMessageBox::sorry( listView(), i18n("Cannot remove file '%1'.").arg( m_fileURL.url() ), i18n("DCC Error") );
  else
  {
    setStatus( Removed );
    updateView();
  }
}

void DccTransfer::openFileInfoDialog()  // public
{
  if ( m_dccType == Send || m_dccStatus == Done )
  {
    QStringList infoList;
    
    QString path=m_fileURL.path();
    
    // get meta info object
    KFileMetaInfo fileInfo(path,QString::null,KFileMetaInfo::Everything);
    
    // is there any info for this file?
    if(fileInfo.isEmpty())
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
              infoList.append("- "+item.translatedKey()+" "+item.string());
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

void DccTransfer::openDetailDialog()  // public
{
  if ( !m_detailDialog )
    m_detailDialog = new DccDetailDialog( this );
  m_detailDialog->show();
}

void DccTransfer::closeDetailDialog()  // public
{
  if ( m_detailDialog )
  {
    delete m_detailDialog;
    m_detailDialog = 0;
  }
}

void DccTransfer::setStatus( DccStatus status, const QString& statusDetail )  // protected
{
  bool changed = ( status != m_dccStatus );
  m_dccStatus = status;
  m_dccStatusDetail = statusDetail;
  if ( changed )
    emit statusChanged( this );
}

void DccTransfer::updateTransferMeters()
{
  const int timeToCalc = 5;
  
  if ( m_dccStatus == Sending || m_dccStatus == Receiving )
  {
    // update CPS
    QValueList<int>::iterator itTime = m_transferLogTime.begin();
    QValueList<KIO::fileoffset_t>::iterator itPos = m_transferLogPosition.begin();
    while ( itTime != m_transferLogTime.end() && ( m_transferLogTime.last() - (*itTime) > timeToCalc * 1000 ) )
    {
      itTime = m_transferLogTime.remove( itTime );
      itPos = m_transferLogPosition.remove( itPos );
    }
    int timeElapsed = m_timeTransferStarted.secsTo( QDateTime::currentDateTime() );
    if ( m_transferLogTime.count() >= 2 )
      m_cps = (double)( m_transferLogPosition.last() - m_transferLogPosition.front() ) / (double)( m_transferLogTime.last() - m_transferLogTime.front() ) * 1000;
    else  // avoid zero devision
      m_cps = CPS_UNKNOWN;
    
    // update the remaining time
    if ( m_cps <= 0 )
      m_timeRemaining = TIME_REMAINING_INFINITE;
    else
      m_timeRemaining = (int)( (double)( m_fileSize - m_transferringPosition ) / m_cps );
  }
  else if ( m_dccStatus >= Done )
  {
    if ( m_timeTransferStarted.secsTo( m_timeTransferFinished ) <= 0 )  // avoid zero devision
      m_cps = CPS_UNKNOWN;
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

QString DccTransfer::getTypeText() const
{
  return s_dccTypeText[ m_dccType ];
}

QPixmap DccTransfer::getTypeIcon() const
{
  QString icon;
  if ( m_dccType == Send )
    icon = "up";
  else if ( m_dccType == Receive )
    icon = "down";
  return KGlobal::iconLoader()->loadIcon( icon, KIcon::Small );
}

QPixmap DccTransfer::getStatusIcon() const
{
  QString icon;
  switch ( m_dccStatus )
  {
    case Queued:
      icon = "player_stop";
      break;
    case Preparing:
    case WaitingRemote:
    case Connecting:
      icon = "goto";
      break;
    case Sending:
    case Receiving:
      icon = "player_play";
      break;
    case Done:
      icon = "ok";
      break;
    case Aborted:
    case Failed:
      icon = "stop";
      break;
    case Removed:
      icon = "trashcan_full";
      break;
    default: ;  // sugar
  }
  return KGlobal::iconLoader()->loadIcon( icon, KIcon::Small );
}

QString DccTransfer::getStatusText() const
{
  return s_dccStatusText[ m_dccStatus ];
}

QString DccTransfer::getFileSizePrettyText() const
{
  return KIO::convertSize( m_fileSize );
}

int DccTransfer::getProgress() const
{
  return (int)( ( (double)m_transferringPosition / (double)m_fileSize ) * 100 );
}

QString DccTransfer::getPositionPrettyText( bool detailed ) const
{
  if ( detailed )
    return getPrettyNumberText( QString::number( m_transferringPosition ) ) + " / " + getPrettyNumberText( QString::number( m_fileSize ) );
  else
    return KIO::convertSize( m_transferringPosition ) + " / " + KIO::convertSize( m_fileSize );
}

QString DccTransfer::getTimeRemainingPrettyText() const
{
  if ( m_timeRemaining == TIME_REMAINING_NOT_AVAILABLE )
    return QString::null;
  else if ( m_timeRemaining == TIME_REMAINING_INFINITE )
    return QString( "?" );
  else
  {
    int remTime = m_timeRemaining;
    int remHour = remTime / 3600; remTime -= remHour * 3600;
    int remMin = remTime / 60; remTime -= remMin * 60;
    QString text;
    if ( remHour )
      text += QString::number( remHour ) + ":";
    if ( remMin )
      text += QString::number( remMin ) + ":";
    if ( text.isEmpty() )
      text = i18n("%1 sec").arg( QString::number( remTime ) );
    else
      text += QString::number( remTime );
    return text;
  }
}

QString DccTransfer::getCPSPrettyText() const
{
  if ( m_cps == CPS_UNKNOWN )
    return QString( "?" );
  else
    return i18n("%1/sec").arg( KIO::convertSize( (KIO::fileoffset_t)m_cps ) );
}

QString DccTransfer::getSenderAddressPrettyText() const
{
  if ( m_dccType == Send )
    return QString( "%1:%2" ).arg( m_ownIp ).arg( m_ownPort );
  else
    return QString( "%1:%2" ).arg( m_partnerIp ).arg( m_partnerPort );
}

//FIXME: IPv6 support
QString DccTransfer::getNumericalIpText( const QString& ipString )  // protected, static
{
  QHostAddress ip;
  ip.setAddress( ipString );
  
  return QString::number( ip.ip4Addr() );
}

unsigned long DccTransfer::intel( unsigned long value )  // protected, static
{
  value = ( (value & 0xff000000) >> 24 ) +
          ( (value & 0xff0000) >> 8 ) +
          ( (value & 0xff00) << 8 ) +
          ( (value & 0xff) << 24 );

  return value;
}

QString DccTransfer::getPrettyNumberText( const QString& numberText )  // protected, static
{
  QString prettyNumberText = numberText;
  int commas = (int)( ( numberText.length() - 1 ) / 3 );
  for ( int i=0 ; i < commas ; ++i )
    prettyNumberText.insert( numberText.length() - ( ( i + 1 ) * 3 ), "," );
  return prettyNumberText;  
}

// public functions
DccTransfer::DccType    DccTransfer::getType()                  const { return m_dccType; }
DccTransfer::DccStatus  DccTransfer::getStatus()                const { return m_dccStatus; }
QDateTime               DccTransfer::getTimeOffer()             const { return m_timeOffer; }
QString                 DccTransfer::getOwnIp()                 const { return m_ownIp; }
QString                 DccTransfer::getOwnPort()               const { return m_ownPort; }
QString                 DccTransfer::getPartnerNick()           const { return m_partnerNick; }
QString                 DccTransfer::getFileName()              const { return m_fileName; }
KIO::filesize_t         DccTransfer::getFileSize()              const { return m_fileSize; }
KIO::fileoffset_t       DccTransfer::getTransferringPosition()  const { return m_transferringPosition; }
KURL                    DccTransfer::getFileURL()               const { return m_fileURL; }
bool                    DccTransfer::isResumed()                const { return m_resumed; }
unsigned long           DccTransfer::getCPS()                   const { return (unsigned long)m_cps; }
int                     DccTransfer::getTimeRemaining()         const { return m_timeRemaining; }

QString DccTransfer::s_dccTypeText[ DccTransfer::DccTypeCount ];
QString DccTransfer::s_dccStatusText[ DccTransfer::DccStatusCount ];


#include "dcctransfer.moc"
