// dccdetaildialog.cpp
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// See COPYING file for licensing information

#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>

#include <klocale.h>
#include <klineedit.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <kurlrequester.h>

#include "dccdetaildialog.h"
#include "dcctransfer.h"

DccDetailDialog::DccDetailDialog( DccTransfer* item )
  : KDialog( 0 )
  , m_item( item )
{
  QVBoxLayout* baseLayout = new QVBoxLayout( this );
  baseLayout->setMargin( marginHint() );
  baseLayout->setSpacing( spacingHint() );
  
  // information //
  
  QFrame* infoFrame = new QFrame( this );
  QGridLayout* infoLayout = new QGridLayout( infoFrame, 1, 2 );
  infoLayout->setSpacing( spacingHint() );
  
  // Filename
  QLabel* fileNameHeader = new QLabel( i18n("File"), infoFrame );
  fileNameHeader->setAlignment( AlignHCenter );
  QLabel* fileName = new QLabel( item->fileName, infoFrame );
  fileName->setAlignment( AlignHCenter );
  
  // Partner
  QLabel* partnerHeader = new QLabel( infoFrame );
  partnerHeader->setAlignment( AlignHCenter );
  if ( m_item->dccType == DccTransfer::Send )
    partnerHeader->setText( i18n("Receiver") );
  else
    partnerHeader->setText( i18n("Sender") );
  m_partner = new QLabel( infoFrame );
  m_partner->setAlignment( AlignHCenter );
  
  // Self
  QLabel* selfHeader = new QLabel( i18n("Self"), infoFrame );
  selfHeader->setAlignment( AlignHCenter );
  m_self = new QLabel( infoFrame );
  m_self->setAlignment( AlignHCenter );
  
  // Status
  QLabel* statusHeader = new QLabel( i18n("Status"), infoFrame );
  statusHeader->setAlignment( AlignHCenter );
  m_status = new QLabel( infoFrame );
  m_status->setAlignment( AlignHCenter );
  
  // Progres
  QLabel* progressHeader = new QLabel( i18n("Progress"), infoFrame );
  progressHeader->setAlignment( AlignHCenter | AlignVCenter );
  m_progress = new KProgress( 100, infoFrame );
  m_progress->setCenterIndicator( true );
  
  // Position
  QLabel* positionHeader = new QLabel( i18n("Position"), infoFrame );
  positionHeader->setAlignment( AlignHCenter );
  m_position = new QLabel( infoFrame );
  m_position->setAlignment( AlignHCenter );
  
  // Local path
  QLabel* localPathHeader = new QLabel( i18n("Local Path"), infoFrame );
  localPathHeader->setAlignment( AlignHCenter | AlignVCenter );
  m_localPath = new KURLRequester( m_item->filePath, infoFrame );
  connect( m_localPath, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotLocalPathChanged( const QString& ) ) );
  
  // buttons //
  
  QFrame* buttonFrame = new QFrame( this );
  QHBoxLayout* buttonLayout = new QHBoxLayout( buttonFrame );
  buttonLayout->setSpacing( spacingHint() );
  
  // Accept
  m_buttonAccept = 0;
  if ( m_item->dccType == DccTransfer::Receive )
  {
    m_buttonAccept = new KPushButton( i18n("Accept"), buttonFrame );
    m_buttonAccept->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    connect( m_buttonAccept, SIGNAL( clicked() ), this, SLOT( slotAccept() ) );
  }
  
  // Abort
  m_buttonAbort = new KPushButton( i18n("Abort"), buttonFrame );
  m_buttonAbort->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  connect( m_buttonAbort, SIGNAL( clicked() ), this, SLOT( slotAbort() ) );
  
  // Close
  KPushButton* buttonClose = new KPushButton( i18n("Close"), buttonFrame );
  buttonClose->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  connect( buttonClose, SIGNAL( clicked() ), this, SLOT( slotClose() ) );
  
  // layout //
  
  // construct layout: base
  baseLayout->addWidget( infoFrame );
  baseLayout->addWidget( buttonFrame );
    
  // construct layout: info
  int row = 0;
  
  infoLayout->addWidget( fileNameHeader, row, 0 );
  infoLayout->addWidget( fileName, row, 1 );
  
  ++row;
  infoLayout->addWidget( localPathHeader, row, 0 );
  infoLayout->addWidget( m_localPath, row, 1 );
  
  ++row;
  infoLayout->addWidget( partnerHeader, row, 0 );
  infoLayout->addWidget( m_partner, row, 1 );
  
  ++row;
  infoLayout->addWidget( selfHeader, row, 0 );
  infoLayout->addWidget( m_self, row, 1 );
  
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
  
  // update //
  updateView();

  // make up //
  setMinimumWidth( 300 );
  setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed ) );
}

DccDetailDialog::~DccDetailDialog()
{
}

void DccDetailDialog::updateView()  // public
{
  // caption //
  
  if ( m_item->dccType == DccTransfer::Send )
    setCaption( i18n("DCC Send") + " : " + m_item->fileName );
  else
    setCaption( i18n("DCC Receive") + " : " + m_item->fileName );
  
  // information //
  
  // Partner
  QString partner( m_item->partnerNick );
  if ( !m_item->partnerIp.isEmpty() || !m_item->partnerPort.isEmpty() )
  {
    partner += " (";
    partner += !m_item->partnerIp.isEmpty() ? m_item->partnerIp : i18n("unknown");
    partner += ":";
    partner += !m_item->partnerPort.isEmpty() ? m_item->partnerPort : i18n("unknown");
    partner += ")";
  }
  m_partner->setText( partner );
  
  // Self
  QString self;
  if ( !m_item->ownIp.isEmpty() || !m_item->ownPort.isEmpty() )
  {
    self += !m_item->ownIp.isEmpty() ? m_item->ownIp : "* ";
    self += ":";
    self += !m_item->ownPort.isEmpty() ? m_item->ownPort : i18n("unknown");
  }
  else
    self = i18n("unknown");
  m_self->setText( self );
  
  // Status
  m_status->setText( m_item->getStatusText() );
  
  // Progress
  // FIXME: in case filesize is unknown
  m_progress->setProgress( (int)( 100 * m_item->transferringPosition / m_item->fileSize ) );
  
  // Position
  m_position->setText( m_item->getPositionPrettyText() );
  
  // Local path
  m_localPath->setURL( m_item->filePath );
  m_localPath->lineEdit()->setReadOnly( m_item->dccStatus != DccTransfer::Queued );
  m_localPath->button()->setEnabled( m_item->dccStatus == DccTransfer::Queued );
  
  // buttons //
  
  // Accept
  if ( m_buttonAccept )
    m_buttonAccept->setEnabled( m_item->dccStatus == DccTransfer::Queued );
  
  // Abort
  m_buttonAbort->setEnabled( m_item->dccStatus != DccTransfer::Failed &&
                             m_item->dccStatus != DccTransfer::Aborted &&
                             m_item->dccStatus != DccTransfer::Done );
  
}

void DccDetailDialog::slotLocalPathChanged( const QString& newFilePath )
{
  m_localPath->setURL( newFilePath.stripWhiteSpace() );
  m_item->setFilePath( newFilePath.stripWhiteSpace() );
}

void DccDetailDialog::slotAccept()
{
  m_item->start();
}

void DccDetailDialog::slotAbort()
{
  m_item->abort();
}

void DccDetailDialog::slotClose()
{
  accept();  // *not* mean accepting the DCC, but close this dialog
}

#include "dccdetaildialog.moc"
