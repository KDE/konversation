/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef IGNORELISTVIEWITEM_H
#define IGNORELISTVIEWITEM_H

#include <QString>

#include <QTreeWidget>


class IgnoreListViewItem : public QTreeWidgetItem
{
    public:
        IgnoreListViewItem(QTreeWidget* parent, const QString& name, int flags);
        ~IgnoreListViewItem() override;

        void setFlag(int flag,bool active);
        bool getFlag(int flag) const { return m_flags & flag; };
        QString getName() const { return text(0); };
        void setName(const QString &name) { setText(0, name); };
        int getFlags() const { return m_flags; };

        void setFlags(int flags);

    private:
        int m_flags;

        Q_DISABLE_COPY(IgnoreListViewItem)
};

#endif
