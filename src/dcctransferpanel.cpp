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
// Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>

#include <qhbox.h>
#include <qheader.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kapplication.h>

#include "konversationapplication.h"
#include "dcctransferdetailedinfopanel.h"
#include "dcctransfermanager.h"
#include "dcctransferpanel.h"
#include "dcctransferpanelitem.h"
#include "dcctransfersend.h"

DccTransferPanel::DccTransferPanel(QWidget* parent) : ChatWindow(parent)
{
    kdDebug() << "DccTransferPanel::DccTransferPanel()" << endl;

    setType(ChatWindow::DccTransferPanel);
    setName(i18n("DCC Status"));

    initGUI();

    connect( KonversationApplication::instance()->dccTransferManager(), SIGNAL( newTransferAdded( DccTransfer* ) ), this, SLOT( slotNewTransferAdded( DccTransfer* ) ) );
}

DccTransferPanel::~DccTransferPanel()
{
    kdDebug() << "DccTransferPanel::~DccTransferPanel()" << endl;
}

void DccTransferPanel::initGUI()
{
    setSpacing( 0 );

    m_listView = new KListView(this,"dcc_control_panel");

    m_listView->setSelectionMode(QListView::Extended);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setSorting(-1,false);
    m_listView->setAllColumnsShowFocus(true);

    for(unsigned int i=0 ; i < Column::COUNT ; ++i)
        m_listView->addColumn("");

    //m_listView->setColumnText(Column::TypeIcon,      "");
    m_listView->setColumnText(Column::OfferDate,     i18n("Started at"));
    m_listView->setColumnText(Column::Status,        i18n("Status"));
    m_listView->setColumnText(Column::FileName,      i18n("File"));
    m_listView->setColumnText(Column::PartnerNick,   i18n("Partner"));
    m_listView->setColumnText(Column::Progress,      i18n("Progress"));
    m_listView->setColumnText(Column::Position,      i18n("Position"));
    m_listView->setColumnText(Column::TimeLeft,      i18n("Remaining"));
    m_listView->setColumnText(Column::CurrentSpeed,  i18n("Speed"));
    m_listView->setColumnText(Column::SenderAddress, i18n("Sender Address"));

    m_listView->setColumnWidth(Column::TypeIcon,       16);
    m_listView->setColumnWidth(Column::OfferDate,      90);
    m_listView->setColumnWidth(Column::Status,         80);
    m_listView->setColumnWidth(Column::FileName,      150);
    m_listView->setColumnWidth(Column::PartnerNick,    70);
    m_listView->setColumnWidth(Column::Progress,       90);
    m_listView->setColumnWidth(Column::Position,      120);
    m_listView->setColumnWidth(Column::TimeLeft,       80);
    m_listView->setColumnWidth(Column::CurrentSpeed,   70);
    m_listView->setColumnWidth(Column::SenderAddress, 120);

    m_listView->setColumnWidthMode(Column::FileName, QListView::Manual);

    m_listView->setColumnAlignment(Column::OfferDate,     AlignHCenter);
    m_listView->setColumnAlignment(Column::Progress,      AlignHCenter);
    m_listView->setColumnAlignment(Column::Position,      AlignHCenter);
    m_listView->setColumnAlignment(Column::TimeLeft,      AlignHCenter);
    m_listView->setColumnAlignment(Column::CurrentSpeed,  AlignHCenter);

    m_listView->setSorting(Column::OfferDate, false);

    connect(m_listView,SIGNAL (selectionChanged()),this,SLOT (updateButton()) );

    // detailed info panel
    m_detailPanel = new DccTransferDetailedInfoPanel(this);

    // button

    QHBox* buttonsBox=new QHBox(this);
    buttonsBox->setSpacing(spacing());

    // convenience, undeffed below again to avoid name clashes
    #define icon(s) KGlobal::iconLoader()->loadIconSet( s, KIcon::Small )

    m_buttonAccept = new QPushButton(icon("player_play"), i18n("Accept"), buttonsBox, "start_dcc");
    m_buttonAbort  = new QPushButton(icon("stop"),        i18n("Abort"),  buttonsBox, "abort_dcc");
    m_buttonClear  = new QPushButton(icon("editdelete"),  i18n("Clear"),  buttonsBox, "clear_dcc");
    m_buttonOpen   = new QPushButton(icon("exec"),        i18n("Open File"),   buttonsBox, "open_dcc_file");
    m_buttonRemove = new QPushButton(icon("edittrash"),   i18n("Remove File"), buttonsBox, "remove_dcc_file");
    m_buttonDetail = new QPushButton(icon("view_text"),   i18n("Transfer Details"), buttonsBox, "detail_dcc");
    m_buttonDetail->setToggleButton( true );

    QToolTip::add( m_buttonAccept, i18n( "Start receiving" ) );
    QToolTip::add( m_buttonAbort,  i18n( "Abort the transfer(s)" ) );
    QToolTip::add( m_buttonClear,  i18n( "Remove from this panel" ) );
    QToolTip::add( m_buttonOpen,   i18n( "Run the file" ) );
    QToolTip::add( m_buttonRemove, i18n( "Remove the received file(s)" ) );
    QToolTip::add( m_buttonDetail, i18n( "View DCC transfer details" ) );

    connect( m_buttonAccept, SIGNAL(clicked()), this, SLOT(acceptDcc()) );
    connect( m_buttonAbort,  SIGNAL(clicked()), this, SLOT(abortDcc()) );
    connect( m_buttonClear,  SIGNAL(clicked()), this, SLOT(clearDcc()) );
    connect( m_buttonOpen,   SIGNAL(clicked()), this, SLOT(runDcc()) );
    connect( m_buttonRemove, SIGNAL(clicked()), this, SLOT(removeFile()) );
    //connect( m_buttonDetail, SIGNAL(clicked()), this, SLOT(openDetail()) );
    connect( m_buttonDetail, SIGNAL(toggled(bool)), m_detailPanel, SLOT(setShown(bool)) );
    m_buttonDetail->setOn(true);


    // popup menu

    m_popup = new KPopupMenu(this);
    m_popup->insertItem(                         i18n("&Select All Items"),           Popup::SelectAll);
    m_popup->insertItem(                         i18n("S&elect All Completed Items"), Popup::SelectAllCompleted);
    m_popup->insertSeparator();                   // -----
    m_popup->insertItem(icon("player_play"),     i18n("&Accept"),                     Popup::Accept);
    m_popup->insertItem(icon("stop"),            i18n("A&bort"),                      Popup::Abort);
    m_popup->insertSeparator();                   // -----
    m_popup->insertItem(icon("editdelete"),      i18n("&Clear"),                      Popup::Clear);
    m_popup->insertSeparator();                   // -----
    m_popup->insertItem(icon("exec"),            i18n("&Open File"),                  Popup::Open);
    m_popup->insertItem(icon("edittrash"),       i18n("&Remove File"),                Popup::Remove);
    m_popup->insertItem(icon("messagebox_info"), i18n("File &Information"),           Popup::Info);

    #undef icon

    connect(m_listView, SIGNAL(contextMenuRequested(QListViewItem*,const QPoint&,int)), this, SLOT(popupRequested(QListViewItem*,const QPoint&,int)));
    connect(m_popup, SIGNAL(activated(int)), this, SLOT(popupActivated(int)));

    // misc.
    connect(m_listView, SIGNAL(doubleClicked(QListViewItem*,const QPoint&,int)), this, SLOT(doubleClicked(QListViewItem*,const QPoint&,int)));

    connect(m_listView, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(setDetailPanelItem(QListViewItem*)));

    updateButton();
}

