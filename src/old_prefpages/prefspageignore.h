/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides an interface to the ignore list
  begin:     Fre Jun 13 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGEIGNORE_H
#define PREFSPAGEIGNORE_H

#include <qptrlist.h>

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class QLineEdit;
class QListViewItem;
class QPushButton;

class KListView;

class Ignore;
class IgnoreCheckBox;

class PrefsPageIgnore : public PrefsPage
{
    Q_OBJECT

        public:
        PrefsPageIgnore(QWidget* newParent,Preferences* newPreferences);
        ~PrefsPageIgnore();

        signals:
        void applyClicked(QPtrList<Ignore> newList);
        void cancelClicked(QSize newSize);

    public slots:
        void applyPreferences();

    protected slots:
        void newIgnore();
        void removeIgnore();
        void select(QListViewItem* item);
        void checked(int flag,bool active);

    protected:
        QPtrList<Ignore> getIgnoreList();

        KListView* ignoreListView;
        QLineEdit* ignoreInput;
        QPushButton* newButton;
        QPushButton* removeButton;
        QPushButton* clearButton;
        QPtrList<IgnoreCheckBox> checkList;
};
#endif
