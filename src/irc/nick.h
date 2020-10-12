/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2007 Peter Simonsson <peter.simonsson@gmail.com>
*/

#ifndef NICK_H
#define NICK_H

#include "channelnick.h"

#include <QTreeWidgetItem>

class NickListView;
class Channel;

class Nick : public QTreeWidgetItem
{
    public:
        Nick(NickListView *listView, Channel* channel,
            const ChannelNickPtr& channelnick);
        ~Nick() override;

        ChannelNickPtr getChannelNick() const;

        QVariant data(int column, int role) const override;
        bool operator<(const QTreeWidgetItem& other) const override;

        void refresh();
        void repositionMe();

    protected:
        QString calculateLabel1() const;
        QString calculateLabel2() const;

        int getSortingValue() const;

    protected:
        ChannelNickPtr m_channelnickptr;
        Channel* m_channel;

        int m_flags;

    public:
        enum Columns {
            NicknameColumn = 0,
            HostmaskColumn = 1
        };
};
#endif
