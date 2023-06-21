/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2011 Eike Hein <hein@kde.org>
*/

#include "outputfilterresolvejob.h"
#include "chatwindow.h"
#include "server.h"


OutputFilterResolveJob::OutputFilterResolveJob(const Konversation::OutputFilterInput& input)
{
    m_reverse = false;
    m_chatWindow = input.context;

    if (!m_chatWindow)
    {
        deleteLater();

        return;
    }

    const QStringList splitString = input.parameter.split(QLatin1Char(' '));
    m_target = splitString[0];

    QHostAddress address(m_target);

    // Parameter is an IP address
    if (address != QHostAddress::Null)
    {
        m_target = address.toString();
        m_reverse = true;
    }
    // If it doesn't contain a dot, we assume it's neither a host nor an IP, but
    // instead a nickname.
    else if (!m_target.contains(QLatin1Char('.'))) {
        if (m_chatWindow->getServer())
            m_chatWindow->getServer()->resolveUserhost(m_target);

        deleteLater();

        return;
    }

    QHostInfo::lookupHost(m_target, this, &OutputFilterResolveJob::resolved);
}

OutputFilterResolveJob::~OutputFilterResolveJob()
{
}

void OutputFilterResolveJob::resolved(const QHostInfo &hostInfo)
{
    QString result;

    if (hostInfo.error() == QHostInfo::NoError)
    {
        if (m_reverse)
            result = hostInfo.hostName();
        else if (!hostInfo.addresses().isEmpty())
            result = hostInfo.addresses().first().toString();
    }

    if (!result.isEmpty())
        m_chatWindow->appendServerMessage(i18n("DNS"), i18n("Resolved %1 to: %2", m_target, result));
    else
        m_chatWindow->appendServerMessage(i18n("Error"), i18n("Unable to resolve %1.", m_target));

    deleteLater();
}

#include "moc_outputfilterresolvejob.cpp"
