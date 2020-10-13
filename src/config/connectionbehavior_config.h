/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#ifndef CONNECTIONBEHAVIOR_CONFIG_H
#define CONNECTIONBEHAVIOR_CONFIG_H

#include "ui_connectionbehavior_config.h"
#include "settingspage.h"

#include <QWidget>

class ConnectionBehavior_Config : public QWidget, public KonviSettingsPage, private Ui::ConnectionBehavior_Config
{
    Q_OBJECT
    public:
        explicit ConnectionBehavior_Config(QWidget* parent = nullptr);

        void restorePageToDefaults() override;
        void saveSettings() override;
        void loadSettings() override;

        bool hasChanged() override;

    private Q_SLOTS:
        void setPasswordChanged(bool changed = true);

    private:
        bool m_passwordChanged;
};

#endif // CONNECTIONBEHAVIOR_CONFIG_H
