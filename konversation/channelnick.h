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

#include <ksharedptr.h>

#include "nickinfo.h"

/** An instance of ChannelNick is made for each nick in each channel.  So for a person in multiple channels, they will have one NickInfo, and multiple ChannelNicks.  It contains a pointer to the NickInfo, and the mode of that person in the channel.*/
class ChannelNick :  public QObject, public KShared
{
  Q_OBJECT
	  
  public:
    ChannelNick(NickInfoPtr nickinfo, bool isop, bool isadmin, bool isowner, bool ishalfop, bool hasvoice);
    bool isOp() const;
    bool isAdmin() const;
    bool isOwner() const;
    bool isHalfOp() const;
    bool hasVoice() const;

    bool setVoice(bool state);
    bool setOp(bool state);
    bool setHalfOp(bool state);
    bool setAdmin(bool state);
    bool setOwner(bool state);
    bool setMode(char mode, bool plus);
    bool setMode(int mode);
    bool setMode(bool admin,bool owner,bool op,bool halfop,bool voice);

    NickInfoPtr getNickInfo() const;
    //Purely provided for convience because they are used so often.
    //Just calls nickInfo->getNickname() etc
    QString getNickname() const;
    QString getHostmask() const;
  private: 
    NickInfoPtr nickInfo;
    bool isop;
    bool isadmin;
    bool isowner;
    bool ishalfop;
    bool hasvoice;
  signals:
    void channelNickChanged();
};

/** A ChannelNickPtr is a pointer to a ChannelNick.  Since it is a KSharedPtr,
 *  the ChannelNick object is automatically destroyed when all references are destroyed.
 */
typedef KSharedPtr<ChannelNick> ChannelNickPtr;
/** A ChannelNickMap is a list of ChannelNick pointers, indexed and sorted by
 *  lowercase nickname. 
 */
typedef QMap<LocaleString,ChannelNickPtr> ChannelNickMap;

typedef QValueList<ChannelNickPtr> ChannelNickList;


/** A ChannelMembershipMap is a list of ChannelNickMap pointers, indexed and 
 *  sorted by lowercase channel name.
 */
typedef QMap<LocaleString,ChannelNickMap *> ChannelMembershipMap;


#endif /* CHANNEL_NICK_H */

