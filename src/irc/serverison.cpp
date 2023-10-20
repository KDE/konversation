/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 John Tapsell <john@geola.co.uk>
*/

#include "serverison.h"
#include "server.h"
#include "application.h"
#include "viewcontainer.h"
#include "konversation_log.h"

ServerISON::ServerISON(Server* server) : m_server(server)
{
    m_ISONList_invalid = true;

    connect( m_server, QOverload<Server*, NickInfoPtr>::of(&Server::nickInfoChanged),
        this, &ServerISON::nickInfoChanged);
    connect( m_server,
        &Server::channelMembersChanged,
        this,
        &ServerISON::slotChannelMembersChanged);
    connect( m_server,
        &Server::channelJoinedOrUnjoined,
        this,
        &ServerISON::slotChannelJoinedOrUnjoined);
    connect(Application::instance(), &Application::serverGroupsChanged,
        this, &ServerISON::slotServerGroupsChanged);
}

QStringList ServerISON::getWatchList()
{
    if(m_ISONList_invalid)
        recalculateAddressees();
    return m_watchList;
}

QStringList ServerISON::getISONList()
{
    if(m_ISONList_invalid)
        recalculateAddressees();
    return m_ISONList;
}

void ServerISON::recalculateAddressees()
{
    // If not watching nicks, no need to build notify list.
    if (Preferences::self()->useNotify())
    {
        QMap<QString,QString> ISONMap;

        const QStringList prefsWatchList =
            Preferences::notifyListByGroupId(m_server->getServerGroup()->id());

        for (const QString& prefsWatch : prefsWatchList) {
            ISONMap.insert(prefsWatch.toLower(), prefsWatch);
        }

        // Build final watch list.
        m_watchList = ISONMap.values();
        // Eliminate nicks that are online in a joined channel, since there is no point
        // in doing an ISON on such nicks.
        m_ISONList.clear();

        for (const QString& nickName : std::as_const(m_watchList)) {
            if (m_server->getNickJoinedChannels(nickName).isEmpty()) {
                m_ISONList.append(nickName);
            }
        }
    }
    else
    {
        m_ISONList.clear();
    }

    m_ISONList_invalid = false;
}

// When user changes preferences and has nick watching turned on, rebuild notify list.
void ServerISON::slotServerGroupsChanged()
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    m_ISONList_invalid = true;
}

void ServerISON::nickInfoChanged(Server* /*server*/, const NickInfoPtr /*nickInfo*/&) {
//We need to call recalculateAddressees before returning m_ISONList

//Maybe we could do something like:
//if(m_ISONList.contains(nickInfo->getNickName())) return;
m_ISONList_invalid = true;
}

void ServerISON::slotChannelMembersChanged(Server* /*server*/, const QString& /*channelName*/,
bool joined, bool parted, const QString& nickname)
{
    // Whenever a nick on the watch list leaves the last joined channel, must recalculate lists.
    // The nick will be added to the ISON list.
    if (joined && parted && m_watchList.contains(nickname))
        if (m_server->getNickJoinedChannels(nickname).isEmpty()) m_ISONList_invalid = true;
}

void ServerISON::slotChannelJoinedOrUnjoined(Server* /*server*/,
const QString& /*channelName*/, bool /*joined*/)
{
    // If user left or joined a channel, need to recalculate lists, since watched nicks
    // may need to be moved from/to ISON list.
    m_ISONList_invalid = true;
}

#include "moc_serverison.cpp"
