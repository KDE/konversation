/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Renchi Raju <renchi@pooh.tam.uiuc.edu>
*/

#ifndef IRCVIEWBOX_H
#define IRCVIEWBOX_H

#include <kvbox.h>

class IRCView;
class SearchBar;
class Server;

class IRCViewBox : public KVBox
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
