/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dccpanel.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qhbox.h>
#include <qheader.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krun.h>

#include "dccpanel.h"
#include "dcctransfer.h"
#include "dcctransfersend.h"

#define USE_INFOLIST

#ifdef USE_MDI
DccPanel::DccPanel(QString caption) : ChatWindow(caption)
#else
DccPanel::DccPanel(QWidget* parent) : ChatWindow(parent)
#endif
{
  setType(ChatWindow::DccPanel);

  m_listView = new KListView(this,"dcc_control_panel");
  
  m_listView->setSelectionMode(QListView::Extended);
  m_listView->setDragEnabled(true);
  m_listView->setAcceptDrops(true);
  m_listView->setSorting(-1,false);
  m_listView->setAllColumnsShowFocus(true);
  
  for(unsigned int i=0 ; i < Column::COUNT ; ++i)
    m_listView->addColumn("");
  
  //m_listView->setColumnText(Column::TypeIcon,      "");
  m_listView->setColumnText(Column::OfferDate,     i18n("Date"));
  m_listView->setColumnText(Column::Status,        i18n("Status"));
  m_listView->setColumnText(Column::FileName,      i18n("File"));
  m_listView->setColumnText(Column::PartnerNick,   i18n("Partner"));
  m_listView->setColumnText(Column::Progress,      i18n("Progress"));
  m_listView->setColumnText(Column::Position,      i18n("Position"));
  m_listView->setColumnText(Column::TimeRemaining, i18n("Remain"));
  m_listView->setColumnText(Column::CPS,           i18n("Speed"));
  m_listView->setColumnText(Column::SenderAddress, i18n("Sender Address"));
  
  m_listView->setColumnWidth(Column::OfferDate,      70);
  m_listView->setColumnWidth(Column::Status,         80);
  m_listView->setColumnWidth(Column::FileName,      150);
  m_listView->setColumnWidth(Column::PartnerNick,    70);
  m_listView->setColumnWidth(Column::Progress,       90);
  m_listView->setColumnWidth(Column::Position,      120);
  m_listView->setColumnWidth(Column::TimeRemaining,  80);
  m_listView->setColumnWidth(Column::CPS,            70);
  
  m_listView->setColumnWidthMode(Column::FileName, QListView::Manual);
  
  m_listView->setColumnAlignment(Column::OfferDate,     AlignHCenter);
  m_listView->setColumnAlignment(Column::Progress,      AlignHCenter);
  m_listView->setColumnAlignment(Column::Position,      AlignHCenter);
  m_listView->setColumnAlignment(Column::TimeRemaining, AlignRight);
  m_listView->setColumnAlignment(Column::CPS,           AlignRight);
  
  m_listView->setSorting(Column::OfferDate, false);
  
  connect(m_listView,SIGNAL (selectionChanged()),this,SLOT (updateButton()) );
  
  // button
  
  QHBox* buttonsBox=new QHBox(this);
  buttonsBox->setSpacing(spacing());
  
  #define icon(s) KGlobal::iconLoader()->loadIcon( s, KIcon::Small )
  
  m_buttonAccept = new QPushButton(icon("player_play"), i18n("Accept"), buttonsBox, "start_dcc");
  m_buttonAbort  = new QPushButton(icon("stop"),        i18n("Abort"),  buttonsBox, "abort_dcc");
  m_buttonClear  = new QPushButton(icon("editdelete"),  i18n("Clear Item"),  buttonsBox, "clear_dcc");
  m_buttonOpen   = new QPushButton(icon("exec"),        i18n("Open File"),   buttonsBox, "open_dcc_file");
  m_buttonRemove = new QPushButton(icon("edittrash"),   i18n("Remove File"), buttonsBox, "remove_dcc_file");
  m_buttonDetail = new QPushButton(icon("view_text"),   i18n("DCC Detail"), buttonsBox, "detail_dcc");
  
  QToolTip::add( m_buttonAccept, i18n( "Start receiving" ) );
  QToolTip::add( m_buttonAbort,  i18n( "Abort the transfer(s)" ) );
  QToolTip::add( m_buttonClear,  i18n( "Remove from this panel" ) );
  QToolTip::add( m_buttonOpen,   i18n( "Run the file" ) );
  QToolTip::add( m_buttonRemove, i18n( "Remove the received file(s)" ) );
  QToolTip::add( m_buttonDetail, i18n( "View DCC detail information" ) );
  
  connect( m_buttonAccept, SIGNAL(clicked()), this, SLOT(acceptDcc()) );
  connect( m_buttonAbort,  SIGNAL(clicked()), this, SLOT(abortDcc()) );
  connect( m_buttonClear,  SIGNAL(clicked()), this, SLOT(clearDcc()) );
  connect( m_buttonOpen,   SIGNAL(clicked()), this, SLOT(runDcc()) );
  connect( m_buttonRemove, SIGNAL(clicked()), this, SLOT(removeFile()) );
  connect( m_buttonDetail, SIGNAL(clicked()), this, SLOT(openDetail()) );

  // popup menu
  
  m_popup = new KPopupMenu(this);
  m_popup->insertItem(                         i18n("Select All Items"),           Popup::SelectAll);
  m_popup->insertItem(                         i18n("Select All Completed Items"), Popup::SelectAllCompleted);
  m_popup->insertSeparator(); // -----
  m_popup->insertItem(icon("player_play"),     i18n("Accept"),                     Popup::Accept);
  m_popup->insertItem(icon("stop"),            i18n("Abort"),                      Popup::Abort);
  m_popup->insertSeparator(); // -----
  m_popup->insertItem(icon("editdelete"),      i18n("Clear Item"),                 Popup::Clear);
  m_popup->insertSeparator(); // -----
  m_popup->insertItem(icon("exec"),            i18n("Open File"),                  Popup::Open);
  m_popup->insertItem(icon("edittrash"),       i18n("Remove File"),                Popup::Remove);
  m_popup->insertItem(icon("messagebox_info"), i18n("File Information"),           Popup::Info);
  m_popup->insertSeparator(); // -----
  m_popup->insertItem(icon("view_text"),       i18n("DCC Detail Information"),     Popup::Detail);
    
  connect(m_listView, SIGNAL(contextMenuRequested(QListViewItem*,const QPoint&,int)), this, SLOT(popupRequested(QListViewItem*,const QPoint&,int)));
  connect(m_popup, SIGNAL(activated(int)), this, SLOT(popupActivated(int)));
  
  // misc.
  connect(m_listView, SIGNAL(doubleClicked(QListViewItem*,const QPoint&,int)), this, SLOT(doubleClicked(QListViewItem*,const QPoint&,int)));
  
  updateButton();
}

