/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "nick.h"
#include "application.h"
#include "images.h"
#include "preferences.h"
#include "nicklistview.h"


Nick::Nick(NickListView *listView, Channel* channel, const ChannelNickPtr& channelnick)
    : QTreeWidgetItem (listView)
{
    m_channelnickptr = channelnick;
    m_channel = channel;

    Q_ASSERT(channelnick);
    Q_ASSERT(m_channel);

    m_flags = 0;

    refresh();

    setFlags((flags() & ~Qt::ItemIsDragEnabled) | Qt::ItemIsDropEnabled);
}

Nick::~Nick()
{
}

ChannelNickPtr Nick::getChannelNick() const
{
    Q_ASSERT(m_channelnickptr);
    return m_channelnickptr;
}

void Nick::refresh()
{
    int flags = 0;
    NickInfoPtr nickInfo(getChannelNick()->getNickInfo());
    bool away = false;
    int textChangedFlags = 0;

    { // NOTE: this scoping is for NoSorting below. Do not remove it
        // Disable auto-sorting while updating data
        NickListView::NoSorting noSorting(qobject_cast<NickListView*>(treeWidget()));
        if ( nickInfo )
            away = nickInfo->isAway();

        if (away)
        {
            flags = 1;
        }

        Images* images = Application::instance()->images();
        QIcon icon;

        if ( getChannelNick()->isOwner() )
        {
            flags += 64;
            icon = images->getNickIcon( Images::Owner, away );
        }
        else if ( getChannelNick()->isAdmin() )
        {
            flags += 128;
            icon = images->getNickIcon( Images::Admin, away );
        }
        else if ( getChannelNick()->isOp() )
        {
            flags += 32;
            icon = images->getNickIcon( Images::Op, away );
        }
        else if ( getChannelNick()->isHalfOp() )
        {
            flags += 16;
            icon = images->getNickIcon( Images::HalfOp, away );
        }
        else if ( getChannelNick()->hasVoice() )
        {
            flags += 8;
            icon = images->getNickIcon( Images::Voice, away );
        }
        else
        {
            flags += 4;
            icon = images->getNickIcon( Images::Normal, away );
        }

        setIcon( NicknameColumn, icon );

        QString newtext = calculateLabel1();
        if(newtext != text(NicknameColumn))
        {
            setText(NicknameColumn, newtext);
            textChangedFlags |= 1 << NicknameColumn;
        }

        newtext = calculateLabel2();
        if(newtext != text(HostmaskColumn))
        {
            setText(HostmaskColumn, newtext);
            textChangedFlags |= 1 << HostmaskColumn;
        }
    }

    if(m_flags != flags || textChangedFlags)
    {
        m_flags = flags;
        // Announce about nick update (and reposition the nick in the nick list as needed).
        emitDataChanged();
        m_channel->nicknameListViewTextChanged(textChangedFlags);

        treeWidget()->repaint();
    }
}

// Triggers reposition of this nick (QTreeWidgetItem) in the nick list
void Nick::repositionMe()
{
    if (treeWidget()->isSortingEnabled())
        emitDataChanged();
}

QString Nick::calculateLabel1() const
{
    NickInfoPtr nickinfo = getChannelNick()->getNickInfo();

    QString retString = nickinfo->getNickname();

    if(Preferences::self()->showRealNames() && !nickinfo->getRealName().isEmpty())
    {
        retString += QLatin1String(" (") + Konversation::removeIrcMarkup(nickinfo->getRealName()) + QLatin1Char(')');
    }

    return retString;
}

QString Nick::calculateLabel2() const
{
    return getChannelNick()->getNickInfo()->getHostmask();
}

bool Nick::operator<(const QTreeWidgetItem& other) const
{
    const auto& otherNick = static_cast<const Nick&>(other);

    if(Preferences::self()->sortByActivity())
    {
        uint thisRecentActivity = getChannelNick()->recentActivity();
        uint otherRecentActivity = otherNick.getChannelNick()->recentActivity();
        if(thisRecentActivity > otherRecentActivity)
        {
            return true;
        }
        if(thisRecentActivity < otherRecentActivity)
        {
            return false;
        }
        uint thisTimestamp = getChannelNick()->timeStamp();
        uint otherTimestamp = otherNick.getChannelNick()->timeStamp();
        if(thisTimestamp > otherTimestamp)
        {
            return true;
        }
        if(thisTimestamp < otherTimestamp)
        {
            return false;
        }
    }

    if(Preferences::self()->sortByStatus())
    {
        int thisFlags = getSortingValue();
        int otherFlags = otherNick.getSortingValue();

        if(thisFlags > otherFlags)
        {
            return false;
        }
        if(thisFlags < otherFlags)
        {
            return true;
        }
    }

    QString thisKey;
    QString otherKey;
    int col = treeWidget()->sortColumn();

    if(col == NicknameColumn)
    {
        if(Preferences::self()->sortCaseInsensitive())
        {
            thisKey = getChannelNick()->loweredNickname();
            otherKey = otherNick.getChannelNick()->loweredNickname();
        }
        else
        {
            thisKey = text(col);
            otherKey = otherNick.text(col);
        }
    }
    else if (col > 0) //the reason we need this: enabling hostnames adds another column
    {
        if(Preferences::self()->sortCaseInsensitive())
        {
            thisKey = text(col).toLower();
            otherKey = otherNick.text(col).toLower();
        }
        else
        {
            thisKey = text(col);
            otherKey = otherNick.text(col);
        }
    }

    return thisKey < otherKey;
}

QVariant Nick::data(int column, int role) const
{
    if (role == Qt::ForegroundRole) {
        NickInfoPtr nickInfo = getChannelNick()->getNickInfo();
        const bool isAway = (nickInfo && nickInfo->isAway());
        return treeWidget()->palette().brush(isAway ? QPalette::Disabled : QPalette::Normal, QPalette::Text);
    }
    return QTreeWidgetItem::data(column, role);
}

int Nick::getSortingValue() const
{
    int flags;
    QString sortingOrder = Preferences::self()->sortOrder();

    if(getChannelNick()->isOwner())       flags=sortingOrder.indexOf(QLatin1Char('q'));
    else if(getChannelNick()->isAdmin())  flags=sortingOrder.indexOf(QLatin1Char('p'));
    else if(getChannelNick()->isOp() )    flags=sortingOrder.indexOf(QLatin1Char('o'));
    else if(getChannelNick()->isHalfOp()) flags=sortingOrder.indexOf(QLatin1Char('h'));
    else if(getChannelNick()->hasVoice()) flags=sortingOrder.indexOf(QLatin1Char('v'));
    else                                  flags=sortingOrder.indexOf(QLatin1Char('-'));

    return flags;
}
