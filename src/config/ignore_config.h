/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef IGNORE_CONFIG_H
#define IGNORE_CONFIG_H

#include "ui_ignore_configui.h"
#include "settingspage.h"

class Ignore;
class Ignore_Config : public QWidget, public KonviSettingsPage, private Ui::Ignore_ConfigUI
{
    Q_OBJECT

    public:
        explicit Ignore_Config( QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags fl = {} );
        ~Ignore_Config() override;

        void restorePageToDefaults() override;
        void saveSettings() override;
        void loadSettings() override;

        bool hasChanged() override;

    public Q_SLOTS:
        virtual void languageChange();

    Q_SIGNALS:
        void modified();

    private Q_SLOTS:
        void newIgnore();
        void removeIgnore();
        void flagCheckboxChanged();
        void select(QTreeWidgetItem* item);
        void removeAllIgnore();

    private:
        QStringList currentIgnoreList() const;  // in hasChanged() format
        QList<Ignore*> getIgnoreList() const; // in prefs format
        void updateEnabledness();

    private:
        QStringList m_oldIgnoreList;

        Q_DISABLE_COPY(Ignore_Config)
};

#endif // IGNORE_CONFIG_H