DccPanel::~DccPanel()
{
  kdDebug() << "DccPanel::~DccPanel()" << endl;
}

void DccPanel::dccStatusChanged(const DccTransfer* /* item */)
{
  updateButton();
}

void DccPanel::updateButton()
{
  bool accept             = true,
       abort              = false,
       clear              = false,
       detail             = true,
       info               = true,
       open               = true,
       remove             = true,
       selectAll          = false,
       selectAllCompleted = false;
  
  int selectedItems = 0;
  QListViewItemIterator it( m_listView );
  
  while( it.current() )
  {
    DccTransfer* item = static_cast<DccTransfer*>( it.current() );
    
    DccTransfer::DccType type = item->getType();
    DccTransfer::DccStatus status = item->getStatus();
    
    selectAll = true;
    selectAllCompleted |= ( status >= DccTransfer::Done );
    
    if( it.current()->isSelected() )
    {
      ++selectedItems;
      
      accept &= ( status == DccTransfer::Queued );
      
      abort  |= ( status < DccTransfer::Done );
      
      clear  |= ( status >= DccTransfer::Done );
      
      info   &= ( type == DccTransfer::Send ||
                  status == DccTransfer::Done );
      
      open   &= ( type == DccTransfer::Send ||
                  status == DccTransfer::Done );
      
      remove &= ( type == DccTransfer::Receive &&
                  status == DccTransfer::Done );
    }
    ++it;
  }
  
  if( !selectedItems )
  {
    accept = false;
    abort = false;
    clear = false;
    detail = false;
    info = false;
    open = false;
    remove = false;
  }
  
  m_buttonAccept->setEnabled( accept );
  m_buttonAbort->setEnabled( abort );
  m_buttonClear->setEnabled( clear );
  m_buttonOpen->setEnabled( open );
  m_buttonRemove->setEnabled( remove );
  m_buttonDetail->setEnabled( detail );
  
  m_popup->setItemEnabled( Popup::SelectAll,          selectAll );
  m_popup->setItemEnabled( Popup::SelectAllCompleted, selectAllCompleted );
  m_popup->setItemEnabled( Popup::Accept,             accept );
  m_popup->setItemEnabled( Popup::Abort,              abort );
  m_popup->setItemEnabled( Popup::Clear,              clear );
  m_popup->setItemEnabled( Popup::Open,               open );
  m_popup->setItemEnabled( Popup::Remove,             remove );
  m_popup->setItemEnabled( Popup::Info,               info );
  m_popup->setItemEnabled( Popup::Detail,             detail );
}

void DccPanel::selectMe(DccTransfer* item)
{
  m_listView->clearSelection();
  m_listView->setSelected(item, true);
  updateButton();
}

void DccPanel::acceptDcc()
{
  QListViewItemIterator it( m_listView );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item->getType() == DccTransfer::Receive && item->getStatus() == DccTransfer::Queued )
        item->start();
    }
    ++it;
  }
}

void DccPanel::abortDcc()
{
  QListViewItemIterator it( m_listView );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item->getStatus() < DccTransfer::Done )
        item->abort();
    }
    ++it;
  }
}

void DccPanel::clearDcc()
{
  QPtrList<QListViewItem> lst;
  QListViewItemIterator it( m_listView );
  while( it.current() )
  {
    DccTransfer* item = static_cast<DccTransfer*>( it.current() );
    // should we check that [item] is not null?
    if( it.current()->isSelected() && item->getStatus() >= DccTransfer::Done )
      lst.append( it.current() );
    ++it;
  }
  lst.setAutoDelete( true );
  while( lst.remove() );
  updateButton();
}

