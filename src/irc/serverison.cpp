/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Fri Sep 03 2004
  copyright: (C) 2004 by John Tapsell
  email:     john@geola.co.uk
*/

#include "serverison.h"
#include "server.h"
#include "application.h"
#include "nickinfo.h"
#include "viewcontainer.h"

ServerISON::ServerISON(Server* server) : m_server(server)
{
    m_ISONList_invalid = true;

    connect( m_server, SIGNAL(nickInfoChanged(Server*,NickInfoPtr)),
        this, SLOT(nickInfoChanged(Server*,NickInfoPtr)));
    connect( m_server,
        SIGNAL(channelMembersChanged(Server*,QString,bool,bool,QString)),
        this,
        SLOT(slotChannelMembersChanged(Server*,QString,bool,bool,QString)));
    connect( m_server,
        SIGNAL(channelJoinedOrUnjoined(Server*,QString,bool)),
        this,
        SLOT(slotChannelJoinedOrUnjoined(Server*,QString,bool)));
    connect(Application::instance(), SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)),
        this, SLOT(slotServerGroupsChanged()));
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

        QStringList prefsWatchList =
            Preferences::notifyListByGroupId(m_server->getServerGroup()->id());
        QStringList::iterator itEnd = prefsWatchList.end();

        for(QStringList::iterator it = prefsWatchList.begin(); it != itEnd; ++it)
        {
            ISONMap.insert((*it).toLower(), (*it));
        }

        // Build final watch list.
        m_watchList = ISONMap.values();
        // Eliminate nicks that are online in a joined channel, since there is no point
        // in doing an ISON on such nicks.
        m_ISONList.clear();
        itEnd = m_watchList.end();

        for(QStringList::iterator it = m_watchList.begin(); it != itEnd; ++it)
        {
            if (m_server->getNickJoinedChannels(*it).isEmpty())
            {
                m_ISONList.append(*it);
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
    qDebug();
    m_ISONList_invalid = true;
}

void ServerISON::nickInfoChanged(Server* /*server*/, const NickInfoPtr /*nickInfo*/) {
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


