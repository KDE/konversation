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

#include "ircdefaultcodec.h"

#include "identity.h"

Identity::Identity()
{
  nicknameList.append(QString::null);
  nicknameList.append(QString::null);
  nicknameList.append(QString::null);
  nicknameList.append(QString::null);

  setCodec(IRCDefaultCodec::getDefaultLocaleCodec());
}

Identity::~Identity()
{
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
bool Identity::getInsertRememberLineOnAway() { return insertRememberLineOnAway; }
void Identity::setShowAwayMessage(bool state)           { showAwayMessages=state; }
bool Identity::getShowAwayMessage() const               { return showAwayMessages; }

void Identity::setAwayMessage(const QString& message)   { awayMessage=message; }
QString Identity::getAwayMessage() const                { return awayMessage; }
void Identity::setReturnMessage(const QString& message) { returnMessage=message; }
QString Identity::getReturnMessage() const              { return returnMessage; }

void Identity::setNicknameList(const QStringList& newList)
{
  nicknameList.clear();
  nicknameList=newList;
  // make sure that there are always 4 nicks in the list
  while(nicknameList.count()!=4) nicknameList.append(QString::null);
}
QStringList Identity::getNicknameList() const           { return nicknameList; }

QTextCodec* Identity::getCodec() const                      { return m_codec; }
void Identity::setCodec(const QString &newCodec)
{
  // never set an empty codec!
  if(!newCodec.isEmpty()){
    m_codec = QTextCodec::codecForName(newCodec.ascii());
  }
}

QString Identity::getAwayNick() { return awayNick; }
void Identity::setAwayNick(const QString& n) { awayNick = n; }
