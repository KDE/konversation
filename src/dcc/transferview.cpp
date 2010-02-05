/*
  This class represents a DCC transferview for transfermodel.
*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009,2010 Bernd Buschinski <b.buschinski@web.de>
*/

#include "transferview.h"


#include <KDebug>
#include <KMenu>
#include <KCategoryDrawer>
#include <klocalizedstring.h>

#include <QHeaderView>

#include <preferences.h>

namespace Konversation
{
    namespace DCC
    {
        TransferView::TransferView(QWidget *parent)
            : QTreeView(parent)
        {
            m_categorieFlags = None;
            m_dccModel = new TransferListModel(this);
            m_proxyModel = new TransferListProxyModel(this);
            m_proxyModel->setSourceModel(m_dccModel);
            setModel(m_proxyModel);

            // doc says it improves performance
            // but brings problems with KCategoryDrawer starting with kde4.4
            setUniformRowHeights(false);

            setSortingEnabled(true);
            setRootIsDecorated(false); //not implemented for special items
            setSelectionMode(QAbstractItemView::ExtendedSelection);

            m_categoryDrawer = new KCategoryDrawer();

            setItemDelegate(new TransferSizeDelegate(m_categoryDrawer, this));

            //only after model was set
            restoreColumns();

            //only after model and columns were set
            setProgressBarDeletegate();

            header()->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(header(), SIGNAL(customContextMenuRequested (const QPoint&)),
                    this, SLOT(headerCustomContextMenuRequested (const QPoint&)));

            m_activeTransfers = 0;
            m_itemCategoryToRemove = 0;
            m_updateTimer = new QTimer(this);
            m_updateTimer->setInterval(500);
            connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(update()));

            connect(model(), SIGNAL(rowsAboutToBeRemoved (const QModelIndex &, int, int)),
                     this, SLOT(rowsAboutToBeRemovedFromModel (const QModelIndex&, int, int)));
            //we can't use rowsRemoved here, it seems when rowsRemoved is emitted
            //the rows are not permanently removed from model,
            //so if we trigger a new removeRows in our slot,
            //the new remove happens before the old pending
            connect(m_dccModel, SIGNAL(rowsPermanentlyRemoved (int, int)),
                    this, SLOT(rowsRemovedFromModel (int, int)));
        }

        TransferView::~TransferView()
        {
            disconnect(m_updateTimer, 0, 0, 0);

            saveColumns();
            clear();

            delete m_categoryDrawer;
        }

        void TransferView::clear()
        {
            if (rowCount() > 0)
            {
                removeItems(TransferItemData::SendItem);
                removeItems(TransferItemData::ReceiveItem);
            }
        }

