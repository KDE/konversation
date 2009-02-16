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
// Copyright (C) 2004-2008 Shintaro Matsuoka <shin@shoegazed.org>

#include "transferpanel.h"
#include "application.h"
#include "transferdetailedinfopanel.h"
#include "transfermanager.h"
#include "transferpanelitem.h"
#include "transfersend.h"
#include "preferences.h"


#include <q3header.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <k3listview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <krun.h>
#include <kapplication.h>
#include <kauthorized.h>
#include <kvbox.h>


DccTransferPanel::DccTransferPanel(QWidget* parent) : ChatWindow(parent)
{
    setType(ChatWindow::DccTransferPanel);
    setName(i18n("DCC Status"));

    initGUI();

    connect( KonversationApplication::instance()->getDccTransferManager(), SIGNAL( newTransferAdded( DccTransfer* ) ), this, SLOT( slotNewTransferAdded( DccTransfer* ) ) );
}

DccTransferPanel::~DccTransferPanel()
{
    // remember column widths
    QList<int> columnWidths;
    for ( uint i = 0 ; i < Column::COUNT ; ++i )
        columnWidths.push_back( m_listView->columnWidth( i ) );
    Preferences::self()->setDccColumnWidths( columnWidths );
}

void DccTransferPanel::initGUI()
{
    setSpacing( 0 );

    m_listView = new K3ListView(this);

    m_listView->setSelectionMode(Q3ListView::Extended);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setSorting(-1,false);
    m_listView->setAllColumnsShowFocus(true);

    for(unsigned int i=0 ; i < Column::COUNT ; ++i)
        m_listView->addColumn("");

    //m_listView->setColumnText(Column::TypeIcon,      "");
    m_listView->setColumnText(Column::OfferDate,     i18n("Started At"));
    m_listView->setColumnText(Column::Status,        i18n("Status"));
    m_listView->setColumnText(Column::FileName,      i18n("File"));
    m_listView->setColumnText(Column::PartnerNick,   i18n("Partner"));
    m_listView->setColumnText(Column::Progress,      i18n("Progress"));
    m_listView->setColumnText(Column::Position,      i18n("Position"));
    m_listView->setColumnText(Column::TimeLeft,      i18n("Remaining"));
    m_listView->setColumnText(Column::CurrentSpeed,  i18n("Speed"));
    m_listView->setColumnText(Column::SenderAddress, i18n("Sender Address"));

    QList<int> columnWidths = Preferences::self()->dccColumnWidths();
    for ( int i = 0 ; i < Column::COUNT && i < columnWidths.count() ; ++i )
        m_listView->setColumnWidth( i, columnWidths[i] );

    m_listView->setColumnWidthMode(Column::FileName, Q3ListView::Manual);

    m_listView->setColumnAlignment(Column::OfferDate,     Qt::AlignHCenter);
    m_listView->setColumnAlignment(Column::Progress,      Qt::AlignHCenter);
    m_listView->setColumnAlignment(Column::Position,      Qt::AlignHCenter);
    m_listView->setColumnAlignment(Column::TimeLeft,      Qt::AlignHCenter);
    m_listView->setColumnAlignment(Column::CurrentSpeed,  Qt::AlignHCenter);

    m_listView->setSorting(Column::OfferDate, false);

    connect(m_listView,SIGNAL (selectionChanged()),this,SLOT (updateButton()) );

    // detailed info panel
    m_detailPanel = new DccTransferDetailedInfoPanel(this);

    // button

    KHBox* buttonsBox=new KHBox(this);
    buttonsBox->setSpacing(spacing());

    m_buttonAccept = new QPushButton(KIcon("media-playback-start"), i18n("Accept"),    buttonsBox);
    m_buttonAccept->setObjectName("start_dcc");
    m_buttonAbort  = new QPushButton(KIcon("process-stop"),                  i18n("Abort"),     buttonsBox);
    m_buttonAbort->setObjectName("abort_dcc");
    m_buttonClear  = new QPushButton(KIcon("edit-delete"),            i18n("Clear"),     buttonsBox);
    m_buttonClear->setObjectName("clear_dcc");
    m_buttonOpen   = new QPushButton(KIcon("system-run"),           i18n("Open File"), buttonsBox);
    m_buttonOpen->setObjectName("open_dcc_file");
    m_buttonDetail = new QPushButton(KIcon("dialog-information"),   i18n("Details"),   buttonsBox);
    m_buttonDetail->setObjectName("detail_dcc");
    m_buttonDetail->setCheckable(true);

    m_buttonAccept->setStatusTip(i18n("Start receiving"));
    m_buttonAbort->setStatusTip(i18n("Abort the transfer(s)"));
    m_buttonOpen->setStatusTip(i18n("Run the file"));
    m_buttonDetail->setStatusTip(i18n("View DCC transfer details"));

    connect( m_buttonAccept, SIGNAL(clicked()), this, SLOT(acceptDcc()) );
    connect( m_buttonAbort,  SIGNAL(clicked()), this, SLOT(abortDcc()) );
    connect( m_buttonClear,  SIGNAL(clicked()), this, SLOT(clearDcc()) );
    connect( m_buttonOpen,   SIGNAL(clicked()), this, SLOT(runDcc()) );
    //connect( m_buttonDetail, SIGNAL(clicked()), this, SLOT(openDetail()) );
    connect( m_buttonDetail, SIGNAL(toggled(bool)), m_detailPanel, SLOT(setVisible(bool)) );
    m_buttonDetail->setChecked(true);


    // popup menu

    m_popup = new KMenu(this);
    m_selectAll =  m_popup->addAction(i18n("&Select All Items"));
    m_selectAllCompleted = m_popup->addAction(i18n("S&elect All Completed Items"));
    m_popup->addSeparator();                           // -----
    m_accept =  m_popup->addAction(KIcon("media-playback-start"), i18n("&Accept"));
    m_abort = m_popup->addAction(KIcon("process-stop"),i18n("A&bort"));
    m_popup->addSeparator();                           // -----
    // FIXME: make it neat
    m_resend = m_popup->addAction(KIcon("edit-redo"),i18n("Resend"));
    m_clear = m_popup->addAction(KIcon("edit-delete"),i18n("&Clear"));
    m_popup->addSeparator();                           // -----
    m_open = m_popup->addAction(KIcon("system-run"),i18n("&Open File"));
    m_info = m_popup->addAction(KIcon("dialog-information"),i18n("File &Information"));

    connect(m_listView, SIGNAL(contextMenuRequested(Q3ListViewItem*,const QPoint&,int)), this, SLOT(popupRequested(Q3ListViewItem*,const QPoint&,int)));
    connect(m_popup, SIGNAL(triggered ( QAction *)), this, SLOT(popupActivated(QAction*)));

    // misc.
    connect(m_listView, SIGNAL(doubleClicked(Q3ListViewItem*,const QPoint&,int)), this, SLOT(doubleClicked(Q3ListViewItem*,const QPoint&,int)));

    connect(m_listView, SIGNAL(currentChanged(Q3ListViewItem*)), this, SLOT(setDetailPanelItem(Q3ListViewItem*)));

    updateButton();
}