void DccTransferPanel::slotNewTransferAdded( DccTransfer* transfer )
{
    DccTransferPanelItem* item = new DccTransferPanelItem( this, transfer );
    connect( transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotTransferStatusChanged() ) );
    m_listView->clearSelection();
    m_listView->setSelected( item, true );
    updateButton();
}

void DccTransferPanel::slotTransferStatusChanged()
{
    updateButton();
    activateTabNotification(Konversation::tnfSystem);
}

void DccTransferPanel::updateButton()
{
    bool accept             = true,
         abort              = false,
         clear              = false,
         info               = true,
         open               = true,
         remove             = true,
         selectAll          = false,
         selectAllCompleted = false;

    int selectedItems = 0;
    QListViewItemIterator it( m_listView );

    while( it.current() )
    {
        DccTransferPanelItem* item = static_cast<DccTransferPanelItem*>( it.current() );

        DccTransfer::DccType type = item->transfer()->getType();
        DccTransfer::DccStatus status = item->transfer()->getStatus();

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
        info = false;
        open = false;
        remove = false;
    }

    if (!kapp->authorize("allow_downloading"))
    {
        accept = false;
    }

    m_buttonAccept->setEnabled( accept );
    m_buttonAbort->setEnabled( abort );
    m_buttonClear->setEnabled( clear );
    m_buttonOpen->setEnabled( open );
    m_buttonRemove->setEnabled( remove );

    m_popup->setItemEnabled( Popup::SelectAll,          selectAll );
    m_popup->setItemEnabled( Popup::SelectAllCompleted, selectAllCompleted );
    m_popup->setItemEnabled( Popup::Accept,             accept );
    m_popup->setItemEnabled( Popup::Abort,              abort );
    m_popup->setItemEnabled( Popup::Clear,              clear );
    m_popup->setItemEnabled( Popup::Open,               open );
    m_popup->setItemEnabled( Popup::Remove,             remove );
    m_popup->setItemEnabled( Popup::Info,               info );
}

void DccTransferPanel::setDetailPanelItem(QListViewItem* item_)
{
    if ( item_ )
    {
        DccTransferPanelItem* item = static_cast< DccTransferPanelItem* >( item_ );
        m_detailPanel->setItem( item );
    }
}

void DccTransferPanel::acceptDcc()
{
    QListViewItemIterator it( m_listView );
    while( it.current() )
    {
        if( it.current()->isSelected() )
        {
            DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
            DccTransfer* transfer = item->transfer();
            if( transfer->getType() == DccTransfer::Receive && transfer->getStatus() == DccTransfer::Queued )
                transfer->start();
        }
        ++it;
    }
}

