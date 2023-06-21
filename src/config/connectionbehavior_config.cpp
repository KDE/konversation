/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "connectionbehavior_config.h"

#include "application.h"
#include "preferences.h"
#include "konversation_log.h"
#include <config-konversation.h>

#include <KLineEdit>
#include <KAuthorized>
#include <KWallet>
#include <KLocalizedString>


ConnectionBehavior_Config::ConnectionBehavior_Config(QWidget* parent)
    : QWidget(parent), m_passwordChanged(false)
{
    setupUi(this);

#if !HAVE_QCA2
    kcfg_EncryptionType->setDisabled(true);
#endif

    kcfg_ReconnectDelay->setSuffix(ki18np(" second", " seconds"));
    m_ProxyPassword->setRevealPasswordAvailable(KAuthorized::authorize(QStringLiteral("lineedit_reveal_password")));
    connect(m_ProxyPassword, &KPasswordLineEdit::passwordChanged, this, [this]() { setPasswordChanged(); });
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
            int ret = Application::instance()->wallet()->writePassword(QStringLiteral("ProxyPassword"), m_ProxyPassword->password());

            if(ret != 0)
            {
                qCCritical(KONVERSATION_LOG) << "Failed to write the proxy password to the wallet, error code:" << ret;
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
            int ret = Application::instance()->wallet()->readPassword(QStringLiteral("ProxyPassword"), password);

            if(ret != 0)
            {
                qCCritical(KONVERSATION_LOG) << "Failed to read the proxy password from the wallet, error code:" << ret;
            }
        }
    }

    m_ProxyPassword->setPassword(password);
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

#include "moc_connectionbehavior_config.cpp"