void DccTransferPanel::slotNewTransferAdded( DccTransfer* transfer )
{
    DccTransferPanelItem* item = new DccTransferPanelItem( this, transfer );
    connect( transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotTransferStatusChanged() ) );
    if ( m_listView->childCount() == 1 )
    {
        m_listView->clearSelection();
        m_listView->setSelected( item, true );
        m_listView->setCurrentItem( item );
        updateButton();
        setDetailPanelItem( item );
    }
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
         resend             = false,
         selectAll          = false,
         selectAllCompleted = false;

    int selectedItems = 0;
    Q3ListViewItemIterator it( m_listView );

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

            resend |= ( type == DccTransfer::Send &&
                status >= DccTransfer::Done );
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
        resend = false;
    }

    if (!KAuthorized::authorizeKAction("allow_downloading"))
    {
        accept = false;
    }

    m_buttonAccept->setEnabled( accept );
    m_buttonAbort->setEnabled( abort );
    m_buttonClear->setEnabled( clear );
    m_buttonOpen->setEnabled( open );

    m_selectAll->setEnabled( selectAll );
    m_selectAllCompleted->setEnabled(selectAllCompleted );
    m_accept->setEnabled(accept );
    m_abort->setEnabled( abort );
    m_clear->setEnabled(clear );
    m_open->setEnabled( open );
    m_resend->setEnabled(resend );
    m_info->setEnabled(info );
}

