/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef IGNORE_CONFIG_H
#define IGNORE_CONFIG_H

#include "ignore_preferencesui.h"
#include "settingspage.h" ////// header renamed

#include <q3ptrlist.h>


class Ignore;
class Ignore_Config : public Ignore_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

    public:
        explicit Ignore_Config( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
        ~Ignore_Config();
        QString flagNames;

        virtual void restorePageToDefaults();
        virtual void saveSettings();
        virtual void loadSettings();

        virtual bool hasChanged();

    private:
        QStringList m_oldIgnoreList;

        QStringList currentIgnoreList();  // in hasChanged() format
        Q3PtrList<Ignore> getIgnoreList(); // in prefs format
        void updateEnabledness();

    public slots:
        virtual void languageChange();

    protected slots:
        void newIgnore();
        void removeIgnore();
        void flagCheckboxChanged();
        void select(Q3ListViewItem* item);
        void removeAllIgnore();
    signals:
        void modified();
};

#endif // IGNORE_CONFIG_H
