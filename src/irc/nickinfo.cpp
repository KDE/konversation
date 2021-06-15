/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Gary Cramblitt <garycramblitt@comcast.net>
*/

/*
  The NickInfo object is a data container for information about a single nickname.
  It is owned by the Server object and should NOT be deleted by anything other than Server.
  Store a pointer to this with NickInfoPtr
*/

#include "nickinfo.h"
#include "application.h"
#include "mainwindow.h"
#include "server.h"

NickInfo::NickInfo(const QString& nick, Server* server): QSharedData()
{
    m_nickname = nick;
    m_loweredNickname = nick.toLower();
    m_owningServer = server;
    m_away = false;
    m_identified = false;
    m_printedOnline = false;
    m_changed = false;

    // reset nick color
    m_nickColor = 0;
}

NickInfo::~NickInfo()
{
}

// Get properties of NickInfo object.
QString NickInfo::getNickname() const
{
    return m_nickname;
}

QString NickInfo::loweredNickname() const
{
    return m_loweredNickname;
}

QString NickInfo::getHostmask() const
{
    return m_hostmask;
}

bool NickInfo::isAway() const
{
    return m_away;
}

QString NickInfo::getAwayMessage() const
{
    return m_awayMessage;
}

QString NickInfo::getRealName() const
{
    return m_realName;
}

QString NickInfo::getNetServer() const
{
    return m_netServer;
}

QString NickInfo::getNetServerInfo() const
{
    return m_netServerInfo;
}

QDateTime NickInfo::getOnlineSince() const
{
    return m_onlineSince;
}

uint NickInfo::getNickColor() const
{
    // do we already have a color?
    if (!m_nickColor) m_nickColor = Konversation::colorForNick(m_nickname) + 1;

    // return color offset -1 (since we store it +1 for 0 checking)
    return m_nickColor-1;
}

bool NickInfo::isIdentified() const
{
    return m_identified;
}

QString NickInfo::getPrettyOnlineSince() const
{
    return QLocale().toString(m_onlineSince, QLocale::LongFormat);
}

// Return the Server object that owns this NickInfo object.
Server* NickInfo::getServer() const
{
    return m_owningServer;
}

// Set properties of NickInfo object.
void NickInfo::setNickname(const QString& newNickname)
{
    Q_ASSERT(!newNickname.isEmpty());
    if(newNickname == m_nickname) return;

    m_nickname = newNickname;
    m_loweredNickname = newNickname.toLower();

    //QString realname = m_addressee.realName(); //TODO why the fuck is this called?
    startNickInfoChangedTimer();
}

void NickInfo::startNickInfoChangedTimer()
{
    setChanged(true);
    m_owningServer->startNickInfoChangedTimer();
}

void NickInfo::setHostmask(const QString& newMask)
{
    if (newMask.isEmpty() || newMask == m_hostmask) return;
    m_hostmask = newMask;

    startNickInfoChangedTimer();
}

void NickInfo::setAway(bool state)
{
    if(state == m_away) return;
    m_away = state;

    startNickInfoChangedTimer();
}

void NickInfo::setIdentified(bool identified)
{
    if(identified == m_identified) return;
    m_identified = identified;
    startNickInfoChangedTimer();
}

void NickInfo::setAwayMessage(const QString& newMessage)
{
    if(m_awayMessage == newMessage) return;
    m_awayMessage = newMessage;

    startNickInfoChangedTimer();
}

void NickInfo::setRealName(const QString& newRealName)
{
    if (newRealName.isEmpty() || m_realName == newRealName) return;
    m_realName = newRealName;
    startNickInfoChangedTimer();
}

void NickInfo::setNetServer(const QString& newNetServer)
{
    if (newNetServer.isEmpty() || m_netServer == newNetServer) return;
    m_netServer = newNetServer;
    startNickInfoChangedTimer();
}

void NickInfo::setNetServerInfo(const QString& newNetServerInfo)
{
    if (newNetServerInfo.isEmpty() || newNetServerInfo == m_netServerInfo) return;
    m_netServerInfo = newNetServerInfo;
    startNickInfoChangedTimer();
}

void NickInfo::setOnlineSince(const QDateTime& datetime)
{
    if (datetime.isNull() || datetime == m_onlineSince) return;
    m_onlineSince = datetime;

    startNickInfoChangedTimer();
}

QString NickInfo::tooltip() const
{

    QString strTooltip;
    QTextStream tooltip( &strTooltip, QIODevice::WriteOnly );
    tooltip << "<qt>";

    tooltip << R"(<table cellspacing="5" cellpadding="0">)";
    tooltipTableData(tooltip);
    tooltip << "</table></qt>";
    return strTooltip;
}


QString NickInfo::getBestAddresseeName() const
{
    if(!getRealName().isEmpty())
    {
        return getRealName();
    }
    else
    {
        return getNickname();
    }
}

void NickInfo::tooltipTableData(QTextStream &tooltip) const
{
    if(!getHostmask().isEmpty())
    {
        tooltip << "<tr><td><b>" << i18n("Hostmask:") << "</b></td><td>" << getHostmask() << "</td></tr>";
    }
    if(!m_account.isEmpty())
    {
        tooltip << "<tr><td><b>" << i18n("Account:") << "</b></td><td>" << m_account << "</td></tr>";
    }
    if(isAway())
    {
        tooltip << "<tr><td><b>" << i18n("Away&nbsp;Message:") << "</b></td><td>";
        if(!getAwayMessage().isEmpty())
            tooltip << getAwayMessage();
        else
            tooltip << i18n("(unknown)");
        tooltip << "</td></tr>";
    }
    if(!getOnlineSince().toString().isEmpty())
    {
        tooltip << "<tr><td><b>" << i18n("Online&nbsp;Since:") << "</b></td><td>" << getPrettyOnlineSince() << "</td></tr>";
    }

}

void NickInfo::setPrintedOnline(bool printed)
{
    m_printedOnline=printed;
}

bool NickInfo::getPrintedOnline() const
{
    return m_printedOnline;
}

void NickInfo::setAccount(const QString &name)
{
    if (name == m_account)
        return;

    m_account = name;

    startNickInfoChangedTimer();
}
