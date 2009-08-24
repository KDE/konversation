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

#include <KGlobal>
#include <KMessageBox>
#include <KMenu>
#include <KRun>
#include <KAuthorized>
#include <KFileMetaInfo>
#include <KToolBar>
#include <QSplitter>

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
            KConfigGroup config(KGlobal::config(), "DCC Settings");
            const QByteArray state = m_splitter->saveState();
            config.writeEntry(QString("PanelSplitter"), state.toBase64());
        }

        void TransferPanel::initGUI()
        {
            setSpacing(0);
            m_toolBar = new KToolBar(this, true, true);
            m_toolBar->setObjectName("dccstatus_toolbar");

            m_splitter = new QSplitter(this);
            m_splitter->setOrientation(Qt::Vertical);

            m_transferView = new TransferView(m_splitter);

            connect(m_transferView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    this, SLOT(updateButton()));

            // detailed info panel
            m_detailPanel = new TransferDetailedInfoPanel(m_splitter);

            m_splitter->setStretchFactor(0, QSizePolicy::Expanding);

            // popup menu
            m_popup = new KMenu(this);
            m_selectAll =  m_popup->addAction(i18n("&Select All Items"), this, SLOT(selectAll()));
            m_selectAllCompleted = m_popup->addAction(i18n("S&elect All Completed Items"), this, SLOT(selectAllCompleted()));
            m_popup->addSeparator();                           // -----
            m_accept =  m_popup->addAction(KIcon("media-playback-start"), i18n("&Accept"), this, SLOT(acceptDcc()));
            m_accept->setStatusTip(i18n("Start receiving"));
            m_abort = m_popup->addAction(KIcon("process-stop"),i18n("A&bort"), this, SLOT(abortDcc()));
            m_abort->setStatusTip(i18n("Abort the transfer(s)"));
            m_popup->addSeparator();                           // -----
            m_resend = m_popup->addAction(KIcon("edit-redo"),i18n("Resend"), this, SLOT(resendFile()));
            m_clear = m_popup->addAction(KIcon("edit-delete"),i18n("&Clear"), this, SLOT(clearDcc()));
            m_clear->setStatusTip(i18n("Clear all selected Items"));
            m_clearCompleted = m_popup->addAction(KIcon("edit-clear-list"),i18n("Clear Completed"), this, SLOT(clearCompletedDcc()));
            m_clearCompleted->setStatusTip(i18n("Clear Completed Items"));
            m_popup->addSeparator();                           // -----
            m_open = m_popup->addAction(KIcon("system-run"), i18n("&Open File"), this, SLOT(runDcc()));
            m_open->setStatusTip(i18n("Run the file"));
            m_openLocation = m_popup->addAction(KIcon("document-open-folder"), i18n("Open Location"), this, SLOT(openLocation()));
            m_openLocation->setStatusTip(i18n("Open the file location"));
            m_info = m_popup->addAction(KIcon("dialog-information"), i18n("File &Information"), this, SLOT(showFileInfo()));

            m_transferView->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(m_transferView, SIGNAL(customContextMenuRequested (const QPoint&)), this, SLOT(popupRequested (const QPoint&)));

            // misc.
            connect(m_transferView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(doubleClicked(const QModelIndex&)));
            connect(m_transferView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
                    this, SLOT(setDetailPanelItem (const QModelIndex&, const QModelIndex&)));

            m_toolBar->addAction(m_accept);
            m_toolBar->addAction(m_abort);
            m_toolBar->addAction(m_clear);
            m_toolBar->addAction(m_clearCompleted);
            m_toolBar->addAction(m_open);
            m_toolBar->addAction(m_openLocation);

            KConfigGroup config(KGlobal::config(), "DCC Settings");
            QByteArray state;
            if (config.hasKey("PanelSplitter"))
            {
                state = config.readEntry("PanelSplitter", state);
                state = QByteArray::fromBase64(state);
                m_splitter->restoreState(state);
            }

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
            bool accept             = false,
                 abort              = false,
                 clear              = false,
                 info               = false,
                 open               = false,
                 openLocation       = false,
                 resend             = false,
                 selectAll          = false,
                 selectAllCompleted = false;

            QItemSelectionModel *selectionModel = m_transferView->selectionModel();
            foreach (const QModelIndex &index, m_transferView->rowIndexes())
            {
                Transfer::Type type = (Transfer::Type)index.data(TransferListModel::TransferType).toInt();
                Transfer::Status status = (Transfer::Status)index.data(TransferListModel::TransferStatus).toInt();

                selectAll = true;
                selectAllCompleted |= (status >= Transfer::Done);

                if (selectionModel->isRowSelected(index.row(), QModelIndex()))
                {
                    accept |= (status == Transfer::Queued);

                    abort  |= (status < Transfer::Done);

                    clear  |= (status >= Transfer::Done);

                    info   |= (type == Transfer::Send ||
                        status == Transfer::Done);

                    open   |= (type == Transfer::Send ||
                        status == Transfer::Done);

                    openLocation = true;

                    resend |= (type == Transfer::Send &&
                        status >= Transfer::Done);
                }
            }

            if (!KAuthorized::authorizeKAction("allow_downloading"))
            {
                accept = false;
            }

            m_selectAll->setEnabled(selectAll);
            m_selectAllCompleted->setEnabled(selectAllCompleted);
            m_accept->setEnabled(accept);
            m_abort->setEnabled(abort);
            m_clear->setEnabled(clear);
            m_clearCompleted->setEnabled(selectAllCompleted);
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
            QList<int> toSelectList;
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
                toSelectList.append(index.row() - offset);
            }
            m_transferView->selectRows(toSelectList);

            if (m_transferView->itemCount() == 0 || m_transferView->selectedIndexes().count() == 0)
            {
                m_detailPanel->clear();
            }

            updateButton();
        }

        void TransferPanel::clearCompletedDcc()
        {
            //save selected item
            Transfer *transfer = m_detailPanel->transfer();
            if (transfer->getStatus() >= Transfer::Done)
            {
                //item will be gone
                transfer = 0;
            }

            QModelIndexList indexesToRemove;
            QModelIndexList selectedIndexes = m_transferView->selectedRows();

            foreach (const QModelIndex &index, m_transferView->rowIndexes())
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
                selectedIndexes.removeOne(index);
            }

            m_transferView->clearSelection();
            QList<int> toSelectList;
            //select everything that got not removed
            foreach (const QModelIndex &index, selectedIndexes)
            {
                int offset = 0;
                foreach (const QModelIndex &removedIndex, indexesToRemove)
                {
                    if (removedIndex.row() < index.row())
                    {
                        ++offset;
                    }
                }
                toSelectList.append(index.row() - offset);
            }
            m_transferView->selectRows(toSelectList);

            if (transfer)
            {
                m_detailPanel->setTransfer(transfer);
            }
            else if (m_transferView->itemCount() == 0 || m_transferView->selectedIndexes().count() == 0)
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
