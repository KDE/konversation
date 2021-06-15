/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Gary Cramblitt <garycramblitt@comcast.net>
*/

#ifndef NICKINFO_H
#define NICKINFO_H

#include <QDateTime>
#include <QTextStream>
#include <QList>

#include <QExplicitlySharedDataPointer>

class Server;

/**
  The NickInfo object is a data container for information about a single nickname.
  It is owned by the Server object and should NOT be deleted by anything other than Server.

  A NickInfo is _only_ for online (or away) nicks.  Not for offline nicks.
  Offline (but watched) nicks are stored in the Server object.

*/
class NickInfo : public QSharedData
{
    public:
        NickInfo(const QString& nick, Server* server);
        ~NickInfo();

        // Get properties of NickInfo object.
        QString getNickname() const;
        QString loweredNickname() const;
        QString getHostmask() const;
        /** Currently return whether the user has set themselves to away with /away.
         *  May be changed in the future to parse the nick string and see if it contains
         *  "|away" or "|afk"  or something.
         */
        bool isAway() const;
        QString getAwayMessage() const;
        QString getRealName() const;
        QString getNetServer() const;
        QString getNetServerInfo() const;
        QDateTime getOnlineSince() const;
        uint getNickColor() const;
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

        /** Set properties of NickInfo object. */
        void setNickname(const QString& newNickname);
        /** Set properties of NickInfo object. Ignores the request is newmask is empty.*/
        void setHostmask(const QString& newMask);
        /** Set properties of NickInfo object. */
        void setAway(bool state);
        /** Set properties of NickInfo object. */
        void setAwayMessage(const QString& newMessage);
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

        /** Returns a full name for this contact. Uses the real name from whois.
         *  If that fails, use nickname.
         *
         *  @return A string to show the user for the name of this contact
         */
        QString getBestAddresseeName() const;

        void setPrintedOnline(bool printed);
        bool getPrintedOnline() const;

        QString account() const { return m_account; }
        void setAccount(const QString &name);

        bool isChanged() const { return m_changed; }
        void setChanged(bool c) { m_changed = c; }

    private:
        /** After calling, emitNickInfoChanged is guaranteed to be called _within_ 1 second.
         *  Used to consolidate changed signals.
         */
        void startNickInfoChangedTimer();
        QString m_nickname;
        QString m_loweredNickname;
        Server* m_owningServer;
        QString m_hostmask;
        bool m_away;
        QString m_awayMessage;
        QString m_realName;
        /** The server they are connected to. */
        QString m_netServer;
        QString m_netServerInfo;
        QDateTime m_onlineSince;
        /** Whether this user is identified with nickserv.
         *  Found only by doing /whois nick
         */
        bool m_identified;
        /* True if "foo is online" message is printed */
        bool m_printedOnline;
        /* The color index for lookup on Preferences::NickColor(index).name()
           Internally stored as index-1 to allow checking for 0 */
        mutable uint m_nickColor;

        QString m_account;

        bool m_changed;

        Q_DISABLE_COPY(NickInfo)
};

/** A NickInfoPtr is a pointer to a NickInfo object.  Since it is a QExplicitlySharedDataPointer, the NickInfo
 * object is automatically destroyed when all references are destroyed.
 */
using NickInfoPtr = QExplicitlySharedDataPointer<NickInfo>;
/** A NickInfoMap is a list of NickInfo objects, indexed and sorted by lowercase nickname.
 */
using NickInfoMap = QMap<QString, NickInfoPtr>;

using NickInfoList = QList<NickInfoPtr>;
#endif
