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

#define QT3_SUPPORT //TODO remove when porting away from K3ListView

#include "nick.h"
#include "addressbook.h"
#include "application.h"
#include "images.h"
#include "preferences.h"

#include <kabc/phonenumber.h>

#include <K3ListView>


Nick::Nick(K3ListView *listView, const ChannelNickPtr& channelnick)
    : QObject (),
    K3ListViewItem (listView, listView->lastItem(), QString(),
                   channelnick->getNickname(), channelnick->getHostmask())
{
    m_channelnickptr = channelnick;

    Q_ASSERT(channelnick);
    if(!channelnick) return;

    m_flags = 0;
    m_height = height();

    refresh();

    connect(this, SIGNAL(refreshed()), listView, SLOT(startResortTimer()));
    connect(getChannelNick().data(), SIGNAL(channelNickChanged()), SLOT(refresh()));
    connect(getChannelNick()->getNickInfo().data(), SIGNAL(nickInfoChanged()), SLOT(refresh()));
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

    if ( nickInfo )
        away = nickInfo->isAway();

    if(away)
        flags=1;

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

    setPixmap( 0, icon );

    /*KABC::Picture pic = nickInfo->getAddressee().photo();

    if(!pic.isIntern())
    {
        pic = nickInfo->getAddressee().logo();
    }

    if(pic.isIntern())
    {
        QPixmap qpixmap(pic.data().scaleHeight(m_height));
        setPixmap(1,qpixmap);
    }*/

    QString newtext1 = calculateLabel1();
    if(newtext1 != text(1))
    {
        setText(1, newtext1);
        flags += 2;
    }

    setText(2, calculateLabel2());
    repaint();

    if(m_flags != flags)
    {
        m_flags = flags;
        emit refreshed();                         // Resort nick list
    }
}

QString Nick::calculateLabel1()
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

QString Nick::calculateLabel2()
{
    return getChannelNick()->getNickInfo()->getHostmask();
}

int Nick::compare(Q3ListViewItem* item,int col,bool ascending) const
{
    Nick* otherItem = static_cast<Nick*>(item);

    if(Preferences::self()->sortByActivity())
    {
        uint thisRecentActivity = getChannelNick()->recentActivity();
        uint otherRecentActivity = otherItem->getChannelNick()->recentActivity();
        if(thisRecentActivity > otherRecentActivity)
        {
            return -1;
        }
        if(thisRecentActivity < otherRecentActivity)
        {
            return 1;
        }
        uint thisTimestamp = getChannelNick()->timeStamp();
        uint otherTimestamp = otherItem->getChannelNick()->timeStamp();
        if(thisTimestamp > otherTimestamp)
        {
            return -1;
        }
        if(thisTimestamp < otherTimestamp)
        {
            return 1;
        }
    }

    if(Preferences::self()->sortByStatus())
    {
        int thisFlags = getSortingValue();
        int otherFlags = otherItem->getSortingValue();

        if(thisFlags > otherFlags)
        {
            return 1;
        }
        if(thisFlags < otherFlags)
        {
            return -1;
        }
    }

    QString thisKey;
    QString otherKey;

    if(col > 1) //the reason we need this: enabling hostnames adds another column
    {
        if(Preferences::self()->sortCaseInsensitive())
        {
            thisKey = thisKey.toLower();
            otherKey = otherKey.toLower();
        }
        else
        {
            thisKey = key(col, ascending);
            otherKey = otherItem->key(col, ascending);
        }
    }
    else if(col == 1)
    {
        if(Preferences::self()->sortCaseInsensitive())
        {
            thisKey = getChannelNick()->loweredNickname();
            otherKey = otherItem->getChannelNick()->loweredNickname();
        }
        else
        {
            thisKey = key(col, ascending);
            otherKey = otherItem->key(col, ascending);
        }
    }

    return thisKey.compare(otherKey);
}

void Nick::paintCell(QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    QColorGroup cg2 = cg;
    NickInfoPtr nickInfo(getChannelNick()->getNickInfo());

    if(nickInfo->isAway())
    {
        cg2.setColor(QPalette::Text, qApp->palette(listView()).color(QPalette::Disabled, QPalette::Text));
    }

    K3ListViewItem::paintCell(p,cg2,column,width,align);
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

#include "nick.moc"
