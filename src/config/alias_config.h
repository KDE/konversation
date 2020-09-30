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

#ifndef ALIAS_CONFIG_H
#define ALIAS_CONFIG_H

#include "ui_alias_configui.h"
#include "settingspage.h"


class Alias_Config : public QWidget, public KonviSettingsPage, private Ui::Alias_ConfigUI
{
    Q_OBJECT

    public:
        explicit Alias_Config(QWidget* parent, const char* name = nullptr);
        ~Alias_Config();

        void saveSettings() override;
        void loadSettings() override;
        void restorePageToDefaults() override;
        bool hasChanged() override;

    Q_SIGNALS:
        void modified();

    protected Q_SLOTS:
        void entrySelected(QTreeWidgetItem* aliasEntry);
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
