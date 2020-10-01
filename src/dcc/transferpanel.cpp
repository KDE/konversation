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
  Copyright (C) 2009,2010 Bernd Buschinski <b.buschinski@web.de>
*/

#include "transferpanel.h"
#include "application.h"
#include "transferdetailedinfopanel.h"
#include "transfermanager.h"
#include "transfersend.h"
#include "preferences.h"
#include "transferview.h"
#include "transferlistmodel.h"

#include <QSplitter>

#include <KMessageBox>
#include <QMenu>
#include <KRun>
#include <KAuthorized>
#include <KToolBar>
#include <KSharedConfig>

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

            connect(Application::instance()->getDccTransferManager(), &TransferManager::newTransferAdded,
                    this, &TransferPanel::slotNewTransferAdded);
        }

        TransferPanel::~TransferPanel()
        {
            KConfigGroup config(KSharedConfig::openConfig(), "DCC Settings");
            const QByteArray state = m_splitter->saveState();
            config.writeEntry(QStringLiteral("PanelSplitter"), state.toBase64());
        }

        void TransferPanel::initGUI()
        {
            setSpacing(0);
            m_toolBar = new KToolBar(this, true, true);
            m_toolBar->setObjectName(QStringLiteral("dccstatus_toolbar"));

            m_splitter = new QSplitter(this);
            m_splitter->setOrientation(Qt::Vertical);

            m_transferView = new TransferView(m_splitter);

            connect(m_transferView->selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &TransferPanel::updateButton);
            connect(m_transferView, &TransferView::runSelectedTransfers, this, &TransferPanel::runDcc);

            // detailed info panel
            m_detailPanel = new TransferDetailedInfoPanel(m_splitter);

            m_splitter->setStretchFactor(0, QSizePolicy::Expanding);

            // popup menu
            m_popup = new QMenu(this);
            m_selectAll =  m_popup->addAction(i18n("&Select All Items"), this, &TransferPanel::selectAll);
            m_selectAllCompleted = m_popup->addAction(i18n("S&elect All Completed Items"), this, &TransferPanel::selectAllCompleted);
            m_popup->addSeparator();                           // -----
            m_accept =  m_popup->addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), i18n("&Accept"), this, &TransferPanel::acceptDcc);
            m_accept->setStatusTip(i18n("Start receiving"));
            m_abort = m_popup->addAction(QIcon::fromTheme(QStringLiteral("process-stop")),i18n("A&bort"), this, &TransferPanel::abortDcc);
            m_abort->setStatusTip(i18n("Abort the transfer(s)"));
            m_popup->addSeparator();                           // -----
            m_resend = m_popup->addAction(QIcon::fromTheme(QStringLiteral("edit-redo")),i18n("Resend"), this, &TransferPanel::resendFile);
            m_clear = m_popup->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")),i18nc("clear selected dcctransfer","&Clear"), this, &TransferPanel::clearDcc);
            m_clear->setStatusTip(i18n("Clear all selected Items"));
            m_clearCompleted = m_popup->addAction(QIcon::fromTheme(QStringLiteral("edit-clear-list")),i18n("Clear Completed"), this, &TransferPanel::clearCompletedDcc);
            m_clearCompleted->setStatusTip(i18n("Clear Completed Items"));
            m_popup->addSeparator();                           // -----
            m_open = m_popup->addAction(QIcon::fromTheme(QStringLiteral("system-run")), i18n("&Open File"), this, &TransferPanel::runDcc);
            m_open->setStatusTip(i18n("Run the file"));
            m_openLocation = m_popup->addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Open Location"), this, SLOT(openLocation()));
            m_openLocation->setStatusTip(i18n("Open the file location"));

            m_transferView->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(m_transferView, &TransferView::customContextMenuRequested, this, &TransferPanel::popupRequested);

            // misc.
            connect(m_transferView, &TransferView::doubleClicked, this, &TransferPanel::doubleClicked);
            connect(m_transferView->selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &TransferPanel::setDetailPanelItem);

            m_toolBar->addAction(m_accept);
            m_toolBar->addAction(m_abort);
            m_toolBar->addAction(m_clear);
            m_toolBar->addAction(m_clearCompleted);
            m_toolBar->addAction(m_open);
            m_toolBar->addAction(m_openLocation);

            KConfigGroup config(KSharedConfig::openConfig(), "DCC Settings");
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
            connect(transfer, &Transfer::statusChanged, this, &TransferPanel::slotTransferStatusChanged);
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

            if (!KAuthorized::authorizeAction(QStringLiteral("allow_downloading")))
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
        }

        void TransferPanel::setDetailPanelItem (const QItemSelection &/*newindex*/, const QItemSelection &/*oldindex*/)
        {
            QModelIndex index;

            if (m_transferView->selectionModel()->selectedRows().contains(m_transferView->selectionModel()->currentIndex()))
            {
                index = m_transferView->selectionModel()->currentIndex();
            }
            else if (!m_transferView->selectionModel()->selectedRows().isEmpty())
            {
                index = m_transferView->selectionModel()->selectedRows().first();
            }

            if (index.isValid())
            {
                Transfer *transfer = qobject_cast<Transfer*>(index.data(TransferListModel::TransferPointer).value<QObject*>());
                if (transfer)
                {
                    m_detailPanel->setTransfer(transfer);
                }
            }
        }

        void TransferPanel::acceptDcc()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                if (index.data(TransferListModel::TransferType).toInt() == Transfer::Receive &&
                    index.data(TransferListModel::TransferStatus).toInt() == Transfer::Queued)
                {
                    Transfer *transfer = qobject_cast<Transfer*>(index.data(TransferListModel::TransferPointer).value<QObject*>());
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
                    Transfer *transfer = qobject_cast<Transfer*>(index.data(TransferListModel::TransferPointer).value<QObject*>());
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
            QList<Transfer*> transferList;
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                if (index.data(TransferListModel::TransferType).toInt() == Transfer::Send &&
                    index.data(TransferListModel::TransferStatus).toInt() >= Transfer::Done)
                {
                    Transfer *transfer = qobject_cast<Transfer*>(index.data(TransferListModel::TransferPointer).value<QObject*>());
                    if (!transfer)
                    {
                        continue;
                    }
                    transferList.append(transfer);
                }
            }

            foreach (Transfer* transfer, transferList)
            {
                TransferSend *newTransfer = Application::instance()->getDccTransferManager()->newUpload();

                newTransfer->setConnectionId(transfer->getConnectionId());
                newTransfer->setPartnerNick(transfer->getPartnerNick());
                newTransfer->setFileURL(transfer->getFileURL());
                newTransfer->setFileName(transfer->getFileName());
                newTransfer->setReverse(transfer->isReverse());

                if (newTransfer->queue())
                {
                    newTransfer->start();
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
            //selected item
            Transfer *transfer = m_detailPanel->transfer();
            if (transfer && transfer->getStatus() >= Transfer::Done)
            {
                //item will be gone
                transfer = nullptr;
            }

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
            std::sort(indexesToRemove.begin(), indexesToRemove.end(), rowGreaterThan);

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

            if (transfer)
            {
                m_detailPanel->setTransfer(transfer);
            }
            else if (!transfer || m_transferView->itemCount() == 0 || m_transferView->selectedIndexes().isEmpty())
            {
                m_detailPanel->clear();
            }

            updateButton();
        }

        void TransferPanel::clearCompletedDcc()
        {
            //save selected item
            Transfer *transfer = m_detailPanel->transfer();
            if (transfer && transfer->getStatus() >= Transfer::Done)
            {
                //item will be gone
                transfer = nullptr;
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
            std::sort(indexesToRemove.begin(), indexesToRemove.end(), rowGreaterThan);

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
            else if (m_transferView->itemCount() == 0 || m_transferView->selectedIndexes().isEmpty() || !transfer)
            {
                m_detailPanel->clear();
            }

            updateButton();
        }

        void TransferPanel::runDcc()
        {
            const int selectedRows = m_transferView->selectedRows().count();
            if (selectedRows > 3)
            {
                int ret = KMessageBox::questionYesNo(this,
                                                     i18np("You have selected %1 file to execute, are you sure you want to continue?",
                                                           "You have selected %1 files to execute, are you sure you want to continue?",
                                                           selectedRows),
                                                     i18np("Execute %1 file",
                                                           "Execute %1 files",
                                                           selectedRows)
                                                     );

                if (ret == KMessageBox::No)
                {
                    return;
                }
            }

            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                if (index.data(TransferListModel::TransferType).toInt() == Transfer::Send ||
                    index.data(TransferListModel::TransferStatus).toInt() == Transfer::Done)
                {
                    Transfer *transfer = qobject_cast<Transfer*>(index.data(TransferListModel::TransferPointer).value<QObject*>());
                    if (transfer)
                    {
                        transfer->runFile();
                    }
                }
            }
        }

        void TransferPanel::openLocation()
        {
            foreach (const QModelIndex &index, m_transferView->selectedRows())
            {
                Transfer *transfer = qobject_cast<Transfer*>(index.data(TransferListModel::TransferPointer).value<QObject*>());
                if (transfer)
                {
                    openLocation(transfer);
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
            m_popup->popup(QWidget::mapToGlobal(m_transferView->viewport()->mapTo(this, pos)));
        }

        void TransferPanel::doubleClicked(const QModelIndex &index)
        {
            Transfer *transfer = qobject_cast<Transfer*>(index.data(TransferListModel::TransferPointer).value<QObject*>());
            if (transfer)
            {
                transfer->runFile();
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

        void TransferPanel::openLocation(Transfer *transfer)
        {
            QString urlString = transfer->getFileURL().toString(QUrl::PreferLocalFile|QUrl::RemoveFilename|QUrl::StripTrailingSlash);
            if (!urlString.isEmpty())
            {
                QUrl url(QUrl::fromLocalFile(urlString));
                new KRun(url, nullptr, true);
            }
        }

    }
}


