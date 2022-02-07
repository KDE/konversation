/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2022 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#ifndef LAUNCHERENTRY_CONFIG_H
#define LAUNCHERENTRY_CONFIG_H

#include "ui_launcherentry_config.h"
#include "settingspage.h"

class LauncherEntry_Config : public QWidget,
                             public KonviSettingsPage,
                             private Ui::LauncherEntry_Config
{
    Q_OBJECT

public:
    explicit LauncherEntry_Config(QWidget* parent = nullptr, const char* name = nullptr);
    ~LauncherEntry_Config() override;

    void restorePageToDefaults() override;
    void saveSettings() override;
    void loadSettings() override;

    bool hasChanged() override;  // implement the interface, will not be used here, though

private:
    Q_DISABLE_COPY(LauncherEntry_Config)
};

#endif
