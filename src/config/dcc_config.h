/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#ifndef DCC_CONFIG_H
#define DCC_CONFIG_H

#include "ui_dcc_configui.h"

class DCC_Config : public QWidget, private Ui::DCC_ConfigUI
{
    Q_OBJECT

    public:
        DCC_Config(QWidget* parent, const char* name);
        ~DCC_Config() override;

    protected:
        void showEvent(QShowEvent *event) override;

    private Q_SLOTS:
        void languageChange();
        void dccMethodChanged(int index);
        void dccUPnPChanged(int state);

    private:
        Q_DISABLE_COPY(DCC_Config)
};

#endif
