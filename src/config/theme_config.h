/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2007 Eike Hein <hein@kde.org>
*/

#ifndef THEME_CONFIG_H
#define THEME_CONFIG_H

#include "ui_theme_configui.h"
#include "settingspage.h"

#include <KIO/Job>


class Theme_Config : public QWidget, public KonviSettingsPage, private Ui::Theme_ConfigUI
{
    Q_OBJECT

    public:
        explicit Theme_Config(QWidget* parent, const char* name = nullptr);
        ~Theme_Config() override;

        void restorePageToDefaults() override;
        void saveSettings() override;
        void loadSettings() override;

        bool hasChanged() override;

    Q_SIGNALS:
        void modified();

    private Q_SLOTS:
        void updatePreview(int id);
        void updateButtons();
        void installTheme();
        void removeTheme();
        void gotNewSchemes();
        void postRemoveTheme(KJob* delete_job);

    private:
        QStringList m_dirs;
        QString m_oldTheme;
        QString m_currentTheme;
        int m_defaultThemeIndex;

        Q_DISABLE_COPY(Theme_Config)
};

#endif
