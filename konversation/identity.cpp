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
*/

#include <qtextcodec.h>

#include <kdebug.h>
#include <kglobal.h>

#include "irccharsets.h"

#include "identity.h"

int Identity::s_availableId = 0;

Identity::Identity() : KShared()
{
  m_id = s_availableId;
  s_availableId++;
  setCodecName(IRCCharsets::encodingForLocale());
}

Identity::Identity(int id) : KShared()
{
  if(id < 0) {
    m_id = s_availableId;
    s_availableId++;
  } else {
    m_id = id;
  }
}

Identity::Identity(const Identity& original) : KShared()
{
  copy(original);
  m_id = original.id();
}

Identity::~Identity()
{
}

void Identity::copy(const Identity& original)
{
  setName(original.getName());
  setRealName(original.getRealName());
  setIdent(original.getIdent());
  setNicknameList(original.getNicknameList());
  setBot(original.getBot());
  setPassword(original.getPassword());
  setPartReason(original.getPartReason());
  setKickReason(original.getKickReason());
  setInsertRememberLineOnAway(original.getInsertRememberLineOnAway());
  setShowAwayMessage(original.getShowAwayMessage());
  setAwayMessage(original.getAwayMessage());
  setReturnMessage(original.getReturnMessage());
  setCodecName(original.getCodecName());
}

void Identity::setName(const QString& newName)          { name=newName; }
QString Identity::getName() const                       { return name; }

void Identity::setRealName(const QString& name)         { realName=name; }
QString Identity::getRealName() const                   { return realName; }
void Identity::setIdent(const QString& newIdent)        { ident=newIdent; }
QString Identity::getIdent() const                      { return ident; }

void Identity::setNickname(int index,const QString& newName) { nicknameList[index]=newName; }
QString Identity::getNickname(int index) const          { return nicknameList[index]; }

void Identity::setBot(const QString& newBot)            { bot=newBot; }
QString Identity::getBot() const                        { return bot; }

void Identity::setPassword(const QString& newPassword)  { password=newPassword; }
QString Identity::getPassword() const                   { return password; }

void Identity::setPartReason(const QString& reason)     { partReason=reason; }
QString Identity::getPartReason() const                 { return partReason; }
void Identity::setKickReason(const QString& reason)     { kickReason=reason; }
QString Identity::getKickReason() const                 { return kickReason; }

void Identity::setInsertRememberLineOnAway(bool state) { insertRememberLineOnAway = state; }
bool Identity::getInsertRememberLineOnAway() const { return insertRememberLineOnAway; }
void Identity::setShowAwayMessage(bool state)           { showAwayMessages=state; }
bool Identity::getShowAwayMessage() const               { return showAwayMessages; }

void Identity::setAwayMessage(const QString& message)   { awayMessage=message; }
QString Identity::getAwayMessage() const                { return awayMessage; }
void Identity::setReturnMessage(const QString& message) { returnMessage=message; }
QString Identity::getReturnMessage() const              { return returnMessage; }

void Identity::setNicknameList(const QStringList& newList)
{
  nicknameList.clear();
  nicknameList = newList;
}

QStringList Identity::getNicknameList() const           { return nicknameList; }

QTextCodec* Identity::getCodec() const                  { return m_codec; }
QString Identity::getCodecName() const                  { return m_codecName; }
void Identity::setCodecName(const QString &newCodecName)
{
  // NOTE: codecName should be based on KCharsets::availableEncodingNames() / descriptiveEncodingNames()
  // We can get a QTextCodec from QString based on them, but can't do the reverse of that.
  
  // never set an empty or borked codec!
  QString codecName=newCodecName.lower();
  if(!IRCCharsets::isValidEncoding(codecName))
    codecName=IRCCharsets::encodingForLocale();
  
  m_codecName=codecName;
  m_codec=QTextCodec::codecForName(codecName.ascii());
}

QString Identity::getAwayNick() { return awayNick; }
void Identity::setAwayNick(const QString& n) { awayNick = n; }