void DccTransferPanel::abortDcc()
{
    QListViewItemIterator it( m_listView );
    while( it.current() )
    {
        if( it.current()->isSelected() )
        {
            DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
            DccTransfer* transfer = item->transfer();
            if( transfer->getStatus() < DccTransfer::Done )
                transfer->abort();
        }
        ++it;
    }
}

void DccTransferPanel::clearDcc()
{
    QPtrList<QListViewItem> lst;
    QListViewItemIterator it( m_listView );
    while( it.current() )
    {
        DccTransferPanelItem* item = static_cast<DccTransferPanelItem*>( it.current() );
        // should we check that [item] is not null?
        if( it.current()->isSelected() && item->transfer()->getStatus() >= DccTransfer::Done )
            lst.append( it.current() );
        ++it;
    }

    // Figure out the first 'gap' in the selection and select that item, 
    // or, if there are no gaps, select first item below the selection
    QPtrListIterator<QListViewItem> selected( lst );
    bool itemSelected = false;
    while( selected.current() )
    {
        if (selected.current()->itemBelow() && !lst.containsRef(selected.current()->itemBelow()))
        {
            m_listView->setSelected(selected.current()->itemBelow(),true);
            m_listView->setCurrentItem(selected.current()->itemBelow());
            itemSelected = true;
            break; 
        }
        ++selected;
    }

    // When there are neither gaps in nor items below the selection, select the first item
    if (!itemSelected)
    {
        m_listView->setSelected(m_listView->firstChild(),true);
        m_listView->setCurrentItem(m_listView->firstChild());
    }

    lst.setAutoDelete( true );
    while( lst.remove() );
    updateButton();
}

void DccTransferPanel::runDcc()
{
    QListViewItemIterator it( m_listView );
    while( it.current() )
    {
        if( it.current()->isSelected() )
        {
            DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
            DccTransfer* transfer = item->transfer();
            if( transfer->getType() == DccTransfer::Send || transfer->getStatus() == DccTransfer::Done )
                item->runFile();
        }
        ++it;
    }
}

void DccTransferPanel::removeFile()
{
    // count deletable files first
    int deletableFiles = 0;
    QListViewItemIterator it( m_listView );
    while ( it.current() )
    {
        if ( it.current()->isSelected() )
        {
            DccTransferPanelItem* item = static_cast<DccTransferPanelItem*>( it.current() );
            DccTransfer* transfer = item->transfer();
            if( transfer->getType() == DccTransfer::Receive && transfer->getStatus() == DccTransfer::Done )
                ++deletableFiles;
        }
        ++it;
    }

    int ret = KMessageBox::warningContinueCancel( this,
        i18n( "Do you really want to remove the selected file?", "Do you really want to remove the selected %n files?", deletableFiles ),
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
                DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
                DccTransfer* transfer = item->transfer();
                if( transfer->getType() == DccTransfer::Receive && transfer->getStatus() == DccTransfer::Done )
                    item->removeFile();
            }
            ++it;
        }
    }
}

void DccTransferPanel::showFileInfo()
{
    QListViewItemIterator it( m_listView );
    while( it.current() )
    {
        if( it.current()->isSelected() )
        {
            DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
            if( item->transfer()->getType() == DccTransfer::Send || item->transfer()->getStatus() == DccTransfer::Done )
                item->openFileInfoDialog();
        }
        ++it;
    }
}

void DccTransferPanel::selectAll()
{
    QListViewItemIterator it( m_listView );
    while ( it.current() )
    {
        m_listView->setSelected( *it, true );
        ++it;
    }
    updateButton();
}

void DccTransferPanel::selectAllCompleted()
{
    QListViewItemIterator it( m_listView );
    while ( it.current() )
    {
        DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
        m_listView->setSelected( *it, item->transfer()->getStatus() >= DccTransfer::Done );
        ++it;
    }
    updateButton();
}

void DccTransferPanel::popupRequested(QListViewItem* /* item */, const QPoint& pos, int /* col */)  // slot
{
    updateButton();
    m_popup->popup(pos);
}

void DccTransferPanel::popupActivated( int id ) // slot
{
    if ( id == Popup::Abort )                    abortDcc();
    else if ( id == Popup::Accept )              acceptDcc();
    else if ( id == Popup::Clear )               clearDcc();
    else if ( id == Popup::Info )                showFileInfo();
    else if ( id == Popup::Open )                runDcc();
    else if ( id == Popup::Remove )              removeFile();
    else if ( id == Popup::SelectAll )           selectAll();
    else if ( id == Popup::SelectAllCompleted )  selectAllCompleted();
}

void DccTransferPanel::doubleClicked(QListViewItem* _item, const QPoint& /* _pos */, int /* _col */)
{
    DccTransferPanelItem* item = static_cast<DccTransferPanelItem*>(_item);
    item->runFile();
}

// virtual
void DccTransferPanel::childAdjustFocus()
{
}

KListView* DccTransferPanel::getListView() 
{ 
  return m_listView; 
}

#include "dcctransferpanel.moc"
