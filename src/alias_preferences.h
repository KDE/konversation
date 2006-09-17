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

#ifndef EXALIASPREFERENCES_H
#define EXALIASPREFERENCES_H

#include "alias_preferencesui.h"
#include "konvisettingspage.h"

class Alias_Config : public Alias_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

    public:
        explicit Alias_Config(QWidget* parent, const char* name = 0);
        ~Alias_Config();

        virtual void saveSettings();
        virtual void loadSettings();
        virtual void restorePageToDefaults();
        virtual bool hasChanged();

    signals:
        void modified();

    protected slots:
        void entrySelected(QListViewItem* aliasEntry);
        void nameChanged(const QString& newName);
        void actionChanged(const QString& newAction);
        void addEntry();
        void removeEntry();

    protected:
        void setAliasListView(const QStringList& aliasList);

        bool m_newItemSelected;

        QStringList m_oldAliasList;
        QStringList currentAliasList();
};

#endif
