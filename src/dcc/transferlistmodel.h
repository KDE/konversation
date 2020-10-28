/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009, 2010 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERLISTMODEL_H
#define TRANSFERLISTMODEL_H

#include <KIO/Global>

#include <QString>
#include <QTreeWidget>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>

#include "transfer.h"

class KCategoryDrawer;

namespace Konversation
{
    namespace DCC
    {
        class TransferHeaderData
        {
        public:
            enum HeaderType
            {
                TypeIcon    = 0,
                OfferDate   = 1,
                Status      = 2,
                FileName    = 3,
                PartnerNick = 4,
                Progress    = 5,
                Position    = 6,
                TimeLeft    = 7,
                CurrentSpeed= 8,
                SenderAdress= 9,
                COUNT       = 10 //last item
            };

            static QString typeToName(int headertype);

            QString name;
            int type;
        };


        class TransferItemData
        {
        public:
            // order+values are important, from the lowest to highest item
            enum ItemDisplayType
            {
                SendItem = QTreeWidgetItem::UserType + 1,
                SendCategory,
                SpaceRow,
                ReceiveItem,
                ReceiveCategory
            };

            int displayType;
            Transfer *transfer;
        };


        class TransferSizeDelegate : public QStyledItemDelegate
        {
            Q_OBJECT

        public:
            explicit TransferSizeDelegate(KCategoryDrawer* categoryDrawer, QObject *parent = nullptr);

            QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;
            void paint(QPainter *painter, const QStyleOptionViewItem & option,
                                const QModelIndex &index) const override;
        private:
            KCategoryDrawer* m_categoryDrawer;
        };

        class TransferProgressBarDelegate : public QStyledItemDelegate
        {
            Q_OBJECT

        public:
            explicit TransferProgressBarDelegate(QObject *parent = nullptr);

            void paint(QPainter *painter, const QStyleOptionViewItem & option,
                                const QModelIndex &index) const override;
        };


        class TransferListProxyModel : public QSortFilterProxyModel
        {
            Q_OBJECT

        public:
            explicit TransferListProxyModel(QObject *parent = nullptr);

            bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
        };


        /**
         * This class represents a DCC transferlist model.
         */
        class TransferListModel : public QAbstractListModel
        {
            Q_OBJECT
        public:
            enum Role
            {
                TransferDisplayType = Qt::UserRole + 1,
                HeaderType,
                TransferType,
                TransferStatus,
                TransferPointer,
                TransferProgress,
                TransferOfferDate //to get the QDateTime, not just a time string
            };

            explicit TransferListModel(QObject *parent);

            void append(const TransferItemData &item);

            void appendHeader(TransferHeaderData data);

            int columnCount(const QModelIndex &parent = QModelIndex()) const override;
            // counts the rows regardless of what type they are
            int rowCount(const QModelIndex &parent = QModelIndex()) const override;
            // itemCount only counts the transferitems, not the CategoryItems
            int itemCount(TransferItemData::ItemDisplayType displaytype) const;

            QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
            QVariant headerData (int section, Qt::Orientation orientation,
                                 int role = Qt::DisplayRole) const override;

            bool removeRow (int row, const QModelIndex &parent = QModelIndex());
            bool removeRows (int row, int count,
                                     const QModelIndex &parent = QModelIndex()) override;

            Qt::ItemFlags flags (const QModelIndex &index) const override;

            static QString getStatusText(Transfer::Status status, Transfer::Type type);
            static QString getSpeedPrettyText(transferspeed_t speed);
            static QString getTimeLeftPrettyText(int timeleft);
            static QString secToHMS(long sec);
        Q_SIGNALS:
            //use our own signal that guarantees the data was removed from model
            //NOTE: rowsRemoved does not
            void rowsPermanentlyRemoved (int startrow, int endrow);

        public Q_SLOTS:
            void transferStatusChanged(Konversation::DCC::Transfer *transfer,
                                       int oldstatus, int newstatus);

        private:
            inline int columnToHeaderType(int column) const;
            inline QString displayTypeToString(int type) const;

            inline QString getPositionPrettyText(KIO::fileoffset_t position,
                                                 KIO::filesize_t filesize) const;
            inline QString getSenderAddressPrettyText(Transfer *transfer) const;
            inline QIcon getStatusIcon(Transfer::Status status) const;
            inline QString getStatusDescription(Transfer::Status status, Transfer::Type type, const QString& errorMessage = QString()) const;

        private:
            QList<TransferItemData> m_transferList;
            QList<TransferHeaderData> m_headerList;
        };
    }
}

#endif //TRANSFERLISTMODEL_H
