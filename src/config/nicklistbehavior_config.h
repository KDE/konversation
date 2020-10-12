/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef NICKLISTBEHAVIOR_CONFIG_H
#define NICKLISTBEHAVIOR_CONFIG_H

#include "settingspage.h"
#include "ui_nicklistbehavior_configui.h"


class NicklistBehavior_Config : public QWidget, public KonviSettingsPage, private Ui::NicklistBehavior_ConfigUI
{
    Q_OBJECT

    public:
        explicit NicklistBehavior_Config(QWidget *parent = nullptr, const char *name = nullptr);
        ~NicklistBehavior_Config();

        void saveSettings() override;
        void loadSettings() override;
        void restorePageToDefaults() override;

        bool hasChanged() override;

    private:
        void setNickList(const QString &sortingOrder);
        QString currentSortingOrder() const;

        QString m_oldSortingOrder;

    Q_SIGNALS:
        void modified();
};

#endif
