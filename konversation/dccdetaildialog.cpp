// dccdetaildialog.cpp
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// See COPYING file for licensing information

#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>

#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <klineedit.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <kurlrequester.h>

#include "dccdetaildialog.h"
#include "dcctransfer.h"
#include "dcctransferrecv.h"

DccDetailDialog::DccDetailDialog( DccTransfer* item )
  : KDialog( 0 )
  , m_item( item )
{
  QVBoxLayout* baseLayout = new QVBoxLayout( this );
  baseLayout->setMargin( marginHint() );
  baseLayout->setSpacing( spacingHint() );
  
  // information
  
  QFrame* infoFrame = new QFrame( this );
  QGridLayout* infoLayout = new QGridLayout( infoFrame, 1, 2 );
  infoLayout->setSpacing( spacingHint() );
  
  // Filename
  QLabel* fileNameHeader = new QLabel( i18n("File"), infoFrame );
  fileNameHeader->setAlignment( AlignHCenter | AlignVCenter );
  KLineEdit* fileName = new KLineEdit( item->fileName, infoFrame );
  fileName->setFocusPolicy( ClickFocus );
  fileName->setReadOnly( true );
  fileName->setFrame( false );
  fileName->setAlignment( AlignHCenter );
  
  // Local File URL
  QLabel* localFileURLHeader = new QLabel( infoFrame );
  localFileURLHeader->setAlignment( AlignHCenter | AlignVCenter );
  if ( m_item->dccType == DccTransfer::Send )
    localFileURLHeader->setText( i18n("Local Path") );
  else
    localFileURLHeader->setText( i18n("Save to") );
  QHBox* localFileURLBox = new QHBox( infoFrame );
  localFileURLBox->setSpacing( spacingHint() );
  m_localFileURL = new KURLRequester( m_item->localFileURL.prettyURL(), localFileURLBox );
  connect( m_localFileURL, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotLocalFileURLChanged( const QString& ) ) );
  m_localFileURLOpen = new KPushButton( KGlobal::iconLoader()->loadIcon( "exec", KIcon::Small ), QString::null, localFileURLBox );
  m_localFileURLOpen->setFixedSize( m_localFileURL->button()->size() );
  connect( m_localFileURLOpen, SIGNAL( clicked() ), this, SLOT( slotOpenFile() ) );
  m_localFileURLRemove = new KPushButton( KGlobal::iconLoader()->loadIcon( "edittrash", KIcon::Small ), QString::null, localFileURLBox );
  m_localFileURLRemove->setFixedSize( m_localFileURL->button()->size() );
  connect( m_localFileURLRemove, SIGNAL( clicked() ), this, SLOT( slotRemoveFile() ) );
  m_localFileURLViewInfo = new KPushButton( KGlobal::iconLoader()->loadIcon( "messagebox_info", KIcon::Small ), QString::null, localFileURLBox );
  m_localFileURLViewInfo->setFixedSize( m_localFileURL->button()->size() );
  connect( m_localFileURLViewInfo, SIGNAL( clicked() ), this, SLOT( slotViewFileInfo() ) );
  
  // Partner
  QLabel* partnerHeader = new QLabel( infoFrame );
  partnerHeader->setAlignment( AlignHCenter | AlignVCenter );
  if ( m_item->dccType == DccTransfer::Send )
    partnerHeader->setText( i18n("Receiver") );
  else
    partnerHeader->setText( i18n("Sender") );
  m_partner = new KLineEdit( infoFrame );
  m_partner->setFocusPolicy( ClickFocus );
  m_partner->setReadOnly( true );
  m_partner->setFrame( false );
  m_partner->setAlignment( AlignHCenter );
    
  // Self
  QLabel* selfHeader = 0;
  m_self = 0;
  if ( m_item->dccType == DccTransfer::Send )
  {
    selfHeader = new QLabel( i18n("Self"), infoFrame );
    selfHeader->setAlignment( AlignHCenter | AlignVCenter );
    m_self = new KLineEdit( infoFrame );
    m_self->setFocusPolicy( ClickFocus );
    m_self->setReadOnly( true );
    m_self->setFrame( false );
    m_self->setAlignment( AlignHCenter );
  }
  
  // Status
  QLabel* statusHeader = new QLabel( i18n("Status"), infoFrame );
  statusHeader->setAlignment( AlignHCenter | AlignVCenter );
  m_status = new KLineEdit( infoFrame );
  m_status->setFocusPolicy( ClickFocus );
  m_status->setReadOnly( true );
  m_status->setFrame( false );
  m_status->setAlignment( AlignHCenter );
  
  // Progres
  QLabel* progressHeader = new QLabel( i18n("Progress"), infoFrame );
  progressHeader->setAlignment( AlignHCenter | AlignVCenter );
  m_progress = new KProgress( 100, infoFrame );
  m_progress->setCenterIndicator( true );
  
  // Position
  QLabel* positionHeader = new QLabel( i18n("Position"), infoFrame );
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
  if ( m_item->dccType == DccTransfer::Receive )
  {
    m_buttonAccept = new KPushButton( KGlobal::iconLoader()->loadIcon( "player_play", KIcon::Small ), i18n("Accept"), buttonFrame );
    m_buttonAccept->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    connect( m_buttonAccept, SIGNAL( clicked() ), this, SLOT( slotAccept() ) );
  }
  
  // Abort
  m_buttonAbort = new KPushButton( KGlobal::iconLoader()->loadIcon( "stop", KIcon::Small ), i18n("Abort"), buttonFrame );
  m_buttonAbort->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  connect( m_buttonAbort, SIGNAL( clicked() ), this, SLOT( slotAbort() ) );
  
  // Close
  KPushButton* buttonClose = new KPushButton( KGlobal::iconLoader()->loadIcon( "button_ok", KIcon::Small ), i18n("Close"), buttonFrame );
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

