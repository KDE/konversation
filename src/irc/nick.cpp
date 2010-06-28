/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Fri Jan 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "nick.h"
#include "addressbook.h"
#include "application.h"
#include "images.h"
#include "preferences.h"
#include "nicklistview.h"

#include <QHeaderView>

#include <kabc/phonenumber.h>

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
            // Brush of the first column will be used for all columns
            setForeground(NicknameColumn,
                qApp->palette(treeWidget()).brush(QPalette::Disabled, QPalette::Text));

            flags = 1;
        }
        else
        {
            // Brush of the first column will be used for all columns
            setForeground(NicknameColumn,
                treeWidget()->palette().brush(QPalette::Normal, QPalette::Text));
        }

        Images* images = Application::instance()->images();
        QPixmap icon;

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
    KABC::Addressee addressee = nickinfo->getAddressee();

    if(!addressee.realName().isEmpty())           //if no addressee, realName will be empty
    {
        return nickinfo->getNickname() + " (" + addressee.realName() + ')';
    }
    else if(Preferences::self()->showRealNames() && !nickinfo->getRealName().isEmpty())
    {
        return nickinfo->getNickname() + " (" + nickinfo->getRealName() + ')';
    }

    return nickinfo->getNickname();
}

QString Nick::calculateLabel2() const
{
    return getChannelNick()->getNickInfo()->getHostmask();
}

bool Nick::operator<(const QTreeWidgetItem& other) const
{
    const Nick& otherNick = static_cast<const Nick&>(other);

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
    if (role == Qt::ForegroundRole && column > 0) {
        // Use brush of the first column for all columns
        return data(NicknameColumn, role);
    }
    return QTreeWidgetItem::data(column, role);
}

int Nick::getSortingValue() const
{
    int flags;
    QString sortingOrder = Preferences::self()->sortOrder();

    if(getChannelNick()->isOwner())       flags=sortingOrder.indexOf('q');
    else if(getChannelNick()->isAdmin())  flags=sortingOrder.indexOf('p');
    else if(getChannelNick()->isOp() )    flags=sortingOrder.indexOf('o');
    else if(getChannelNick()->isHalfOp()) flags=sortingOrder.indexOf('h');
    else if(getChannelNick()->hasVoice()) flags=sortingOrder.indexOf('v');
    else                                  flags=sortingOrder.indexOf('-');

    return flags;
}
