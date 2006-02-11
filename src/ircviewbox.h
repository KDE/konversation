/* ============================================================
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2005-06-02
 * Copyright 2005 by Renchi Raju
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * ============================================================ */

#ifndef IRCVIEWBOX_H
#define IRCVIEWBOX_H

#include <qvbox.h>

class IRCView;
class SearchBar;
class Server;

class IRCViewBox : public QVBox
{
    Q_OBJECT

        public:

        IRCViewBox(QWidget* parent, Server* newServer);
        ~IRCViewBox();

        IRCView*   ircView() const;

    public slots:

        void slotSearch();
        void slotSearchNext();
        void slotSearchPrevious();
        void slotSearchChanged(const QString& pattern);

    protected:
        void searchNext(bool reversed = false);

    private:

        IRCView*   m_ircView;
        SearchBar* m_searchBar;
        bool       m_matchedOnce;
};
#endif                                            /* IRCVIEWBOX_H */
