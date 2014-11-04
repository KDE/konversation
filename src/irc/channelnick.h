/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  channelnick.h - There is an instance of this for each nick in each channel.  So for a person in multiple channels, they will have one NickInfo, and multiple ChannelNicks.
  begin:     Wed Aug 04 2004
  copyright: (C) 2002,2003,2004 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef CHANNEL_NICK_H
#define CHANNEL_NICK_H

#include "nickinfo.h"

#include <QExplicitlySharedDataPointer>


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
        bool setMode(int mode);
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

    protected:
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

    Q_SIGNALS:
        void channelNickChanged();
};

/** A ChannelNickPtr is a pointer to a ChannelNick.  Since it is a QExplicitlySharedDataPointer,
 *  the ChannelNick object is automatically destroyed when all references are destroyed.
 */
typedef QExplicitlySharedDataPointer<ChannelNick> ChannelNickPtr;

/** A ChannelNickMap is a list of ChannelNick pointers, indexed and sorted by
 *  lowercase nickname.
 */
typedef QMap<QString,ChannelNickPtr> ChannelNickMap;

typedef QList<ChannelNickPtr> ChannelNickList;

/** A ChannelMembershipMap is a list of ChannelNickMap pointers, indexed and
 *  sorted by lowercase channel name.
 */
typedef QMap<QString,ChannelNickMap *> ChannelMembershipMap;
#endif                                            /* CHANNEL_NICK_H */
