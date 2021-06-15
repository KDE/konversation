/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002, 2003, 2004 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "channelnick.h"

#include "channel.h"
#include "server.h"
#include "konversation_log.h"


ChannelNick::ChannelNick(const NickInfoPtr& nickInfo, const QString& channel)
: QSharedData()
{
    m_nickInfo = nickInfo;
    m_isop = false;
    m_isadmin = false;
    m_isowner = false;
    m_ishalfop = false;
    m_hasvoice = false;
    m_timeStamp = 0;
    m_recentActivity = 0;
    m_channel = channel;
    m_isChanged = false;
}

ChannelNick::~ChannelNick()
{
}

bool ChannelNick::isOp() const
{
  return m_isop;
}

bool ChannelNick::isAdmin() const
{
  return m_isadmin;
}

bool ChannelNick::isOwner() const
{
  return m_isowner;
}

bool ChannelNick::isHalfOp() const
{
  return m_ishalfop;
}

bool ChannelNick::hasVoice() const
{
  return m_hasvoice;
}

bool ChannelNick::isAnyTypeOfOp() const
{
  return m_isop || m_isadmin || m_isowner || m_ishalfop;
}

NickInfoPtr ChannelNick::getNickInfo() const
{
  return m_nickInfo;
}

/** @param mode 'v' to set voice, 'a' to set admin, 'h' to set halfop, 'o' to set op.
 *  @param state what to set the mode to.
 */
bool ChannelNick::setMode(char mode, bool state)
{
    switch (mode)
    {
        case 'q':
            return setOwner(state);
        case 'a':
            return setAdmin(state);
        case 'o':
            return setOp(state);
        case 'h':
            return setHalfOp(state);
        case 'v':
            return setVoice(state);
        default:
            qCDebug(KONVERSATION_LOG) << "Mode '" << mode << "' not recognised in setModeForChannelNick";
            return false;
    }
}

/** Used still for passing modes from inputfilter to Server.  Should be removed.
 */
bool ChannelNick::setMode(uint mode)
{
    bool voice = mode%2;
    mode >>= 1;
    bool halfop = mode %2;
    mode >>= 1;
    bool op = mode %2;
    mode >>= 1;
    bool owner = mode %2;
    mode >>= 1;
    bool admin = mode %2;
    return setMode(admin, owner, op, halfop, voice);
}

bool ChannelNick::setMode(bool admin,bool owner,bool op,bool halfop,bool voice)
{
    if(m_isadmin==admin && m_isowner==owner && m_isop==op && m_ishalfop==halfop && m_hasvoice==voice)
        return false;
    m_isadmin=admin;
    m_isowner=owner;
    m_isop=op;
    m_ishalfop=halfop;
    m_hasvoice=voice;
    markAsChanged();
    return true;
}

/** set the voice for the nick, and update
 * @returns Whether it needed to be changed.  False for no change.
 */
bool ChannelNick::setVoice(bool state)
{
    if(m_hasvoice==state) return false;
    m_hasvoice=state;
    markAsChanged();
    return true;
}

bool ChannelNick::setOwner(bool state)
{
    if(m_isowner==state) return false;
    m_isowner=state;
    markAsChanged();
    return true;
}

bool ChannelNick::setAdmin(bool state)
{
    if(m_isadmin==state) return false;
    m_isadmin=state;
    markAsChanged();
    return true;
}

bool ChannelNick::setHalfOp(bool state)
{
    if(m_ishalfop==state) return false;
    m_ishalfop=state;
    markAsChanged();
    return true;
}

bool ChannelNick::setOp(bool state)
{
    if(m_isop==state) return false;
    m_isop=state;
    markAsChanged();
    return true;
}

//Purely provided for convience because they are used so often.
//Just calls nickInfo->getNickname() etc
QString ChannelNick::getNickname() const
{
    return m_nickInfo->getNickname();
}

QString ChannelNick::getHostmask() const
{
    return m_nickInfo->getHostmask();
}

QString ChannelNick::tooltip() const
{
    QString strTooltip;
    QTextStream tooltip( &strTooltip, QIODevice::WriteOnly );

    tooltip << "<qt>";

    tooltip << R"(<table cellspacing="5" cellpadding="0">)";

    m_nickInfo->tooltipTableData(tooltip);

    QStringList modes;
    if(isOp()) modes << i18n("Operator");
    if(isAdmin()) modes << i18n("Admin");
    if(isOwner()) modes << i18n("Owner");
    if(isHalfOp()) modes << i18n("Half-operator");
    if(hasVoice()) modes << i18n("Has voice");
    //Don't show anything if the user is just a normal user
    //if(modes.empty()) modes << i18n("A normal user");
    if(!modes.empty())
    {
        tooltip << "<tr><td><b>" << i18n("Mode") << ":</b></td><td>" << modes.join(QLatin1String(", ")) << "</td></tr>";
    }
    tooltip << "</table></qt>";
    //qCDebug(KONVERSATION_LOG) << strTooltip ;
    //if(!dirty) return QString();
    return strTooltip;
}

QString ChannelNick::loweredNickname() const
{
    return m_nickInfo->loweredNickname();
}

uint ChannelNick::timeStamp() const
{
  return m_timeStamp;
}

uint ChannelNick::recentActivity() const
{
    return m_recentActivity;
}

void ChannelNick::moreActive()
{
    m_recentActivity++;
}

void ChannelNick::lessActive()
{
    m_recentActivity--;
}

void ChannelNick::setTimeStamp(uint stamp)
{
  m_timeStamp = stamp;
}

void ChannelNick::markAsChanged()
{
    setChanged(true);
    m_nickInfo->getServer()->startChannelNickChangedTimer(m_channel);
}
