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

#include "channelnick.h"
#include <server.h>
/** An instance of ChannelNick is made for each nick in each channel.  So for a person in multiple channels, they will have one NickInfo, and multiple ChannelNicks.  It contains a pointer to the NickInfo, and the mode of that person in the channel.*/

ChannelNick::ChannelNick(NickInfoPtr nickinfo, bool isop, bool isadmin, bool isowner, bool ishalfop, bool hasvoice) : KShared() {
    this->nickInfo = nickinfo;
    this->isop = isop;
    this->isadmin = isadmin;
    this->isowner = isowner;
    this->ishalfop = ishalfop;
    this->hasvoice = hasvoice;
}
bool ChannelNick::isOp() const { return isop; }
bool ChannelNick::isAdmin() const {return isadmin; }
bool ChannelNick::isOwner() const {return isowner; }
bool ChannelNick::isHalfOp() const {return ishalfop; }
bool ChannelNick::hasVoice() const {return hasvoice; }
NickInfoPtr ChannelNick::getNickInfo() const { return nickInfo; }
/** @param mode 'v' to set voice, 'a' to set admin, 'h' to set halfop, 'o' to set op.
 *  @param state what to set the mode to.
 */
bool ChannelNick::setMode(char mode, bool state) {
  switch (mode) {
    case 'v':
     return setVoice(state);
    case 'a':
     return setAdmin(state);
    case 'h':
     return setHalfOp(state);
    case 'o':
     return setOp(state);
    default:
     kdDebug() << "Mode '" << mode << "' not recognised in setModeForChannelNick";
     return false;
  }
}
/** Used still for passing modes from inputfilter to Server.  Should be removed.
 */
bool ChannelNick::setMode(int mode) {
  bool voice = mode%1;
  mode >>= 1;
  bool halfop = mode %1;
  mode >>= 1;
  bool op = mode %1;
  mode >>= 1;
  bool owner = mode %1;
  mode >>= 1;
  bool admin = mode %1;
  return setMode(admin, owner, op, halfop, voice);
}

bool ChannelNick::setMode(bool admin,bool owner,bool op,bool halfop,bool voice) {
  if(isadmin==admin && isowner==owner && isop==op && ishalfop==halfop && hasvoice==voice)
	  return false;
  isadmin=admin;
  isowner=owner;
  isop=op;
  ishalfop=halfop;
  hasvoice=voice;
  nickInfo->getServer()->emitChannelNickChanged(this);
  return true;  
}


/** set the voice for the nick, and update 
 * @returns Whether it needed to be changed.  False for no change.
 */
bool ChannelNick::setVoice(bool state) {
  if(hasvoice==state) return false;
  hasvoice=state;
  nickInfo->getServer()->emitChannelNickChanged(this);
  return true;
}
bool ChannelNick::setAdmin(bool state) {
  if(isadmin==state) return false;
  isadmin=state;
  nickInfo->getServer()->emitChannelNickChanged(this);
  return true;  
}
bool ChannelNick::setHalfOp(bool state) {
  if(ishalfop==state) return false;
  ishalfop=state;
  nickInfo->getServer()->emitChannelNickChanged(this);
  return true;
}
bool ChannelNick::setOp(bool state) {
  if(isop==state) return false;
  isop=state;
  nickInfo->getServer()->emitChannelNickChanged(this);
  return true;
}

//Purely provided for convience because they are used so often.
//Just calls nickInfo->getNickname() etc
QString ChannelNick::getNickname() const { return nickInfo->getNickname(); }
QString ChannelNick::getHostmask() const { return nickInfo->getHostmask(); }


