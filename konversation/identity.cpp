/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  identity.cpp  -  This class holds the various user identities
  begin: Son Feb 9 2003
  copyright: (C) 2003 by Dario Abatianni
  email: eisfuchs@tigress.com

  $Id$
*/

#include "identity.h"

Identity::Identity()
{
  nicknameList.append("");
  nicknameList.append("");
  nicknameList.append("");
  nicknameList.append("");
}

Identity::~Identity()
{
}

void Identity::setName(const QString& newName)          { name=newName; }
const QString& Identity::getName()                      { return name; }

void Identity::setRealName(const QString& name)         { realName=name; }
const QString Identity::getRealName()                   { return realName; }
void Identity::setIdent(const QString& newIdent)        { ident=newIdent; }
const QString& Identity::getIdent()                     { return ident; }

void Identity::setNickname(int index,const QString& newName) { nicknameList[index]=newName; }
const QString& Identity::getNickname(int index)         { return nicknameList[index]; }

void Identity::setBot(const QString& newBot)            { bot=newBot; }
const QString& Identity::getBot()                       { return bot; }

void Identity::setPassword(const QString& newPassword)  { password=newPassword; }
const QString& Identity::getPassword()                  { return password; }

void Identity::setPartReason(const QString& reason)     { partReason=reason; }
const QString& Identity::getPartReason()                { return partReason; }
void Identity::setKickReason(const QString& reason)     { kickReason=reason; }
const QString& Identity::getKickReason()                { return kickReason; }

void Identity::setShowAwayMessage(bool state)           { showAwayMessages=state; }
bool Identity::getShowAwayMessage()                     { return showAwayMessages; }

void Identity::setAwayMessage(const QString& message)   { awayMessage=message; }
const QString& Identity::getAwayMessage()               { return awayMessage; }
void Identity::setReturnMessage(const QString& message) { returnMessage=message; }
const QString& Identity::getReturnMessage()             { return returnMessage; }

void Identity::setNicknameList(const QStringList& newList)
{
  nicknameList.clear();
  nicknameList=newList;
  // make sure that there are always 4 nicks in the list
  while(nicknameList.count()!=4) nicknameList.append("");
}
const QStringList& Identity::getNicknameList()          { return nicknameList; }
