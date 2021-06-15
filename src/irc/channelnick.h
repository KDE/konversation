/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002, 2003, 2004 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef CHANNEL_NICK_H
#define CHANNEL_NICK_H

#include "nickinfo.h"

#include <QExplicitlySharedDataPointer>


/**
 * An instance of ChannelNick is made for each nick in each channel.
 * So for a person in multiple channels, they will have one NickInfo, and multiple ChannelNicks.
 * It contains a pointer to the NickInfo, and the mode of that person in the channel.
 */
class ChannelNick : public QSharedData
{
    public:
        ChannelNick(const NickInfoPtr& nickInfo, const QString& channel);
        ~ChannelNick();
        bool isOp() const;
        bool isAdmin() const;
        bool isOwner() const;
        bool isHalfOp() const;

        /** Return true if the may have any privillages at all
         * @return true if isOp() || isAdmin() || isOwner() || isHalfOp()
         */
        bool isAnyTypeOfOp() const;
        bool hasVoice() const;
        uint timeStamp() const;
        uint recentActivity() const;
        void moreActive();
        void lessActive();

        bool setVoice(bool state);
        bool setOp(bool state);
        bool setHalfOp(bool state);
        bool setAdmin(bool state);
        bool setOwner(bool state);
        bool setMode(char mode, bool plus);
        bool setMode(uint mode);
        bool setMode(bool admin,bool owner,bool op,bool halfop,bool voice);
        void setTimeStamp(uint stamp);

        NickInfoPtr getNickInfo() const;
        //Purely provided for convenience because they are used so often.
        //Just calls nickInfo->getNickname() etc
        QString getNickname() const;
        QString loweredNickname() const;
        QString getHostmask() const;
        QString tooltip() const;

        void setChanged(bool changed) { m_isChanged = changed; }
        bool isChanged () const { return m_isChanged; }

    Q_SIGNALS:
        void channelNickChanged();

    private:
        void markAsChanged();

    private:
        NickInfoPtr m_nickInfo;
        bool m_isop;
        bool m_isadmin;
        bool m_isowner;
        bool m_ishalfop;
        bool m_hasvoice;
        uint m_timeStamp;
        uint m_recentActivity;
        QString m_channel;

        bool m_isChanged;

        Q_DISABLE_COPY(ChannelNick)
};

/** A ChannelNickPtr is a pointer to a ChannelNick.  Since it is a QExplicitlySharedDataPointer,
 *  the ChannelNick object is automatically destroyed when all references are destroyed.
 */
using ChannelNickPtr = QExplicitlySharedDataPointer<ChannelNick>;

/** A ChannelNickMap is a list of ChannelNick pointers, indexed and sorted by
 *  lowercase nickname.
 */
using ChannelNickMap = QMap<QString, ChannelNickPtr>;

using ChannelNickList = QList<ChannelNickPtr>;

/** A ChannelMembershipMap is a list of ChannelNickMap pointers, indexed and
 *  sorted by lowercase channel name.
 */
using ChannelMembershipMap = QMap<QString, ChannelNickMap *>;
#endif                                            /* CHANNEL_NICK_H */
