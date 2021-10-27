/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
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
        ~Alias_Config() override;

        void saveSettings() override;
        void loadSettings() override;
        void restorePageToDefaults() override;
        bool hasChanged() override;

    Q_SIGNALS:
        void modified();

    private Q_SLOTS:
        void entrySelected(QTreeWidgetItem* aliasEntry);
        void nameChanged(const QString& newName);
        void actionChanged(const QString& newAction);
        void addEntry();
        void removeEntry();

    private:
        void setAliasListView(const QStringList& aliasList);
        QStringList currentAliasList() const;

    private:
        bool m_newItemSelected;
        QStringList m_oldAliasList;

        Q_DISABLE_COPY(Alias_Config)
};

#endif
