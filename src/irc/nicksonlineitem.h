/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef NICKSONLINEITEM_H
#define NICKSONLINEITEM_H

#include <QTreeWidget>

class NicksOnlineItem : public QTreeWidgetItem
{
    public:
        enum NickListViewColumn
        {
            NetworkRootItem=0,  // TODO: not used yet
            NicknameItem=1,     // TODO: not used yet
            ChannelItem=2      // TODO: not used yet
        };

        NicksOnlineItem(int type,
                        QTreeWidget* parent,
                        const QString& name,
                        const QString& col2 = QString());

        NicksOnlineItem(int type,
                        QTreeWidgetItem* parent,
                        const QString& name,
                        const QString& col2 = QString());
        ~NicksOnlineItem() override = default;

        /**
        * Reimplemented to make sure, "Offline" items always get sorted to the bottom of the list
        * @param item              Pointer to the QTreeWidgetItem to compare with.
        * @return                  -1 if this item's value is smaller than i, 0 if they are equal, 1 if it's greater
        */
        bool operator<(const QTreeWidgetItem &item) const override;
        /**
        * Returns the type of the item.
        * @return                  One of the enum NickListViewColumn
        */
        int type() const;

        /// Set the connection ID this item is associated with to @p id.
        void setConnectionId(int id) { m_connectionId = id; }
        /// Returns the connection ID this item is associated with.
        int connectionId() const { return m_connectionId; }

        /// Set the nick's offline state as @p state
        void setOffline (bool state) { setData(0, Qt::UserRole, state); }
        /// Returns true if the nick is currently offline.
        bool isOffline () const { return data(0, Qt::UserRole).toBool(); }

    private:
        int m_type;
        int m_connectionId;

        Q_DISABLE_COPY(NicksOnlineItem)
};

#endif
