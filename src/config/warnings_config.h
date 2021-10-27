/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef WARNINGS_CONFIG_H
#define WARNINGS_CONFIG_H

#include "ui_warnings_configui.h"
#include "settingspage.h"


class Warnings_Config : public QWidget, public KonviSettingsPage, private Ui::Warnings_ConfigUI
{
    Q_OBJECT

    public:
        explicit Warnings_Config( QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags fl = {} );
        ~Warnings_Config() override;

        void restorePageToDefaults() override;
        void saveSettings() override;
        void loadSettings() override;

        bool hasChanged() override;

    public Q_SLOTS:
        virtual void languageChange();

    Q_SIGNALS:
        void modified();

    private:
        QString currentWarningsChecked() const; // for hasChanged()

    private:
        QString m_oldWarningsChecked;     // for hasChanged()

        Q_DISABLE_COPY(Warnings_Config)
};

#endif // WARNINGS_CONFIG_H
