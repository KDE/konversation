/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    nicklisttooltip.h  -  The class that controls what the tooltip looks like when you hover over a person in the nicklistview.  This is used to show contact information about the person from the addressbook.
    begin:     Sun 25 July 2004
    copyright: (C) 2004 by John Tapsell
    email:     john@geola.co.uk
*/

#ifndef KONVERSATIONNICKLISTTOOLTIP_H
#define KONVERSATIONNICKLISTTOOLTIP_H

#include "addressbook.h"

#include <qtooltip.h>
#include <qwidget.h>
#include <qpoint.h>
#include <qstring.h>

#include <klocale.h>


class NickListView;

namespace Konversation
{

    class KonversationNickListViewToolTip : public QToolTip
    {
        public:
            KonversationNickListViewToolTip(QWidget *parent, NickListView *lv);
            virtual ~KonversationNickListViewToolTip();

            void maybeTip( const QPoint &pos );

        private:
            NickListView *m_listView;
    };
}
#endif
