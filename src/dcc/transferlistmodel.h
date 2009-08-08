/*
  This class represents a DCC transferlist model.
*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERLISTMODEL_H
#define TRANSFERLISTMODEL_H

#include <QString>
#include <QTreeWidget>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>
#include <kio/global.h>

#include "transfer.h"

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


        class TransferProgressBarDelete : public QStyledItemDelegate
        {
        public:
            virtual void paint (QPainter *painter, const QStyleOptionViewItem & option,
                                const QModelIndex &index) const;
        };


        class TransferListProxyModel : public QSortFilterProxyModel
        {
        public:
            TransferListProxyModel(QObject *parent = 0);

            bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

#if (QT_VERSION < QT_VERSION_CHECK(4, 5, 0))
            virtual void sort ( int column, Qt::SortOrder order = Qt::AscendingOrder );
        private:
            Qt::SortOrder m_sortOrder;
#endif
        };


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
                TransferProgress
            };

            TransferListModel(QObject *parent);

            void append(const TransferItemData &item);

            void appendHeader(TransferHeaderData data);

            int columnCount(const QModelIndex &parent = QModelIndex()) const;
            // counts the rows regardless of what type they are
            int rowCount(const QModelIndex &parent = QModelIndex()) const;
            // itemCount only counts the transferitems, not the CategoryItems
            int itemCount(TransferItemData::ItemDisplayType displaytype) const;

            QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
            QVariant headerData (int section, Qt::Orientation orientation,
                                 int role = Qt::DisplayRole) const;

            bool removeRow (int row, const QModelIndex &parent = QModelIndex());
            virtual bool removeRows (int row, int count,
                                     const QModelIndex &parent = QModelIndex());

            Qt::ItemFlags flags (const QModelIndex &index) const;

            static QString getStatusText(Transfer::Status status, Transfer::Type type);
            static QString getSpeedPrettyText(transferspeed_t speed);
            static QString getTimeLeftPrettyText(int timeleft);
            static QString secToHMS(long sec);
        signals:
            //use our own signal that guarantees the data was removed from model
            //NOTE: rowsRemoved does not
            void rowsPermanentlyRemoved (int startrow, int endrow);

        public slots:
            void transferStatusChanged(Konversation::DCC::Transfer *transfer,
                                       int oldstatus, int newstatus);

        private:
            inline int columnToHeaderType(int column) const;

            inline QString getPositionPrettyText(KIO::fileoffset_t position,
                                                 KIO::filesize_t filesize) const;
            inline QString getSenderAddressPrettyText(Transfer *transfer) const;
            inline QPixmap getStatusIcon(Transfer::Status status) const;
            inline QPixmap getTypeIcon(Transfer::Type type) const;

            QList<TransferItemData> m_transferList;
            QList<TransferHeaderData> m_headerList;
        };
    }
}

#endif //TRANSFERLISTMODEL_H
