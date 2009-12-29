/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Renchi Raju <renchi@pooh.tam.uiuc.edu>
  Copyright (C) 2006 Peter Simonsson <psn@linux.se>
*/

#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include "ui_searchbarbase.h"

/* TODO:
   - Changing case-sensitivity and search-forward restarts search from beginning.
     fix to continue search from current position
   - figure out what "from cursor" and "whole words" means and is it important for
     the konvi gods
 */

class KMenu;
class KIcon;

class SearchBar : public QWidget, private Ui::SearchBarBase
{
    Q_OBJECT

    public:
        explicit SearchBar(QWidget* parent);
        ~SearchBar();

        void setHasMatch(bool value);

        QString pattern() const;

        bool searchForward() const;
        bool caseSensitive() const;
        bool wholeWords() const;
        bool fromCursor() const;

    protected:
        virtual void showEvent(QShowEvent* e);
        virtual void hideEvent(QHideEvent* e);
        bool focusedChild();

    private slots:
        void slotTextChanged();
        void slotFind();
        void slotFindNext();
        void slotFindPrevious();

        void toggleSearchFoward(bool value);
        void toggleMatchCase(bool value);
        void toggleWholeWords(bool value);
        void toggleFromCursor(bool value);

        void showOptionsMenu();

    signals:
        void signalSearchChanged(const QString& pattern);
        void signalSearchNext();
        void signalSearchPrevious();
        void signalPropertiesChanged();
        void hidden();

    private:
        QTimer* m_timer;

        KMenu* m_optionsMenu;
        KIcon m_goUpSearch;
        KIcon m_goDownSearch;

        bool m_searchFoward;
        bool m_matchCase;
        bool m_wholeWords;
        bool m_fromCursor;
};

#endif                                            /* SEARCHBAR_H */
