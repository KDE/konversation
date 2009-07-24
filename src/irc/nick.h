/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Fri Jan 25 2002
  copyright: (C) 2002 by Dario Abatianni
             (C) 2007 Peter Simonsson <peter.simonsson@gmail.com>
  email:     eisfuchs@tigress.com
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
        ~Nick();

        ChannelNickPtr getChannelNick() const;

        virtual QVariant data(int column, int role) const;
        virtual bool operator<(const QTreeWidgetItem& other) const;

        void refresh();
        void repositionMe();
        // Compatibility with Qt 4.4 + make it public for >= Qt 4.5
        void emitDataChanged();

    protected:
        QString calculateLabel1();
        QString calculateLabel2();

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

    private:
#if (QT_VERSION < QT_VERSION_CHECK(4, 5, 0))
        bool m_emitDataChanged;
#endif
};
#endif
