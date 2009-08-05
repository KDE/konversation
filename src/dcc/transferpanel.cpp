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
/*
  Copyright (C) 2004-2008 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "transferpanel.h"
#include "application.h"
#include "transferdetailedinfopanel.h"
#include "transfermanager.h"
#include "transfersend.h"
#include "preferences.h"
#include "transferview.h"
#include "transferlistmodel.h"

#include <QPushButton>

#include <KDialog>
#include <KGlobal>
#include <KGlobalSettings>
#include <KIconLoader>
#include <KMessageBox>
#include <KMenu>
#include <KRun>
#include <KAuthorized>
#include <KVBox>
#include <KFileMetaInfo>

namespace Konversation
{
    namespace DCC
    {
        TransferPanel::TransferPanel(QWidget *parent)
            : ChatWindow(parent)
        {
            setType(ChatWindow::DccTransferPanel);
            setName(i18n("DCC Status"));

            initGUI();

            connect(Application::instance()->getDccTransferManager(), SIGNAL(newTransferAdded(Konversation::DCC::Transfer*)),
                    this, SLOT(slotNewTransferAdded(Konversation::DCC::Transfer*)));
        }

        TransferPanel::~TransferPanel()
        {
        }

        void TransferPanel::initGUI()
        {
            setSpacing(0);

            m_transferView = new TransferView(this);

            connect(m_transferView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    this, SLOT(updateButton()));

            // detailed info panel
            m_detailPanel = new TransferDetailedInfoPanel(this);

            // button

            KHBox *buttonsBox = new KHBox(this);
            buttonsBox->setSpacing(spacing());

            m_buttonAccept = new QPushButton(KIcon("media-playback-start"), i18n("Accept"), buttonsBox);
            m_buttonAccept->setObjectName("start_dcc");
            m_buttonAbort  = new QPushButton(KIcon("process-stop"), i18n("Abort"), buttonsBox);
            m_buttonAbort->setObjectName("abort_dcc");
            m_buttonClear  = new QPushButton(KIcon("edit-delete"), i18n("Clear"), buttonsBox);
            m_buttonClear->setObjectName("clear_dcc");
            m_buttonOpen   = new QPushButton(KIcon("system-run"), i18n("Open File"), buttonsBox);
            m_buttonOpen->setObjectName("open_dcc_file");
            m_buttonOpenLocation = new QPushButton(KIcon("document-open-folder"), i18n("Open Location"), buttonsBox);
            m_buttonOpenLocation->setObjectName("open_dcc_file_location");
            m_buttonDetail = new QPushButton(KIcon("dialog-information"), i18n("Details"), buttonsBox);
            m_buttonDetail->setObjectName("detail_dcc");
            m_buttonDetail->setCheckable(true);

            m_buttonAccept->setStatusTip(i18n("Start receiving"));
            m_buttonAbort->setStatusTip(i18n("Abort the transfer(s)"));
            m_buttonOpen->setStatusTip(i18n("Run the file"));
            m_buttonOpenLocation->setStatusTip(i18n("Open the file location"));
            m_buttonDetail->setStatusTip(i18n("View DCC transfer details"));

            connect(m_buttonAccept, SIGNAL(clicked()), this, SLOT(acceptDcc()));
            connect(m_buttonAbort, SIGNAL(clicked()), this, SLOT(abortDcc()));
            connect(m_buttonClear, SIGNAL(clicked()), this, SLOT(clearDcc()));
            connect(m_buttonOpen, SIGNAL(clicked()), this, SLOT(runDcc()));
            connect(m_buttonOpenLocation, SIGNAL(clicked()), this, SLOT(openLocation()));
            connect(m_buttonDetail, SIGNAL(toggled(bool)), m_detailPanel, SLOT(setVisible(bool)));
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
            m_openLocation = m_popup->addAction(KIcon("document-open-folder"), i18n("Open Location"));
            m_info = m_popup->addAction(KIcon("dialog-information"),i18n("File &Information"));

            m_transferView->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(m_transferView, SIGNAL(customContextMenuRequested (const QPoint&)), this, SLOT(popupRequested (const QPoint&)));
            connect(m_popup, SIGNAL(triggered (QAction*)), this, SLOT(popupActivated(QAction*)));

            // misc.
            connect(m_transferView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(doubleClicked(const QModelIndex&)));
            connect(m_transferView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
                    this, SLOT(setDetailPanelItem (const QModelIndex&, const QModelIndex&)));

            updateButton();
        }

        void TransferPanel::slotNewTransferAdded(Transfer *transfer)
        {
            connect(transfer, SIGNAL(statusChanged(Konversation::DCC::Transfer*, int, int)), this, SLOT(slotTransferStatusChanged()));
            m_transferView->addTransfer(transfer);
            if (m_transferView->itemCount() == 1)
            {
                m_transferView->selectAll();
                m_detailPanel->setTransfer(transfer);
                updateButton();
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
            QItemSelectionModel *selectionModel = m_transferView->selectionModel();
            foreach (const QModelIndex &index, m_transferView->rowIndexes())
            {
                Transfer::Type type = (Transfer::Type)index.data(TransferListModel::TransferType).toInt();
                Transfer::Status status = (Transfer::Status)index.data(TransferListModel::TransferStatus).toInt();

                selectAll = true;
                selectAllCompleted |= (status >= Transfer::Done);

                if (selectionModel->isRowSelected(index.row(), QModelIndex()))
                {
                    ++selectedItems;

                    accept &= (status == Transfer::Queued);

                    abort  |= (status < Transfer::Done);

                    clear  |= (status >= Transfer::Done);

                    info   &= (type == Transfer::Send ||
                        status == Transfer::Done);

                    open   &= (type == Transfer::Send ||
                        status == Transfer::Done);

                    openLocation = true;

                    resend |= (type == Transfer::Send &&
                        status >= Transfer::Done);
                }
            }

            if(!selectedItems)
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

            m_buttonAccept->setEnabled(accept);
            m_buttonAbort->setEnabled(abort);
            m_buttonClear->setEnabled(clear);
            m_buttonOpen->setEnabled(open);
            m_buttonOpenLocation->setEnabled(openLocation);

            m_selectAll->setEnabled(selectAll);
            m_selectAllCompleted->setEnabled(selectAllCompleted);
            m_accept->setEnabled(accept);
            m_abort->setEnabled(abort);
            m_clear->setEnabled(clear);
            m_open->setEnabled(open);
            m_openLocation->setEnabled(openLocation);
            m_resend->setEnabled(resend);
            m_info->setEnabled(info);
        }

        void TransferPanel::setDetailPanelItem (const QModelIndex &newindex, const QModelIndex &/*oldindex*/)
        {
            Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(newindex.data(TransferListModel::TransferPointer)));
            if (transfer)
            {
                m_detailPanel->setTransfer(transfer);
            }
        }

        void TransferPanel::acceptDcc()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                if (index.data(TransferListModel::TransferType).toInt() == Transfer::Receive &&
                    index.data(TransferListModel::TransferStatus).toInt() == Transfer::Queued)
                {
                    Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(index.data(TransferListModel::TransferPointer)));
                    if (transfer)
                    {
                        transfer->start();
                    }
                }
            }
            updateButton();
        }

        void TransferPanel::abortDcc()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                if (index.data(TransferListModel::TransferStatus).toInt() < Transfer::Done)
                {
                    Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(index.data(TransferListModel::TransferPointer)));
                    if (transfer)
                    {
                        transfer->abort();
                    }
                }
            }
            updateButton();
        }

        void TransferPanel::resendFile()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                if (index.data(TransferListModel::TransferType).toInt() == Transfer::Send &&
                    index.data(TransferListModel::TransferStatus).toInt() >= Transfer::Done)
                {
                    Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(index.data(TransferListModel::TransferPointer)));
                    if (!transfer)
                    {
                        continue;
                    }

                    TransferSend *newTransfer = Application::instance()->getDccTransferManager()->newUpload();

                    newTransfer->setConnectionId(transfer->getConnectionId());
                    newTransfer->setPartnerNick(transfer->getPartnerNick());
                    newTransfer->setFileURL(transfer->getFileURL());
                    newTransfer->setFileName(transfer->getFileName());

                    if (newTransfer->queue())
                    {
                        newTransfer->start();
                    }
                }
            }
        }

        //sort QModelIndexList descending
        bool rowGreaterThan(const QModelIndex &index1, const QModelIndex &index2)
        {
            return index1.row() >= index2.row();
        }

        void TransferPanel::clearDcc()
        {
            QModelIndexList indexes = m_transferView->selectedRows();
            QModelIndexList indexesToRemove;

            foreach (const QModelIndex &index, indexes)
            {
                if (index.data(TransferListModel::TransferStatus).toInt() >= Transfer::Done)
                {
                    indexesToRemove.append(index);
                }
            }

            //sort QModelIndexList descending
            //NOTE: selectedRows() returned an unsorted list
            qSort(indexesToRemove.begin(), indexesToRemove.end(), rowGreaterThan);

            //remove from last to first item, to keep a valid row
            foreach (const QModelIndex &index, indexesToRemove)
            {
                m_transferView->model()->removeRow(index.row(), QModelIndex());
                //needed, otherwise valid rows "can be treated" as invalid,
                //proxymodel does not keep up with changes
                m_transferView->updateModel();
            }

            //remove all gone items
            foreach (const QModelIndex &index, indexesToRemove)
            {
                indexes.removeOne(index);
            }

            m_transferView->clearSelection();
            //select everything that got not removed
            foreach (const QModelIndex &index, indexes)
            {
                int offset = 0;
                foreach (const QModelIndex &removedIndex, indexesToRemove)
                {
                    if (removedIndex.row() < index.row())
                    {
                        ++offset;
                    }
                }
                m_transferView->selectRow(index.row() - offset);
            }

            if (m_transferView->itemCount() == 0 || m_transferView->selectedIndexes().count() == 0)
            {
                m_detailPanel->clear();
            }

            updateButton();
        }

        void TransferPanel::runDcc()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                if (index.data(TransferListModel::TransferType).toInt() == Transfer::Send &&
                    index.data(TransferListModel::TransferStatus).toInt() == Transfer::Done)
                {
                    Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(index.data(TransferListModel::TransferPointer)));
                    if (transfer)
                    {
                        runFile(transfer);
                    }
                }
            }
        }

        void TransferPanel::openLocation()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(index.data(TransferListModel::TransferPointer)));
                if (transfer)
                {
                    openLocation(transfer);
                }
            }
        }

        void TransferPanel::showFileInfo()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(index.data(TransferListModel::TransferPointer)));
                if (transfer)
                {
                    openFileInfoDialog(transfer);
                }
            }
        }

        void TransferPanel::selectAll()
        {
            m_transferView->selectAll();
            updateButton();
        }

        void TransferPanel::selectAllCompleted()
        {
            m_transferView->selectAllCompleted();
            updateButton();
        }

        void TransferPanel::popupRequested(const QPoint &pos)
        {
            updateButton();
            m_popup->popup(QWidget::mapToGlobal(pos));
        }

        void TransferPanel::popupActivated(QAction *act)
        {
            if (act == m_abort)
            {
                abortDcc();
            }
            else if (act == m_accept)
            {
                acceptDcc();
            }
            else if (act == m_clear)
            {
                clearDcc();
            }
            else if (act == m_info)
            {
                showFileInfo();
            }
            else if (act == m_open)
            {
                runDcc();
            }
            else if (act == m_openLocation)
            {
                openLocation();
            }
            else if (act == m_selectAll)
            {
                selectAll();
            }
            else if (act == m_selectAllCompleted)
            {
                selectAllCompleted();
            }
            else if (act == m_resend)
            {
                resendFile();
            }
        }

        void TransferPanel::doubleClicked(const QModelIndex &index)
        {
            Transfer *transfer = static_cast<Transfer*>(qVariantValue<QObject*>(index.data(TransferListModel::TransferPointer)));
            if (transfer)
            {
                runFile(transfer);
            }
        }

        // virtual
        void TransferPanel::childAdjustFocus()
        {
        }

        TransferView *TransferPanel::getTransferView()
        {
            return m_transferView;
        }

        void TransferPanel::runFile(Transfer *transfer)
        {
            if (transfer->getType() == Transfer::Send || transfer->getStatus() == Transfer::Done)
            {
                new KRun(transfer->getFileURL(), getTransferView());
            }
        }

        void TransferPanel::openLocation(Transfer *transfer)
        {
            QString urlString = transfer->getFileURL().path();
            if (!urlString.isEmpty())
            {
                KUrl url(urlString);
                url.setFileName(QString());
                new KRun(url, 0, 0, true, true);
            }
        }

        void TransferPanel::openFileInfoDialog(Transfer *transfer)
        {
            if (transfer->getType() == Transfer::Send || transfer->getStatus() == Transfer::Done)
            {
                QStringList infoList;

                QString path = transfer->getFileURL().path();

                // get meta info object
                KFileMetaInfo fileMetaInfo(path, QString(), KFileMetaInfo::Everything);

                // is there any info for this file?
                if (fileMetaInfo.isValid())
                {
                    const QHash<QString, KFileMetaInfoItem>& items = fileMetaInfo.items();
                    QHash<QString, KFileMetaInfoItem>::const_iterator it = items.constBegin();
                    const QHash<QString, KFileMetaInfoItem>::const_iterator end = items.constEnd();
                    while (it != end)
                    {
                        const KFileMetaInfoItem &metaInfoItem = it.value();
                        const QVariant &value = metaInfoItem.value();
                        if (value.isValid())
                        {
                            // append item information to list
                            infoList.append("- " + metaInfoItem.name() + ' ' + value.toString());
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
                            getTransferView(),
                            "<qt>"+infoList.join("<br>")+"</qt>",
                            i18n("File Information")
                            );
                        #endif
                    }
                }
                else
                {
                    KMessageBox::sorry(getTransferView(), i18n("No detailed information for this file found."), i18n("File Information"));
                }
            }
        }

    }
}

#include "transferpanel.moc"
