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

#include "connectionbehavior_config.h"
#include "application.h"
#include "preferences.h"

#include <KLineEdit>
#include <kwallet.h>

#include <config-konversation.h>

ConnectionBehavior_Config::ConnectionBehavior_Config(QWidget* parent)
    : QWidget(parent), m_passwordChanged(false)
{
    setupUi(this);

#ifndef HAVE_QCA2
    kcfg_EncryptionType->setDisabled(true);
#endif

    kcfg_ReconnectDelay->setSuffix(ki18np(" second", " seconds"));

    connect(m_ProxyPassword, SIGNAL(textChanged(QString)), this, SLOT(setPasswordChanged()));
}

void ConnectionBehavior_Config::restorePageToDefaults()
{
    m_ProxyPassword->clear();
}

void ConnectionBehavior_Config::saveSettings()
{
    if(kcfg_ProxyEnabled->isChecked () && m_passwordChanged)
    {
        if(Application::instance()->wallet())
        {
            int ret = Application::instance()->wallet()->writePassword("ProxyPassword", m_ProxyPassword->text());

            if(ret != 0)
            {
                kError() << "Failed to write the proxy password to the wallet, error code:" << ret;
            }
        }
    }

    setPasswordChanged(false);
}

void ConnectionBehavior_Config::loadSettings()
{
    QString password;

    if(Preferences::self()->proxyEnabled())
    {
        if(Application::instance()->wallet())
        {
            int ret = Application::instance()->wallet()->readPassword("ProxyPassword", password);

            if(ret != 0)
            {
                kError() << "Failed to read the proxy password from the wallet, error code:" << ret;
            }
        }
    }

    m_ProxyPassword->setText(password);
    setPasswordChanged(false);
}

bool ConnectionBehavior_Config::hasChanged()
{
    return m_passwordChanged;
}

void ConnectionBehavior_Config::setPasswordChanged(bool changed)
{
    m_passwordChanged = changed;
}

#include "connectionbehavior_config.moc"
