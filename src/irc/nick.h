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

#include <K3ListView>

class NickListView;

class Nick : public K3ListViewItem
{
    public:
        Nick(NickListView *listView,
            const ChannelNickPtr& channelnick);
        ~Nick();

        ChannelNickPtr getChannelNick() const;

        virtual void paintCell(QPainter * p, const QColorGroup & cg, int column, int width, int align);
        virtual int compare(Q3ListViewItem* item,int col,bool ascending) const;

        void refresh();

    signals:
        void refreshed();

    protected:
        QString calculateLabel1();
        QString calculateLabel2();

        int getSortingValue() const;

    protected:
        ChannelNickPtr m_channelnickptr;
        NickListView* m_nickListView;

//        int m_height;
        int m_flags;
};
#endif