void DccTransferPanel::setDetailPanelItem(Q3ListViewItem* item_)
{
    if ( item_ )
    {
        DccTransferPanelItem* item = static_cast< DccTransferPanelItem* >( item_ );
        m_detailPanel->setItem( item );
    }
}

void DccTransferPanel::acceptDcc()
{
    Q3ListViewItemIterator it( m_listView );
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
    Q3ListViewItemIterator it( m_listView );
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

void DccTransferPanel::resendFile()
{
    Q3ListViewItemIterator it( m_listView );
    while( it.current() )
    {
        if( it.current()->isSelected() )
        {
            DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
            DccTransfer* transfer = item->transfer();
            if( transfer->getType() == DccTransfer::Send && transfer->getStatus() >= DccTransfer::Done )
            {
                DccTransferSend* newTransfer = KonversationApplication::instance()->getDccTransferManager()->newUpload();

                newTransfer->setConnectionId( transfer->getConnectionId() );
                newTransfer->setPartnerNick( transfer->getPartnerNick() );
                newTransfer->setFileURL( transfer->getFileURL() );
                newTransfer->setFileName( transfer->getFileName() );
                // FIXME
                newTransfer->setOwnIp( transfer->getOwnIp() );

                if ( newTransfer->queue() )
                    newTransfer->start();
            }
        }
        ++it;
    }
}

void DccTransferPanel::clearDcc()
{
    Q3PtrList<Q3ListViewItem> lst;
    Q3ListViewItemIterator it( m_listView );
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
    Q3PtrListIterator<Q3ListViewItem> selected( lst );
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
    while( lst.remove() ) ;
    updateButton();
}

void DccTransferPanel::runDcc()
{
    Q3ListViewItemIterator it( m_listView );
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

void DccTransferPanel::showFileInfo()
{
    Q3ListViewItemIterator it( m_listView );
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
    Q3ListViewItemIterator it( m_listView );
    while ( it.current() )
    {
        m_listView->setSelected( *it, true );
        ++it;
    }
    updateButton();
}

void DccTransferPanel::selectAllCompleted()
{
    Q3ListViewItemIterator it( m_listView );
    while ( it.current() )
    {
        DccTransferPanelItem* item=static_cast<DccTransferPanelItem*>( it.current() );
        m_listView->setSelected( *it, item->transfer()->getStatus() >= DccTransfer::Done );
        ++it;
    }
    updateButton();
}

void DccTransferPanel::popupRequested(Q3ListViewItem* /* item */, const QPoint& pos, int /* col */)  // slot
{
    updateButton();
    m_popup->popup(pos);
}

void DccTransferPanel::popupActivated( QAction * act ) // slot
{
    if ( act == m_abort )
        abortDcc();
    else if ( act == m_accept )
        acceptDcc();
    else if ( act == m_clear )
        clearDcc();
    else if ( act == m_info )
        showFileInfo();
    else if ( act == m_open )
        runDcc();
    else if ( act == m_selectAll )
        selectAll();
    else if ( act == m_selectAllCompleted )
        selectAllCompleted();
    else if ( act == m_resend )
        resendFile();
}

void DccTransferPanel::doubleClicked(Q3ListViewItem* _item, const QPoint& /* _pos */, int /* _col */)
{
    DccTransferPanelItem* item = static_cast<DccTransferPanelItem*>(_item);
    item->runFile();
}

// virtual
void DccTransferPanel::childAdjustFocus()
{
}

K3ListView* DccTransferPanel::getListView()
{
  return m_listView;
}

#include "transferpanel.moc"
