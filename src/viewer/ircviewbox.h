/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Renchi Raju <renchi@pooh.tam.uiuc.edu>
*/

#ifndef IRCVIEWBOX_H
#define IRCVIEWBOX_H

#include <QWidget>

class IRCView;
class SearchBar;

class IRCViewBox : public QWidget
{
    Q_OBJECT

    public:

        explicit IRCViewBox(QWidget* parent);
        ~IRCViewBox() override;

        IRCView*   ircView() const;

    public Q_SLOTS:

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