        void TransferView::drawRow(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
        {
            int type = index.data(TransferListModel::TransferDisplayType).toInt();

            if (type == TransferItemData::SendCategory || type == TransferItemData::ReceiveCategory)
            {
                m_categoryDrawer->drawCategory(index,
                                               0, //ignored anyway
                                               option,
                                               painter);
            }
            else
            {
                QTreeView::drawRow(painter, option, index);
            }
        }

        void TransferView::addTransfer (Transfer *transfer)
        {
            //save selected rows
            QModelIndexList selectedIndexes = selectedRows();
            QList<QVariant> selectedItems;
            foreach (const QModelIndex &index, selectedIndexes)
            {
                selectedItems.append(index.data(TransferListModel::TransferPointer));
            }

            if (transfer->getType() == Transfer::Send)
            {
                if (!(m_categorieFlags & SendCategory))
                {
                    addItem(0, TransferItemData::SendCategory);
                    m_categorieFlags |= SendCategory;
                }
                if ((m_categorieFlags & ReceiveCategory) && !(m_categorieFlags & SpacerRow))
                {
                    addItem(0, TransferItemData::SpaceRow);
                    m_categorieFlags |= SpacerRow;
                }
                addItem(transfer, TransferItemData::SendItem);
            }
            else if (transfer->getType() == Transfer::Receive)
            {
                if (!(m_categorieFlags & ReceiveCategory))
                {
                    addItem(0, TransferItemData::ReceiveCategory);
                    m_categorieFlags |= ReceiveCategory;
                }
                if ((m_categorieFlags & SendCategory) && !(m_categorieFlags & SpacerRow))
                {
                    addItem(0, TransferItemData::SpaceRow);
                    m_categorieFlags |= SpacerRow;
                }
                addItem(transfer, TransferItemData::ReceiveItem);
            }

            //catch already running transfers
            if (transfer->getStatus() == Transfer::Transferring)
            {
                ++m_activeTransfers;
                if (m_activeTransfers > 0 && !m_updateTimer->isActive())
                {
                    m_updateTimer->start();
                    kDebug() << "timer start";
                }
            }

            connect (transfer, SIGNAL(statusChanged(Konversation::DCC::Transfer*,int,int)),
                     this, SLOT(transferStatusChanged(Konversation::DCC::Transfer*,int,int)));

            updateModel();

            clearSelection();
            //restore selected
            foreach (const QModelIndex &index, rowIndexes())
            {
                QVariant pointer = index.data(TransferListModel::TransferPointer);
                if (selectedItems.contains(pointer))
                {
                    selectedItems.removeOne(pointer);
                    selectRow(index.row());
                    if (selectedItems.isEmpty())
                    {
                        break;
                    }
                }
            }
        }

        void TransferView::addItem (Transfer* transfer, TransferItemData::ItemDisplayType type)
        {
            TransferItemData tD;
            tD.displayType = type;
            tD.transfer = transfer;
            m_dccModel->append(tD);
        }

        void TransferView::transferStatusChanged(Transfer *transfer,
                                                 int newStatus, int oldStatus)
        {
            Q_ASSERT(newStatus != oldStatus);

            QModelIndex rowIndex = index(transfer);
            if (rowIndex.isValid())
            {
                dataChanged(rowIndex, index(rowIndex.row(), model()->columnCount()-1));
            }

            if (newStatus == Transfer::Transferring)
            {
                ++m_activeTransfers;
                if (m_activeTransfers > 0 && !m_updateTimer->isActive())
                {
                    m_updateTimer->start();
                }
            }
            if (oldStatus == Transfer::Transferring)
            {
                --m_activeTransfers;
                if (m_activeTransfers <= 0 && m_updateTimer->isActive())
                {
                    m_updateTimer->stop();
                }
            }
            update();
        }

        int TransferView::itemCount() const
        {
            int offset = 0;
            if (m_categorieFlags & SendCategory)
            {
                ++offset;
            }
            if (m_categorieFlags & ReceiveCategory)
            {
                ++offset;
            }
            if (m_categorieFlags & SpacerRow)
            {
                ++offset;
            }
            return m_dccModel->rowCount() - offset;
        }

        int TransferView::rowCount() const
        {
            return m_dccModel->rowCount();
        }

        QList<QModelIndex> TransferView::rowIndexes(int column) const
        {
            QList<QModelIndex> list;
            if (column >= m_dccModel->columnCount())
            {
                return list;
            }

            for (int i = 0; i < m_dccModel->rowCount(); ++i)
            {
                QModelIndex index = m_proxyModel->index(i, column);
                int displaytype = index.data(TransferListModel::TransferDisplayType).toInt();
                if (displaytype == TransferItemData::ReceiveItem || displaytype == TransferItemData::SendItem)
                {
                    list.append(index);
                }
            }
            return list;
        }

        QList<QModelIndex> TransferView::selectedIndexes() const
        {
            return selectionModel()->selectedIndexes();
        }

        QList<QModelIndex> TransferView::selectedRows(int column) const
        {
            return selectionModel()->selectedRows(column);
        }

        QModelIndex TransferView::index(int row, int column) const
        {
            return model()->index(row, column);
        }

        QModelIndex TransferView::index(Transfer *transfer) const
        {
            if (!transfer)
            {
                return QModelIndex();
            }

            foreach (const QModelIndex& rowIndex, rowIndexes())
            {
                Transfer *rowTransfer = static_cast<Transfer*>(qVariantValue<QObject*>(rowIndex.data(TransferListModel::TransferPointer)));
                if (rowTransfer == transfer)
                {
                    return rowIndex;
                }
            }
            return QModelIndex();
        }

        void TransferView::headerCustomContextMenuRequested(const QPoint &pos)
        {
            KMenu menu(this);
            menu.addTitle(i18n("Columns"));

            for (int i = 0; i < m_dccModel->columnCount(); ++i)
            {
                QAction *tAction = new QAction(m_dccModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString(), &menu);
                tAction->setCheckable(true);

                int headerType = m_dccModel->headerData(i, Qt::Horizontal, TransferListModel::HeaderType).toInt();

                //there must be at least one column that is not hideable
                if (headerType == TransferHeaderData::FileName)
                {
                    delete tAction;
                    continue;
                }

                switch (headerType)
                {
        //            case DccHeaderData::FileName:
        //                 connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toggleFilenameColumn(bool)));
        //                 break;
                    case TransferHeaderData::PartnerNick:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(togglePartnerNickColumn(bool)));
                        break;
                    case TransferHeaderData::Progress:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toggleProgressColumn(bool)));
                        break;
                    case TransferHeaderData::OfferDate:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toggleStartedAtColumn(bool)));
                        break;
                    case TransferHeaderData::Position:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(togglePositionColumn(bool)));
                        break;
                    case TransferHeaderData::CurrentSpeed:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toggleCurrentSpeedColumn(bool)));
                        break;
                    case TransferHeaderData::SenderAdress:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toggleSenderAdressColumn(bool)));
                        break;
                    case TransferHeaderData::Status:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toggleStatusColumn(bool)));
                        break;
                    case TransferHeaderData::TimeLeft:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toggleTimeLeftColumn(bool)));
                        break;
                    case TransferHeaderData::TypeIcon:
                        connect (tAction, SIGNAL(toggled(bool)), this, SLOT(toogleTypeIconColumn(bool)));
                        break;
                }

                tAction->setChecked(!isColumnHidden(i));
                menu.addAction(tAction);
            }

            menu.exec(QWidget::mapToGlobal(pos));
        }

        void TransferView::toggleFilenameColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::FileName), !visible);
        }

        void TransferView::togglePartnerNickColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::PartnerNick), !visible);
        }

        void TransferView::toggleProgressColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::Progress), !visible);
        }

        void TransferView::toggleStartedAtColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::OfferDate), !visible);
        }

        void TransferView::toggleCurrentSpeedColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::CurrentSpeed), !visible);
        }

        void TransferView::togglePositionColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::Position), !visible);
        }

        void TransferView::toggleSenderAdressColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::SenderAdress), !visible);
        }

        void TransferView::toggleStatusColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::Status), !visible);
        }

        void TransferView::toggleTimeLeftColumn (bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::TimeLeft), !visible);
        }

        void TransferView::toogleTypeIconColumn(bool visible)
        {
            setColumnHidden(headerTypeToColumn(TransferHeaderData::TypeIcon), !visible);
        }

        int TransferView::headerTypeToColumn (int headerType) const
        {
            for (int i = 0; i < m_dccModel->columnCount(); ++i)
            {
                if (m_dccModel->headerData(i, Qt::Horizontal, TransferListModel::HeaderType).toInt() == headerType)
                {
                    return i;
                }
            }
            kDebug() << "unknown headerType: " << headerType;
            return -1;
        }

        void TransferView::setProgressBarDeletegate()
        {
            for (int i = 0; i < m_dccModel->columnCount(); ++i)
            {
                int headerType = m_dccModel->headerData(i, Qt::Horizontal, TransferListModel::HeaderType).toInt();
                if (headerType == TransferHeaderData::Progress)
                {
                    setItemDelegateForColumn (i, new TransferProgressBarDelegate(this));
                    return;
                }
            }
        }

        void TransferView::saveColumns()
        {
            QList<int> columnWidths;
            QList<int> columnOrder;
            QList<int> columnVisible;
            for (int i = 0; i < header()->count(); ++i)
            {
                int index = header()->logicalIndex(i);

                columnWidths.append(columnWidth(index));
                columnOrder.append(m_dccModel->headerData(index, Qt::Horizontal, TransferListModel::HeaderType).toInt());
                columnVisible.append(!isColumnHidden(index));
            }
            Preferences::self()->setDccColumnWidths(columnWidths);
            Preferences::self()->setDccColumnOrders(columnOrder);
            Preferences::self()->setDccColumnVisibles(columnVisible);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
            Preferences::self()->setDccColumnSorted( m_proxyModel->sortColumn() );
            Preferences::self()->setDccColumnSortDescending ( m_proxyModel->sortOrder() == Qt::DescendingOrder ? true : false );
#endif
        }

        void TransferView::restoreColumns()
        {
            QList<int> columnWidths = Preferences::self()->dccColumnWidths();
            QList<int> columnOrder = Preferences::self()->dccColumnOrders();
            QList<int> columnVisible = Preferences::self()->dccColumnVisibles();

            //fallback, columnOrder is empty for me after crash
            //rather restore default than show an empty TransferView
            if (columnOrder.count() == TransferHeaderData::COUNT &&
                columnWidths.count() == TransferHeaderData::COUNT &&
                columnVisible.count() == TransferHeaderData::COUNT)
            {
                for (int i = 0; i < columnOrder.count(); ++i)
                {
                    TransferHeaderData data;
                    data.type = columnOrder.at(i);
                    m_dccModel->appendHeader(data);
                }

                //update model, otherwise new column are unknown
                updateModel();

                for (int i = 0; i < columnOrder.count(); ++i)
                {
                    int column = headerTypeToColumn(columnOrder.at(i));
                    setColumnWidth(column, columnWidths.at(i) == 0 ? 100 : columnWidths.at(i));
                    setColumnHidden(column, (columnVisible.at(i) > 0) ? false : true);
                }

#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
                Qt::SortOrder order;
                if (Preferences::self()->dccColumnSortDescending())
                {
                    order = Qt::DescendingOrder;
                }
                else
                {
                    order = Qt::AscendingOrder;
                }
                sortByColumn(Preferences::self()->dccColumnSorted(), order);
#endif
            }
            else
            {
                kDebug() << "transferview fallback, did we crash last time?\n"
                         << " columnOrder.count():"<< columnOrder.count()
                         << " columnWidths.count():"<< columnWidths.count()
                         << " columnVisible.count():"<< columnVisible.count()
                         << ", expected: " << TransferHeaderData::COUNT;
                for (int i = 0; i < TransferHeaderData::COUNT; ++i)
                {
                    TransferHeaderData data;
                    data.type = i;
                    m_dccModel->appendHeader(data);
                }
                updateModel();
            }
        }

        void TransferView::updateModel()
        {
            m_proxyModel->setSourceModel(m_dccModel);
            m_proxyModel->invalidate();
        }

        void TransferView::scrollContentsBy (int dx, int dy)
        {
            if (dx) //KCategoryDrawer is a bit slow when it comes to horiz redraws, force it
            {
                update();
            }
            QTreeView::scrollContentsBy (dx, dy);
        }

        void TransferView::selectAllCompleted()
        {
            QItemSelection selection;
            foreach (const QModelIndex &index, rowIndexes())
            {
                if (index.data(TransferListModel::TransferStatus).toInt() >= Transfer::Done)
                {
                    selection.append(QItemSelectionRange(index));
                }
            }
            selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }

        void TransferView::selectRow(int row)
        {
            if (row >= rowCount())
            {
                return;
            }
            selectionModel()->select(m_proxyModel->index(row, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }

        void TransferView::selectRows(QList<int> rows)
        {
            QItemSelection selection;
            foreach (const QModelIndex &index, rowIndexes())
            {
                foreach (int row, rows)
                {
                    if (row == index.row())
                    {
                        selection.append(QItemSelectionRange(index));
                        break;
                    }
                }
            }
            selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }

        void TransferView::update()
        {
            const int columnCount = model()->columnCount()-1;
            foreach (const QModelIndex& rowIndex, rowIndexes(0))
            {
                int status = rowIndex.data(TransferListModel::TransferStatus).toInt();
                if (status == Transfer::Transferring)
                {
                    dataChanged(rowIndex, index(rowIndex.row(), columnCount));
                }
            }
            QTreeView::update();
        }

        void TransferView::rowsAboutToBeRemovedFromModel (const QModelIndex &/*parent*/,
                                                          int start, int end)
        {
            // The items that will be removed are those between start and end inclusive
            for (int i = start; i < end+1; ++i)
            {
                m_itemCategoryToRemove |= model()->index(i, 0).data(TransferListModel::TransferType).toInt();
            }
        }

        void TransferView::rowsRemovedFromModel (int start, int end)
        {
            if (m_itemCategoryToRemove & Transfer::Send)
            {
                if (m_dccModel->itemCount(TransferItemData::SendItem) == (start - end))
                {
                    m_itemCategoryToRemove &= ~Transfer::Send;
                    m_categorieFlags &= ~TransferView::SendCategory;
                    int removed = removeItems(TransferItemData::SendCategory);
                    //kDebug() << "Sendremoved:" << removed;
                    if (removed > 0 && (m_categorieFlags & SpacerRow))
                    {
                        removeItems(TransferItemData::SpaceRow);
                        m_categorieFlags &= ~TransferView::SpacerRow;
                    }
                }
            }
            if (m_itemCategoryToRemove & Transfer::Receive)
            {
                if (m_dccModel->itemCount(TransferItemData::ReceiveItem) == (start - end))
                {
                    m_itemCategoryToRemove &= ~Transfer::Receive;
                    m_categorieFlags &= ~TransferView::ReceiveCategory;
                    int removed = removeItems(TransferItemData::ReceiveCategory);
                    //kDebug() << "Receiveremoved:" << removed;
                    if (removed > 0 && (m_categorieFlags & SpacerRow))
                    {
                        removeItems(TransferItemData::SpaceRow);
                        m_categorieFlags &= ~TransferView::SpacerRow;
                    }
                }
            }
        }

        int TransferView::removeItems (TransferItemData::ItemDisplayType displaytype)
        {
            int removed = 0;
            for (int i = model()->rowCount()-1; i >= 0; --i)
            {
                QModelIndex index = m_proxyModel->index(i, 0);
                if (index.data(TransferListModel::TransferDisplayType).toInt() == displaytype)
                {
                    model()->removeRow(index.row());
                    ++removed;
                }
            }
            return removed;
        }

    }
}

#include "transferview.moc"