void DccDetailDialog::updateView()  // public
{
  // caption
  
  if ( m_item->dccType == DccTransfer::Send )
    setCaption( i18n("DCC Send") + " : " + m_item->fileName );
  else
    setCaption( i18n("DCC Receive") + " : " + m_item->fileName );
  
  // information
  
  // Local path
  m_localFileURL->setURL( m_item->localFileURL.prettyURL() );
  m_localFileURL->lineEdit()->setFocusPolicy( m_item->dccStatus == DccTransfer::Queued ? StrongFocus : ClickFocus );
  m_localFileURL->lineEdit()->setReadOnly( m_item->dccStatus != DccTransfer::Queued );
  m_localFileURL->lineEdit()->setFrame( m_item->dccStatus == DccTransfer::Queued );
  m_localFileURL->lineEdit()->setAlignment( m_item->dccStatus == DccTransfer::Queued ? AlignLeft : AlignHCenter );
  m_localFileURL->button()->setEnabled( m_item->dccStatus == DccTransfer::Queued );
  m_localFileURLOpen->setEnabled( m_item->dccType == DccTransfer::Send || m_item->dccStatus == DccTransfer::Done );
  m_localFileURLRemove->setEnabled( m_item->dccType == DccTransfer::Receive && m_item->dccStatus == DccTransfer::Done );
  m_localFileURLViewInfo->setEnabled( m_item->dccType == DccTransfer::Send || m_item->dccStatus == DccTransfer::Done );
  
  // Partner
  if ( !m_item->partnerIp.isEmpty() || !m_item->partnerPort.isEmpty() )
    m_partner->setText( QString( "%1 (%2:%3)" )
                       .arg( m_item->partnerNick )
                       .arg( !m_item->partnerIp.isEmpty() ? m_item->partnerIp : i18n("unknown") )
                       .arg( !m_item->partnerPort.isEmpty() ? m_item->partnerPort : i18n("unknown") )
                      );
  else
    m_partner->setText( m_item->partnerNick );
  
  // Self
  if ( m_self )
  {
    QString self;
    if ( !m_item->ownIp.isEmpty() || !m_item->ownPort.isEmpty() )
      m_self->setText( QString( "%1:%2" )
                      .arg( !m_item->ownIp.isEmpty() ? m_item->ownIp : "* " )
                      .arg( !m_item->ownPort.isEmpty() ? m_item->ownPort : i18n("unknown") )
                     );
    else
      m_self->setText( i18n("unknown") );
  }
  
  // Status
  m_status->setText( m_item->dccStatusDetail.isEmpty() ? m_item->getStatusText() : m_item->getStatusText() + " (" + m_item->dccStatusDetail + ")" );
  
  // Progress
  // FIXME: in case filesize is unknown
  m_progress->setProgress( (int)( 100 * m_item->transferringPosition / m_item->fileSize ) );
  
  // Position
  m_position->setText( m_item->getPositionPrettyText() );
  
  // buttons
  
  // Accept
  if ( m_buttonAccept )
    m_buttonAccept->setEnabled( m_item->dccStatus == DccTransfer::Queued );
  
  // Abort
  m_buttonAbort->setEnabled( m_item->dccStatus < DccTransfer::Done );
}

void DccDetailDialog::slotLocalFileURLChanged( const QString& newURL )
{
  DccTransferRecv* item = static_cast<DccTransferRecv*>( m_item );
  if ( item )
  {
    item->setLocalFileURL( KURL::fromPathOrURL( newURL ) );
    m_localFileURL->setURL( item->localFileURL.prettyURL() );
  }
}

void DccDetailDialog::slotOpenFile()
{
  m_item->runFile();
}

void DccDetailDialog::slotRemoveFile()
{
  m_item->removeFile();
}

void DccDetailDialog::slotViewFileInfo()
{
  m_item->openFileInfoDialog();
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
  accept();  // *not* mean accepting DCC, but closing this dialog
}

#include "dccdetaildialog.moc"
