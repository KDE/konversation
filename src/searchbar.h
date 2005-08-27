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

#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include <qhbox.h>

/* TODO:
   - Changing case-sensitivity and search-forward restarts search from beginning.
     fix to continue search from current position
   - figure out what "from cursor" and "whole words" means and is it important for
     the konvi gods
 */

class QLineEdit;
class QCheckBox;
class QPushButton;
class QTimer;
class QLabel;
class QPixmap;

class SearchBar : public QHBox
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

    protected:

        void focusInEvent(QFocusEvent* e);

    public slots:

        virtual void hide();

    private slots:

        void slotTextChanged();
        void slotFind();
        void slotFindNext();

        signals:

        void signalSearchChanged(const QString& pattern);
        void signalSearchNext();
        void signalPropertiesChanged();

    private:

        QLineEdit*    m_lineEdit;
        QPushButton*  m_nextBtn;
        QCheckBox*  m_fwdBox;
        QCheckBox*  m_caseSenBox;
        QPushButton*  m_hideBtn;
        QLabel*       m_statusPixLabel;
        QLabel*       m_statusTextLabel;

        QTimer*       m_timer;
};
#endif                                            /* SEARCHBAR_H */
