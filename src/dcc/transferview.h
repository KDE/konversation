/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009, 2010 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERVIEW_H
#define TRANSFERVIEW_H

#include <QTreeView>



#include "transferlistmodel.h"

class QKeyEvent;

class KCategoryDrawer;

namespace Konversation
{
    namespace DCC
    {
        class Transfer;
        class TransferListProxyModel;

        /**
         * This class represents a DCC transferview for transfermodel.
         */
        class TransferView : public QTreeView
        {
            Q_OBJECT
        public:

            explicit TransferView(QWidget *parent = nullptr);
            ~TransferView() override;

            void addTransfer(Transfer *transfer);

            int itemCount() const;
            int rowCount() const;

            QList<QModelIndex> rowIndexes(int column = 0) const;
            QList<QModelIndex> selectedIndexes() const override;
            QList<QModelIndex> selectedRows(int column = 0) const;
            QModelIndex index(int row, int column) const;
            QModelIndex index(Transfer *transfer) const;

            void selectAllCompleted();
            void selectRow(int row);
            void selectRows(const QList<int>& rows);

        Q_SIGNALS:
            void runSelectedTransfers();

        public Q_SLOTS:
            void clear();

            void headerCustomContextMenuRequested(const QPoint &pos);

            void toggleFilenameColumn(bool visible);
            void togglePartnerNickColumn(bool visible);
            void toggleProgressColumn(bool visible);
            void toggleStartedAtColumn(bool visible);
            void togglePositionColumn(bool visible);
            void toggleCurrentSpeedColumn(bool visible);
            void toggleSenderAdressColumn(bool visible);
            void toggleStatusColumn(bool visible);
            void toggleTimeLeftColumn (bool visible);
            void toogleTypeIconColumn(bool visible);

            void update();
            void updateModel();

            void transferStatusChanged(Konversation::DCC::Transfer *transfer,
                                       int newStatus, int oldStatus);

        protected:
            void drawRow(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const override;
            void scrollContentsBy(int dx, int dy) override;
            void keyPressEvent(QKeyEvent *event) override;

        private Q_SLOTS:
            void rowsAboutToBeRemovedFromModel(const QModelIndex &parent,
                                               int start, int end);
            void rowsRemovedFromModel(int start, int end);

        private:
            //extra enum needed because ItemDisplayType are not or-able
            enum CategoryState
            {
                None            = 0,
                SendCategory    = 1,
                ReceiveCategory = 1 << 1,
                SpacerRow       = 1 << 2
            };
            int m_categorieFlags;

            inline int headerTypeToColumn(int headerType) const;
            inline void setProgressBarDeletegate();
            inline int removeItems(TransferItemData::ItemDisplayType displaytype);

            inline void saveColumns();
            inline void restoreColumns();

            inline void addItem(Transfer *transfer, TransferItemData::ItemDisplayType type);

        private:
            KCategoryDrawer *m_categoryDrawer;

            TransferListModel *m_dccModel;
            TransferListProxyModel *m_proxyModel;

            QTimer *m_updateTimer;
            int m_activeTransfers;

            int m_itemCategoryToRemove;

            Q_DISABLE_COPY(TransferView)
        };

    }
}

#endif //TRANSFERVIEW_H
