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

#include <QPushButton>

#include <Q3Header>

#include <KDialog>
#include <KGlobal>
#include <KGlobalSettings>
#include <KIconLoader>
#include <KMessageBox>
#include <KMenu>
#include <KRun>
#include <KAuthorized>
#include <KVBox>

#include <K3ListView>

namespace Konversation
{
    namespace DCC
    {
        TransferPanel::TransferPanel(QWidget* parent) : ChatWindow(parent)
        {
            setType(ChatWindow::DccTransferPanel);
            setName(i18n("DCC Status"));

            initGUI();

            connect( Application::instance()->getDccTransferManager(), SIGNAL( newTransferAdded( Konversation::DCC::Transfer* ) ), this, SLOT( slotNewTransferAdded( Konversation::DCC::Transfer* ) ) );
        }

        TransferPanel::~TransferPanel()
        {
            // remember column widths
            QList<int> columnWidths;
            for ( uint i = 0 ; i < Column::COUNT ; ++i )
                columnWidths.push_back( m_listView->columnWidth( i ) );
            Preferences::self()->setDccColumnWidths( columnWidths );
        }

        void TransferPanel::initGUI()
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
            m_detailPanel = new TransferDetailedInfoPanel(this);

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
            m_buttonOpenLocation = new QPushButton(KIcon("document-open-folder"), i18n("Open Location"), buttonsBox);
            m_buttonOpenLocation->setObjectName("open_dcc_file_location");
            m_buttonDetail = new QPushButton(KIcon("dialog-information"),   i18n("Details"),   buttonsBox);
            m_buttonDetail->setObjectName("detail_dcc");
            m_buttonDetail->setCheckable(true);

            m_buttonAccept->setStatusTip(i18n("Start receiving"));
            m_buttonAbort->setStatusTip(i18n("Abort the transfer(s)"));
            m_buttonOpen->setStatusTip(i18n("Run the file"));
            m_buttonOpenLocation->setStatusTip(i18n("Open the file location"));
            m_buttonDetail->setStatusTip(i18n("View DCC transfer details"));

            connect( m_buttonAccept, SIGNAL(clicked()), this, SLOT(acceptDcc()) );
            connect( m_buttonAbort,  SIGNAL(clicked()), this, SLOT(abortDcc()) );
            connect( m_buttonClear,  SIGNAL(clicked()), this, SLOT(clearDcc()) );
            connect( m_buttonOpen,   SIGNAL(clicked()), this, SLOT(runDcc()) );
            connect( m_buttonOpenLocation, SIGNAL(clicked()), this, SLOT(openLocation()));
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

        void TransferPanel::slotNewTransferAdded( Transfer* transfer )
        {
            TransferPanelItem* item = new TransferPanelItem( this, transfer );
            connect( transfer, SIGNAL( statusChanged( Konversation::DCC::Transfer*, int, int ) ), this, SLOT( slotTransferStatusChanged() ) );
            if ( m_listView->childCount() == 1 )
            {
                m_listView->clearSelection();
                m_listView->setSelected( item, true );
                m_listView->setCurrentItem( item );
                updateButton();
                setDetailPanelItem( item );
            }
        }

        void TransferPanel::slotTransferStatusChanged()
        {
            updateButton();
            activateTabNotification(Konversation::tnfSystem);
        }

