/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nickinfo.h    -  Nick Information
  begin:     Sat Jan 17 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

#ifndef NICKINFO_H
#define NICKINFO_H

/*
  @author Gary Cramblitt
*/

#include <qstringlist.h>
#include <qdatetime.h>
#include <ksharedptr.h>

#include <kabc/addressbook.h>

class Server;

/**
  The NickInfo object is a data container for information about a single nickname.
  It is owned by the Server object and should NOT be deleted by anything other than Server.

  A NickInfo is _only_ for online (or away) nicks.  Not for offline nicks.
  Offline (but watched or in addressbook) nicks are stored in the Server object.

*/
class NickInfo : public QObject, public KShared
{
  Q_OBJECT

  public:
    NickInfo(const QString& nick, Server* server);
    ~NickInfo();
     
    // Get properties of NickInfo object.
    QString getNickname() const;
    QString getHostmask() const;
    /** Currently return whether the user has set themselves to away with /away.
     *  May be changed in the future to parse the nick string and see if it contains
     *  "|away" or "|afk"  or something.
     */
    bool isAway() const;
    QString getAwayMessage() const;
    QString getIdentdInfo() const;
    QString getVersionInfo() const;
    bool isNotified() const;
    QString getRealName() const;
    QString getNetServer() const;
    QString getNetServerInfo() const;
    QDateTime getOnlineSince() const;
    /** Whether this user is identified with nickserv.
     *  Found only by doing /whois nick
     */
    bool isIdentified() const;
    /** This returns a string of the date and time that the user has been online since.
     *  It will return null if a /whois hasn't been issued yet for this nickinfo
     *  @return a date-string in the form of "Today, 4:23pm", "Yesterday, 12:32pm" or "Mon 3 Mar 2004, 8:02am"
     */
    QString getPrettyOnlineSince() const;
    /// Return the Server object that owns this NickInfo object.
    Server* getServer() const;
     
    /// Return the kabc (kaddressbook) contact for this nick
    KABC::Addressee getAddressee() const;
    
    /** Set properties of NickInfo object. */
    void setNickname(const QString& newNickname);
    /** Set properties of NickInfo object. */
    void setHostmask(const QString& newMask);
    /** Set properties of NickInfo object. */
    void setAway(bool state);
    /** Set properties of NickInfo object. */
    void setAwayMessage(const QString& newMessage);
    /** Set properties of NickInfo object. */
    void setIdentdInfo(const QString& newIdentdInfo);
    /** Set properties of NickInfo object. */
    void setVersionInfo(const QString& newVersionInfo);
    /** Set properties of NickInfo object. */
    void setNotified(bool state);
    /** Set properties of NickInfo object. */
    void setRealName(const QString& newRealName);
    /** Set properties of NickInfo object. */
    void setNetServer(const QString& newNetServer);
    /** Set properties of NickInfo object. */
    void setNetServerInfo(const QString& newNetServerInfo);
    /** Whether this user is identified with nickserv.
     *  Found only by doing /whois nick
     */
    void setIdentified(bool identified);
    /** Updates the time online since.
     *  This will be called from the results of a /whois
     *  This function also calculates and sets prettyOnlineSince
     *  @see getPrettyOnlineSince()
     */
    void setOnlineSince(const QDateTime& datetime);
    /** Returns html describing this nickInfo - useful for tooltips when hovering over this nick.
     */
    QString tooltip() const;
    /** Returns just the <tr><td>..   data for a tooltip.
     *  Used so that channelNick->tooltip()  can call this, then append on its own information.
     */
    void tooltipTableData(QTextStream &tooltip) const;
  private:
    QString m_nickname;
    Server* m_owningServer;
    QString m_hostmask;
    bool m_away;
    QString m_awayMessage;
    QString m_identdInfo;
    QString m_versionInfo;
    bool m_notified;
    QString m_realName;
    /** The server they are connected to. */
    QString m_netServer;
    QString m_netServerInfo;
    QDateTime m_onlineSince;
    KABC::Addressee m_addressee;
    /** Whether this user is identified with nickserv.
     *  Found only by doing /whois nick
     */
    bool m_identified;
  private slots:
    void refreshAddressee();
  signals:
    void nickInfoChanged(void);
};

/** A NickInfoPtr is a pointer to a NickInfo object.  Since it is a KSharedPtr, the NickInfo
 * object is automatically destroyed when all references are destroyed.
 */
typedef KSharedPtr<NickInfo> NickInfoPtr;
/** A NickInfoMap is a list of NickInfo objects, indexed and sorted by lowercase nickname.
 */
typedef QMap<QString,NickInfoPtr> NickInfoMap;

typedef QValueList<NickInfoPtr> NickInfoList;

#endif