void DccPanel::runDcc()
{
  QListViewItemIterator it( m_listView );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item->getType() == DccTransfer::Send || item->getStatus() == DccTransfer::Done )
        item->runFile();
    }
    ++it;
  }
}

void DccPanel::removeFile()
{
  // count deletable files first
  int deletableFiles = 0;
  QListViewItemIterator it( m_listView );
  while ( it.current() )
  {
    if ( it.current()->isSelected() )
    {
      DccTransfer* item = static_cast<DccTransfer*>( it.current() );
      if( item->getType() == DccTransfer::Receive && item->getStatus() == DccTransfer::Done )
        ++deletableFiles;
    }
    ++it;
  }
  
  int ret = KMessageBox::warningContinueCancel( this,
                                                i18n( "Do you really want to remove the selected %1 file(s)?" ).arg( deletableFiles ),
                                                i18n( "Delete Confirmation" ),
                                                i18n( "&Delete" ),
                                                "RemoveDCCReceivedFile",
                                                KMessageBox::Dangerous
                                              );
  if ( ret == KMessageBox::Continue )
  {  
    QListViewItemIterator it = QListViewItemIterator( m_listView );
    while( it.current() )
    {
      if( it.current()->isSelected() )
      {
        DccTransfer* item=static_cast<DccTransfer*>( it.current() );
        if( item->getType() == DccTransfer::Receive && item->getStatus() == DccTransfer::Done )
          item->removeFile();
      }
      ++it;
    }
  } 
}

void DccPanel::showFileInfo()
{
  QListViewItemIterator it( m_listView );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item->getType() == DccTransfer::Send || item->getStatus() == DccTransfer::Done )
        item->openFileInfoDialog();
    }
    ++it;
  }
}

void DccPanel::openDetail()
{
  QListViewItemIterator it( m_listView );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      item->openDetailDialog();
    }
    ++it;
  }
}

void DccPanel::selectAll()
{
  QListViewItemIterator it( m_listView );
  while ( it.current() )
  {
    m_listView->setSelected( *it, true );
    ++it;
  }
  updateButton();
}

void DccPanel::selectAllCompleted()
{
  QListViewItemIterator it( m_listView );
  while ( it.current() )
  {
    DccTransfer* item=static_cast<DccTransfer*>( it.current() );
    m_listView->setSelected( *it, item->getStatus() >= DccTransfer::Done );
    ++it;
  }
  updateButton();
}

void DccPanel::popupRequested(QListViewItem* /* item */, const QPoint& pos, int /* col */)  // slot
{
  updateButton();
  m_popup->popup(pos);
}

void DccPanel::popupActivated( int id )  // slot
{
       if ( id == Popup::Abort )               abortDcc();
  else if ( id == Popup::Accept )              acceptDcc();
  else if ( id == Popup::Clear )               clearDcc();
  else if ( id == Popup::Detail )              openDetail();
  else if ( id == Popup::Info )                showFileInfo();
  else if ( id == Popup::Open )                runDcc();
  else if ( id == Popup::Remove )              removeFile();
  else if ( id == Popup::SelectAll )           selectAll();
  else if ( id == Popup::SelectAllCompleted )  selectAllCompleted();
}

void DccPanel::doubleClicked(QListViewItem* _item, const QPoint& /* _pos */, int /* _col */)
{
  DccTransfer* item = static_cast<DccTransfer*>(_item);
  if(item->getType() == DccTransfer::Send || item->getStatus() == DccTransfer::Done)
    new KRun( item->getFileURL() );
}

DccTransfer* DccPanel::getTransferByPort(const QString& port,DccTransfer::DccType type,bool resumed)
{
  int index=0;
  DccTransfer* item;
  do
  {
    // TODO: Get rid of this cast
    item=static_cast<DccTransfer*>(getListView()->itemAtIndex(index++));
    if(item)
    {
      if( (item->getStatus()==DccTransfer::Queued || item->getStatus()==DccTransfer::WaitingRemote) &&
         item->getType()==type &&
         !(resumed && !item->isResumed()) &&
         item->getOwnPort()==port) return item;
    }
  } while(item);

  return 0;
}

// To find the resuming dcc over firewalls that change the port numbers
DccTransfer* DccPanel::getTransferByName(const QString& name,DccTransfer::DccType type,bool resumed)
{
  int index=0;
  DccTransfer* item;
  do
  {
    // TODO: Get rid of this cast
    item=static_cast<DccTransfer*>(getListView()->itemAtIndex(index++));
    if(item)
    {
      if( (item->getStatus()==DccTransfer::Queued || item->getStatus()==DccTransfer::WaitingRemote) &&
         item->getType()==type &&
         !(resumed && !item->isResumed()) &&
         item->getFileName()==name) return item;
    }
  } while(item);

  return 0;
}

#ifdef USE_MDI
void DccPanel::closeYourself(ChatWindow*)
{
  emit chatWindowCloseRequest(this);
}
#endif

// virtual
void DccPanel::childAdjustFocus()
{
}

KListView* DccPanel::getListView() { return m_listView; }

#include "dccpanel.moc"
