/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2011 Eike Hein <hein@kde.org>
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

    QStringList splitString = input.parameter.split(' ');
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
    else if (!m_target.contains('.'))
    {
        if (m_chatWindow->getServer())
            m_chatWindow->getServer()->resolveUserhost(m_target);

        deleteLater();

        return;
    }

    QHostInfo::lookupHost(m_target, this, SLOT(resolved(QHostInfo)));
}

OutputFilterResolveJob::~OutputFilterResolveJob()
{
}

void OutputFilterResolveJob::resolved(QHostInfo hostInfo)
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