        void TransferPanel::updateButton()
        {
            bool accept             = true,
                 abort              = false,
                 clear              = false,
                 info               = true,
                 open               = true,
                 openLocation       = false,
                 resend             = false,
                 selectAll          = false,
                 selectAllCompleted = false;

            int selectedItems = 0;
            Q3ListViewItemIterator it( m_listView );

            while( it.current() )
            {
                TransferPanelItem* item = static_cast<TransferPanelItem*>( it.current() );

                Transfer::Type type = item->transfer()->getType();
                Transfer::Status status = item->transfer()->getStatus();

                selectAll = true;
                selectAllCompleted |= ( status >= Transfer::Done );

                if( it.current()->isSelected() )
                {
                    ++selectedItems;

                    accept &= ( status == Transfer::Queued );

                    abort  |= ( status < Transfer::Done );

                    clear  |= ( status >= Transfer::Done );

                    info   &= ( type == Transfer::Send ||
                        status == Transfer::Done );

                    open   &= ( type == Transfer::Send ||
                        status == Transfer::Done );

                    openLocation = true;

                    resend |= ( type == Transfer::Send &&
                        status >= Transfer::Done );
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
            m_buttonOpenLocation->setEnabled( openLocation );

            m_selectAll->setEnabled( selectAll );
            m_selectAllCompleted->setEnabled(selectAllCompleted );
            m_accept->setEnabled(accept );
            m_abort->setEnabled( abort );
            m_clear->setEnabled(clear );
            m_open->setEnabled( open );
            m_resend->setEnabled(resend );
            m_info->setEnabled(info );
        }

        void TransferPanel::setDetailPanelItem(Q3ListViewItem* item_)
        {
            if ( item_ )
            {
                TransferPanelItem* item = static_cast< TransferPanelItem* >( item_ );
                m_detailPanel->setItem( item );
            }
        }

        void TransferPanel::acceptDcc()
        {
            Q3ListViewItemIterator it( m_listView );
            while( it.current() )
            {
                if( it.current()->isSelected() )
                {
                    TransferPanelItem* item=static_cast<TransferPanelItem*>( it.current() );
                    Transfer* transfer = item->transfer();
                    if( transfer->getType() == Transfer::Receive && transfer->getStatus() == Transfer::Queued )
                        transfer->start();
                }
                ++it;
            }
        }

        void TransferPanel::abortDcc()
        {
            Q3ListViewItemIterator it( m_listView );
            while( it.current() )
            {
                if( it.current()->isSelected() )
                {
                    TransferPanelItem* item=static_cast<TransferPanelItem*>( it.current() );
                    Transfer* transfer = item->transfer();
                    if( transfer->getStatus() < Transfer::Done )
                        transfer->abort();
                }
                ++it;
            }
        }

        void TransferPanel::resendFile()
        {
            Q3ListViewItemIterator it( m_listView );
            while( it.current() )
            {
                if( it.current()->isSelected() )
                {
                    TransferPanelItem* item=static_cast<TransferPanelItem*>( it.current() );
                    Transfer* transfer = item->transfer();
                    if( transfer->getType() == Transfer::Send && transfer->getStatus() >= Transfer::Done )
                    {
                        TransferSend* newTransfer = Application::instance()->getDccTransferManager()->newUpload();

                        newTransfer->setConnectionId( transfer->getConnectionId() );
                        newTransfer->setPartnerNick( transfer->getPartnerNick() );
                        newTransfer->setFileURL( transfer->getFileURL() );
                        newTransfer->setFileName( transfer->getFileName() );

                        if ( newTransfer->queue() )
                            newTransfer->start();
                    }
                }
                ++it;
            }
        }

        void TransferPanel::clearDcc()
        {
            Q3PtrList<Q3ListViewItem> lst;
            Q3ListViewItemIterator it( m_listView );
            while( it.current() )
            {
                TransferPanelItem* item = static_cast<TransferPanelItem*>( it.current() );
                // should we check that [item] is not null?
                if( it.current()->isSelected() && item->transfer()->getStatus() >= Transfer::Done )
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

        void TransferPanel::runDcc()
        {
            Q3ListViewItemIterator it( m_listView );
            while( it.current() )
            {
                if( it.current()->isSelected() )
                {
                    TransferPanelItem* item=static_cast<TransferPanelItem*>( it.current() );
                    Transfer* transfer = item->transfer();
                    if( transfer->getType() == Transfer::Send || transfer->getStatus() == Transfer::Done )
                        item->runFile();
                }
                ++it;
            }
        }

        void TransferPanel::openLocation()
        {
            Q3ListViewItemIterator it( m_listView );
            while( it.current() )
            {
                if( it.current()->isSelected() )
                {
                    TransferPanelItem* item=static_cast<TransferPanelItem*>( it.current() );
                    item->openLocation();
                }
                ++it;
            }
        }

        void TransferPanel::showFileInfo()
        {
            Q3ListViewItemIterator it( m_listView );
            while( it.current() )
            {
                if( it.current()->isSelected() )
                {
                    TransferPanelItem* item=static_cast<TransferPanelItem*>( it.current() );
                    if( item->transfer()->getType() == Transfer::Send || item->transfer()->getStatus() == Transfer::Done )
                        item->openFileInfoDialog();
                }
                ++it;
            }
        }

        void TransferPanel::selectAll()
        {
            Q3ListViewItemIterator it( m_listView );
            while ( it.current() )
            {
                m_listView->setSelected( *it, true );
                ++it;
            }
            updateButton();
        }

        void TransferPanel::selectAllCompleted()
        {
            Q3ListViewItemIterator it( m_listView );
            while ( it.current() )
            {
                TransferPanelItem* item=static_cast<TransferPanelItem*>( it.current() );
                m_listView->setSelected( *it, item->transfer()->getStatus() >= Transfer::Done );
                ++it;
            }
            updateButton();
        }

        void TransferPanel::popupRequested(Q3ListViewItem* /* item */, const QPoint& pos, int /* col */)  // slot
        {
            updateButton();
            m_popup->popup(pos);
        }

        void TransferPanel::popupActivated( QAction * act ) // slot
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

        void TransferPanel::doubleClicked(Q3ListViewItem* _item, const QPoint& /* _pos */, int /* _col */)
        {
            TransferPanelItem* item = static_cast<TransferPanelItem*>(_item);
            item->runFile();
        }

        // virtual
        void TransferPanel::childAdjustFocus()
        {
        }

        K3ListView* TransferPanel::getListView()
        {
          return m_listView;
        }

    }
}

#include "transferpanel.moc"
