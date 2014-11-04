/*
Copyright 2009  Peter Simonsson <peter.simonsson@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
        explicit ConnectionBehavior_Config(QWidget* parent = NULL);

        virtual void restorePageToDefaults();
        virtual void saveSettings();
        virtual void loadSettings();

        virtual bool hasChanged();

    protected Q_SLOTS:
        void setPasswordChanged(bool changed = true);

    private:
        bool m_passwordChanged;
};

#endif // CONNECTIONBEHAVIOR_CONFIG_H
