/***************************************************************************
    begin                : Thu Jul 25 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kdebug.h>

#include "nicklistviewitem.h"
#include "konversationapplication.h"
#include "nick.h"
#include "nickinfo.h"
#include "nicklistview.h"
#include "images.h"

NickListViewItem::NickListViewItem(KListView* parent,
QListViewItem *after,
const QString& passed_label,
const QString& passed_label2,
Nick *n) :
KListViewItem(parent,after,QString(),passed_label,passed_label2)
{
    Q_ASSERT(n);
    nick = n;
    m_flags = 0;

    m_height = height();
    connect(nick->getChannelNick(), SIGNAL(channelNickChanged()), SLOT(refresh()));
    connect(nick->getNickInfo(), SIGNAL(nickInfoChanged()), SLOT(refresh()));

    refresh();
}

NickListViewItem::~NickListViewItem()
{
}

void NickListViewItem::refresh()
{
    int flags = 0;
    NickInfo* nickInfo = nick->getNickInfo();
    bool away = false;

    if ( nickInfo )
        away = nickInfo->isAway();

    if(away)
        flags=1;

    Images* images = KonversationApplication::instance()->images();
    QPixmap icon;

    if ( nick->isOwner() )
    {
        flags += 64;
        icon = images->getNickIcon( Images::Owner, away );
    }
    else if ( nick->isAdmin() )
    {
        flags += 128;
        icon = images->getNickIcon( Images::Admin, away );
    }
    else if ( nick->isOp() )
    {
        flags += 32;
        icon = images->getNickIcon( Images::Op, away );
    }
    else if ( nick->isHalfop() )
    {
        flags += 16;
        icon = images->getNickIcon( Images::HalfOp, away );
    }
    else if ( nick->hasVoice() )
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

    KABC::Picture pic = nickInfo->getAddressee().photo();

    if(!pic.isIntern())
    {
        pic = nickInfo->getAddressee().logo();
    }

    if(pic.isIntern())
    {
        QPixmap qpixmap(pic.data().scaleHeight(m_height));
        setPixmap(1,qpixmap);
    }

    QString newtext1 = calculateLabel1();
    if(newtext1 != text(1))
    {
        setText(1,calculateLabel1());
        flags += 2;
    }

    setText(2,calculateLabel2());
    repaint();

    if(m_flags != flags)
    {
        m_flags = flags;
        emit refreshed();                         // Resort nick list
    }
}

QString NickListViewItem::calculateLabel1()
{
    NickInfoPtr nickinfo = nick->getNickInfo();
    KABC::Addressee addressee = nickinfo->getAddressee();

    if(!addressee.realName().isEmpty())           //if no addressee, realName will be empty
    {
        return nick->getNickInfo()->getNickname() + " (" + addressee.realName() + ')';
    }
    else if(Preferences::showRealNames() && !nick->getNickInfo()->getRealName().isEmpty())
    {
        return nick->getNickInfo()->getNickname() + " (" + nick->getNickInfo()->getRealName() + ')';
    }

    return nick->getNickInfo()->getNickname();
}

QString NickListViewItem::calculateLabel2()
{
    return nick->getNickInfo()->getHostmask();
}

int NickListViewItem::compare(QListViewItem* item,int col,bool ascending) const
{
    NickListViewItem* otherItem = static_cast<NickListViewItem*>(item);

    if(Preferences::sortByStatus())
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

    if(col > 1)
    {
        if(Preferences::sortCaseInsensitive())
        {
            thisKey = thisKey.lower();
            otherKey = otherKey.lower();
        }
        else
        {
            thisKey = key(col, ascending);
            otherKey = otherItem->key(col, ascending);
        }
    }
    else if(col == 1)
    {
        if(Preferences::sortCaseInsensitive())
        {
            thisKey = nick->loweredNickname();
            otherKey = otherItem->getNick()->loweredNickname();
        }
        else
        {
            thisKey = key(col, ascending);
            otherKey = otherItem->key(col, ascending);
        }
    }

    return thisKey.compare(otherKey);
}

void NickListViewItem::paintCell(QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    QColorGroup cg2 = cg;
    NickInfo* nickInfo = nick->getNickInfo();

    if(nickInfo->isAway())
    {
        cg2.setColor(QColorGroup::Text, kapp->palette().disabled().text());
    }

    KListViewItem::paintCell(p,cg2,column,width,align);
}

int NickListViewItem::getSortingValue() const
{
    int flags;
    QString sortingOrder=Preferences::sortOrder();

    if(nick->isOwner())       flags=sortingOrder.find('q');
    else if(nick->isAdmin())  flags=sortingOrder.find('p');
    else if(nick->isOp() )    flags=sortingOrder.find('o');
    else if(nick->isHalfop()) flags=sortingOrder.find('h');
    else if(nick->hasVoice()) flags=sortingOrder.find('v');
    else                      flags=sortingOrder.find('-');

    return flags;
}

Nick *NickListViewItem::getNick()
{
    return nick;
}

#include "nicklistviewitem.moc"
