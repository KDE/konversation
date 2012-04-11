/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Nick Information
  begin:     Sat Jan 17 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

/*
  The NickInfo object is a data container for information about a single nickname.
  It is owned by the Server object and should NOT be deleted by anything other than Server.
  Store a pointer to this with NickInfoPtr
*/

#include "nickinfo.h"
#include "application.h"
#include "linkaddressbook/addressbook.h"
#include "linkaddressbook/linkaddressbookui.h"
#include "mainwindow.h"
#include "server.h"

#include <Q3MimeSourceFactory>


NickInfo::NickInfo(const QString& nick, Server* server): KShared()
{
    m_addressee = Konversation::Addressbook::self()->getKABCAddresseeFromNick(nick, server->getServerName(), server->getDisplayName());
    m_nickname = nick;
    m_loweredNickname = nick.toLower();
    m_owningServer = server;
    m_away = false;
    m_identified = false;
    m_printedOnline = true;
    m_changed = false;

    if(!m_addressee.isEmpty())
        Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid(), 4);

    // reset nick color
    m_nickColor = 0;
}

NickInfo::~NickInfo()
{
    if(!m_addressee.isEmpty())
        Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid(), 1);
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

uint NickInfo::getNickColor()
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
    return KGlobal::locale()->formatDateTime(m_onlineSince, KLocale::FancyLongDate, false);
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

    KABC::Addressee newaddressee = Konversation::Addressbook::self()->getKABCAddresseeFromNick(newNickname, m_owningServer->getServerName(), m_owningServer->getDisplayName());
                                                  //We now know who this person is
    if(m_addressee.isEmpty() && !newaddressee.isEmpty())
    {
                                                  //Associate the old nickname with new contact
        Konversation::Addressbook::self()->associateNick(newaddressee,m_nickname, m_owningServer->getServerName(), m_owningServer->getDisplayName());
        Konversation::Addressbook::self()->saveAddressee(newaddressee);
    }
    else if(!m_addressee.isEmpty() && newaddressee.isEmpty())
    {
        Konversation::Addressbook::self()->associateNick(m_addressee, newNickname, m_owningServer->getServerName(), m_owningServer->getDisplayName());
        Konversation::Addressbook::self()->saveAddressee(newaddressee);
        newaddressee = m_addressee;
    }

    m_addressee = newaddressee;
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

    if(!m_addressee.isEmpty())
        Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid());
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


KABC::Addressee NickInfo::getAddressee() const
{
    return m_addressee;
}

void NickInfo::refreshAddressee()
{
    //m_addressee might not have changed, but information inside it may have.
    KABC::Addressee addressee=Konversation::Addressbook::self()->getKABCAddresseeFromNick(m_nickname, m_owningServer->getServerName(), m_owningServer->getDisplayName());
    if(!addressee.isEmpty() && addressee.uid() != m_addressee.uid())
    {
        //This nick now belongs to a different addressee.  We need to update the status for both the old and new addressees.
        Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid());
    }
    m_addressee = addressee;

    startNickInfoChangedTimer();

    if(!m_addressee.isEmpty())
        Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid());
}

QString NickInfo::tooltip() const
{

    QString strTooltip;
    QTextStream tooltip( &strTooltip, QIODevice::WriteOnly );
    tooltip << "<qt>";

    tooltip << "<table cellspacing=\"5\" cellpadding=\"0\">";
    tooltipTableData(tooltip);
    tooltip << "</table></qt>";
    return strTooltip;
}


QString NickInfo::getBestAddresseeName()
{
    if(!m_addressee.formattedName().isEmpty())
    {
        return m_addressee.formattedName();
    }
    else if(!m_addressee.realName().isEmpty())
    {
        return m_addressee.realName();
    }
    else if(!getRealName().isEmpty())
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
    tooltip << "<tr><td colspan=\"2\" valign=\"top\">";

    KABC::Picture photo = m_addressee.photo();
    KABC::Picture logo = m_addressee.logo();
    bool isimage=false;
    if(photo.isIntern())
    {
        Q3MimeSourceFactory::defaultFactory()->setImage( "photo", photo.data() );
        tooltip << "<img src=\"photo\">";
        isimage=true;
    }
    else if(!photo.url().isEmpty())
    {
        //JOHNFLUX FIXME TODO:
        //Are there security problems with this?  loading from an external refrence?
        //Assuming not.
        tooltip << "<img src=\"" << photo.url() << "\">";
        isimage=true;
    }
    if(logo.isIntern())
    {
        Q3MimeSourceFactory::defaultFactory()->setImage( "logo", logo.data() );
        tooltip << "<img src=\"logo\">";
        isimage=true;
    }
    else if(!logo.url().isEmpty())
    {
        //JOHNFLUX FIXME TODO:
        //Are there security problems with this?  loading from an external refrence?
        //Assuming not.
        tooltip << "<img src=\"" << logo.url() << "\">";
        isimage=true;
    }
    tooltip << "<b>" << (isimage?"":"<center>");
    if(!m_addressee.formattedName().isEmpty())
    {
        tooltip << m_addressee.formattedName();
    }
    else if(!m_addressee.realName().isEmpty())
    {
        tooltip << m_addressee.realName();
    }
    else if(!getRealName().isEmpty() && getRealName().toLower() != loweredNickname())
    {
        QString escapedRealName( getRealName() );
        escapedRealName.replace('<',"&lt;").replace('>',"&gt;");
        tooltip << escapedRealName;
    }
    else
    {
        tooltip << getNickname();
    }
    if(m_identified) tooltip << i18n(" (identified)");
    tooltip << (isimage?"":"</center>") << "</b>";

    tooltip << "</td></tr>";
    if(!m_addressee.emails().isEmpty())
    {
        tooltip << "<tr><td><b>" << i18n("Email") << ":</b></td><td>";
        tooltip << m_addressee.emails().join(", ");
        tooltip << "</td></tr>";
    }

    if(!m_addressee.organization().isEmpty())
    {
        tooltip << "<tr><td><b>" << m_addressee.organizationLabel() << ":</b></td><td>" << m_addressee.organization() << "</td></tr>";
    }
    if(!m_addressee.role().isEmpty())
    {
        tooltip << "<tr><td><b>" << m_addressee.roleLabel() << ":</b></td><td>" << m_addressee.role() << "</td></tr>";
    }
    KABC::PhoneNumber::List numbers = m_addressee.phoneNumbers();
    for( KABC::PhoneNumber::List::ConstIterator it = numbers.constBegin(); it != numbers.constEnd(); ++it)
    {
        tooltip << "<tr><td><b>" << (*it).typeLabel() << ":</b></td><td>" << (*it).number() << "</td></tr>";
    }
    if(!m_addressee.birthday().toString().isEmpty() )
    {
        tooltip << "<tr><td><b>" << m_addressee.birthdayLabel() << ":</b></td><td>" << m_addressee.birthday().toString("ddd d MMMM yyyy") << "</td></tr>";
    }
    if(!getHostmask().isEmpty())
    {
        tooltip << "<tr><td><b>" << i18n("Hostmask:") << "</b></td><td>" << getHostmask() << "</td></tr>";
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

bool NickInfo::getPrintedOnline()
{
    return m_printedOnline;
}
