/* ============================================================
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2005-06-02
 * Copyright 2005 by Renchi Raju
 * Copyright (C) 2006 Peter Simonsson <psn@linux.se>
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

#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include "searchbarbase.h"

/* TODO:
   - Changing case-sensitivity and search-forward restarts search from beginning.
     fix to continue search from current position
   - figure out what "from cursor" and "whole words" means and is it important for
     the konvi gods
 */

class QPopupMenu;

class SearchBar : public SearchBarBase
{
    Q_OBJECT

    public:
        SearchBar(QWidget* parent);
        ~SearchBar();

        void setHasMatch(bool value);
        void setStatus(const QPixmap& pix, const QString& text);

        QString pattern() const;

        bool searchForward() const;
        bool caseSensitive() const;
        bool wholeWords() const;
        bool fromCursor() const;

    protected:
        virtual void showEvent(QShowEvent* e);
        bool focusedChild();

    public slots:
        virtual void hide();

    private slots:
        void slotTextChanged();
        void slotFind();
        void slotFindNext();
        void slotFindPrevious();

        void toggleSearchFoward();
        void toggleMatchCase();
        void toggleWholeWords();
        void toggleFromCursor();

        void showOptionsMenu();

    signals:
        void signalSearchChanged(const QString& pattern);
        void signalSearchNext();
        void signalSearchPrevious();
        void signalPropertiesChanged();
        void hidden();

    private:
        QTimer* m_timer;

        QPopupMenu* m_optionsMenu;

        bool m_searchFoward;
        bool m_matchCase;
        bool m_wholeWords;
        bool m_fromCursor;
};

#endif                                            /* SEARCHBAR_H */
