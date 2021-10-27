/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
*/

#ifndef TABS_CONFIG_H
#define TABS_CONFIG_H

#include "ui_tabs_configui.h"


class Tabs_Config : public QWidget, private Ui::Tabs_PreferencesUI
{
    Q_OBJECT

    public:
        explicit Tabs_Config(QWidget *parent = nullptr, const char *name = nullptr);
        ~Tabs_Config() override;

    protected:
        void showEvent(QShowEvent *event) override;

    private Q_SLOTS:
        void toggleCheckBoxes(int activated);

    private:
        Q_DISABLE_COPY(Tabs_Config)
};

#endif
