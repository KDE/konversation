/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The konversation DCOP interface class
  begin:     Mar 7 2003
  copyright: (C) 2003 by Alex Zepeda
  email:     zipzippy@sonic.net
*/

#include <kapplication.h>
#include <kdebug.h>
#include <dcopclient.h>

#include <qstring.h>
#include <klocale.h>

#include "konversationapplication.h"
#include "konvdcop.h"
#include "identity.h"
#include "ircevent.h"
#include "server.h"

KonvDCOP::KonvDCOP()
      : DCOPObject("Konversation"),
        QObject(0,"Konversation")
{
  // reset hook counter
  hookId=0;

  KConfig *config = KGlobal::config();
  config->setGroup("AutoAway");  //TODO - add this to preferences somewhere

  if (config->readBoolEntry("UseAutoAway", true))
  {
    connectDCOPSignal("kdesktop", "KScreensaverIface",
                      "KDE_start_screensaver()", "setAutoAway()", false);
  }
  else
  {
    disconnectDCOPSignal("kdesktop", "KScreensaverIface",
    "KDE_start_screensaver()", "setAutoAway()");
  }
}

void KonvDCOP::raw(const QString& server,const QString& command)
{
  kdDebug() << "KonvDCOP::raw()" << endl;
  // send raw IRC protocol data
  emit dcopRaw(server,command);
}

QStringList KonvDCOP::listServers()
{
  QStringList serverlist;
  QPtrList<Server>servers = static_cast<KonversationApplication *>(kapp)->getServerList();
  Server *server;
  for ( server = servers.first(); server; server = servers.next() )
    serverlist.append(server->getServerName());
  return serverlist;
}

QStringList KonvDCOP::listConnectedServers()
{
  QStringList serverlist;
  QPtrList<Server>servers = static_cast<KonversationApplication *>(kapp)->getServerList();
  Server *server;
  for ( server = servers.first(); server; server = servers.next() )
    if(server->connected())
      serverlist.append(server->getServerName());
  return serverlist;
}

void KonvDCOP::setAway(const QString &awaymessage)
{
  if(awaymessage.isEmpty())
    emit dcopMultiServerRaw("away " + i18n("Gone away for now."));  //away messages can't be empty.
  else
    emit dcopMultiServerRaw("away " + awaymessage);
}
void KonvDCOP::setAutoAway()
{
  kdDebug() << "set auto away" << endl;
  emit dcopSetAutoAway();
}
void KonvDCOP::setBack()
{
  emit dcopMultiServerRaw("away");
}
void KonvDCOP::sayToAll(const QString &message)
{
  emit dcopMultiServerRaw("msg " + message);
}
void KonvDCOP::actionToAll(const QString &message)
{
  emit dcopMultiServerRaw("me " + message);
}

void KonvDCOP::say(const QString& server,const QString& target,const QString& command)
{
  kdDebug() << "KonvDCOP::say()" << endl;
  // Act as if the user typed it
  emit dcopSay(server,target,command);
}

void KonvDCOP::info(const QString& string)
{
  kdDebug() << "KonvDCOP::info()" << endl;
  emit dcopInfo(string);
}

void KonvDCOP::debug(const QString& string)
{
  kdDebug() << "KonvDCOP::debug()" << endl;
  emit dcopInfo(QString("Debug: %1").arg(string));
}

void KonvDCOP::error(const QString& string)
{
  kdDebug() << "KonvDCOP::error()" << endl;
  emit dcopInfo(QString("Error: %1").arg(string));
}

void KonvDCOP::insertRememberLine()
{
  emit dcopInsertRememberLine();
}

void KonvDCOP::connectToServer(const QString& url, int port, const QString& channel, const QString& password)
{
  emit dcopConnectToServer(url, port, channel, password);
}

/*
 app is the dcop app name, object is that dcop app's object name, and signal is the name of the
 function for that dcop app's object. I didn't implement any matching code yet, so I think it
 just passes all events through at this point. We could register more than one hook for an event
 type when matching code is done.
*/

int KonvDCOP::registerEventHook(const QString& type,
                                const QString& criteria,
                                const QString& app,
                                const QString& object,
                                const QString& signal)
{
  hookId++; // FIXME: remember that this could wrap around sometimes! Find a better way!

  // add new event to registered list of event hooks. the id is needed to help unregistering
  registered_events.append(new IRCEvent(type,criteria,app,object,signal,hookId));

  return hookId;
}

void KonvDCOP::unregisterEventHook(int hookId)
{
  // go through the list of registered events
  for(unsigned int index=0;index<registered_events.count();index++)
  {
    // if we found the id we were looking for ...
    if(registered_events.at(index)->hookId()==hookId)
    {
      // ... remove it and return
      registered_events.remove(index);
      kdDebug() << "KonvDCOP::unregisterEventHook(): hook id " << hookId << " removed. Remaining hooks: " << registered_events.count() << endl;
      return;
    }
  } // endfor
  kdDebug() << "KonvDCOP::unregisterEventHook(): hook id " << hookId << " not found!" << endl;
}

bool KonvDCOP::isIgnore (int serverid, const QString &hostmask, Ignore::Type type)
{
  return isIgnore(serverid, hostmask, static_cast<int>(type));
}

bool KonvDCOP::isIgnore (int /*serverid*/, const QString &/*hostmask*/, int /*type*/)
{
  return false;
}

QString KonvDCOP::getNickname (int /*serverid*/)
{
  return QString::null;
}

// Identity stuff
KonvIdentDCOP::KonvIdentDCOP()
             : DCOPObject("KonvDCOPIdentity"),
               QObject(0, "KonvDCOPIdentity")
{
}

void KonvIdentDCOP::setrealName(const QString &/*identity*/, const QString& /*name*/)
{
}

QString KonvIdentDCOP::getrealName(const QString &id_name)
{
  QValueList<IdentityPtr> ids = KonversationApplication::preferences.getIdentityList();

  for(QValueList<IdentityPtr>::iterator it = ids.begin(); it != ids.end(); ++it)
  {
    if ((*it)->getName() == id_name)
    {
      return (*it)->getRealName();
    }
  }

  return QString::null;
}

void KonvIdentDCOP::setIdent(const QString &/*identity*/, const QString& /*ident*/)
{
  //KonversationApplication::preferences.getIdentityByName(identity)->.setIdent(;
}

QString KonvIdentDCOP::getIdent(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getIdent();
}

void KonvIdentDCOP::setNickname(const QString &identity, int index,const QString& nick)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setNickname(index, nick);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getNickname(const QString &identity, int index)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getNickname(index);
}

void KonvIdentDCOP::setBot(const QString &identity, const QString& bot)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setBot(bot);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getBot(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getBot();
}

void KonvIdentDCOP::setPassword(const QString &identity, const QString& password)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setPassword(password);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getPassword(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getPassword();
}

void KonvIdentDCOP::setNicknameList(const QString &identity, const QStringList& newList)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setNicknameList(newList);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QStringList KonvIdentDCOP::getNicknameList(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getNicknameList();
}

void KonvIdentDCOP::setPartReason(const QString &identity, const QString& reason)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setPartReason(reason);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getPartReason(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getPartReason();
}

void KonvIdentDCOP::setKickReason(const QString &identity, const QString& reason)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setKickReason(reason);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getKickReason(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getKickReason();
}

void KonvIdentDCOP::setShowAwayMessage(const QString &identity, bool state)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setShowAwayMessage(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvIdentDCOP::getShowAwayMessage(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getShowAwayMessage();
}

void KonvIdentDCOP::setAwayMessage(const QString &identity, const QString& message)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setAwayMessage(message);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getAwayMessage(const QString &identity)
{
  const QString f = KonversationApplication::preferences.getIdentityByName(identity)->getAwayMessage();
  return f;
}

void KonvIdentDCOP::setReturnMessage(const QString &identity, const QString& message)
{
  const Identity *i = KonversationApplication::preferences.getIdentityByName(identity);
  const_cast<Identity *>(i)->setReturnMessage(message);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getReturnMessage(const QString &identity)
{
  return KonversationApplication::preferences.getIdentityByName(identity)->getReturnMessage();
}

// Prefs class stuff
KonvPrefsDCOP::KonvPrefsDCOP ()
             : DCOPObject("KonvDCOPPreferences"),
               QObject(0, "KonvDCOPPreferences")
{
}

bool KonvPrefsDCOP::getAutoReconnect()
{
  return KonversationApplication::preferences.getAutoReconnect();
}

void KonvPrefsDCOP::setAutoReconnect(bool state)
{
  KonversationApplication::preferences.setAutoReconnect(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getAutoRejoin()
{
  return KonversationApplication::preferences.getAutoRejoin();
}

void KonvPrefsDCOP::setAutoRejoin(bool state)
{
  KonversationApplication::preferences.setAutoRejoin(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getBeep()
{
  return KonversationApplication::preferences.getBeep();
}

void KonvPrefsDCOP::setBeep(bool state)
{
  KonversationApplication::preferences.setBeep(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setLog(bool state)
{
  KonversationApplication::preferences.setLog(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getLog()
{
  return KonversationApplication::preferences.getLog();
}

void KonvPrefsDCOP::setLowerLog(bool state)
{
  KonversationApplication::preferences.setLowerLog(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getLowerLog()
{
  return KonversationApplication::preferences.getLowerLog();
}

void KonvPrefsDCOP::setLogFollowsNick(bool state)
{
  KonversationApplication::preferences.setLogFollowsNick(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getLogFollowsNick()
{
  return KonversationApplication::preferences.getLogFollowsNick();
}

void KonvPrefsDCOP::setLogPath(QString path)
{
  KonversationApplication::preferences.setLogPath(path);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvPrefsDCOP::getLogPath()
{
  return KonversationApplication::preferences.getLogPath();
}

void KonvPrefsDCOP::setDccAddPartner(bool state)
{
  KonversationApplication::preferences.setDccAddPartner(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccAddPartner()
{
  return KonversationApplication::preferences.getDccAddPartner();
}

void KonvPrefsDCOP::setDccCreateFolder(bool state)
{
  KonversationApplication::preferences.setDccCreateFolder(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccCreateFolder()
{
  return KonversationApplication::preferences.getDccCreateFolder();
}

void KonvPrefsDCOP::setDccAutoGet(bool state)
{
  KonversationApplication::preferences.setDccAutoGet(state);
  if(KonversationApplication::preferences.getDccAutoResume() == true && state == false)
  {
	  KonversationApplication::preferences.setDccAutoResume(false);
  }
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccAutoGet()
{
  return KonversationApplication::preferences.getDccAutoGet();
}

void KonvPrefsDCOP::setDccAutoResume(bool state)
{
  KonversationApplication::preferences.setDccAutoResume(state);
  if(KonversationApplication::preferences.getDccAutoGet() == false && state == true)
  {
	  KonversationApplication::preferences.setDccAutoGet(true);
  }
  static_cast<KonversationApplication*>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccAutoResume()
{
  return KonversationApplication::preferences.getDccAutoResume();
}

void KonvPrefsDCOP::setDccBufferSize(unsigned long size)
{
  KonversationApplication::preferences.setDccBufferSize(size);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

unsigned long KonvPrefsDCOP::getDccBufferSize()
{
  return KonversationApplication::preferences.getDccBufferSize();
}

void KonvPrefsDCOP::setDccPath(QString path)
{
  KonversationApplication::preferences.setDccPath(path);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvPrefsDCOP::getDccPath()
{
  return KonversationApplication::preferences.getDccPath();
}

void KonvPrefsDCOP::setDccMethodToGetOwnIp(int methodId)
{
  KonversationApplication::preferences.setDccMethodToGetOwnIp(methodId);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

int KonvPrefsDCOP::getDccMethodToGetOwnIp()
{
  return KonversationApplication::preferences.getDccMethodToGetOwnIp();
}

void KonvPrefsDCOP::setDccSpecificOwnIp(const QString& ip)
{
  KonversationApplication::preferences.setDccSpecificOwnIp(ip);
}

QString KonvPrefsDCOP::getDccSpecificOwnIp()
{
  return KonversationApplication::preferences.getDccSpecificOwnIp();
}

void KonvPrefsDCOP::setDccSpecificSendPorts(bool state)
{
  KonversationApplication::preferences.setDccSpecificSendPorts(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccSpecificSendPorts()
{
  return KonversationApplication::preferences.getDccSpecificSendPorts();
}

void KonvPrefsDCOP::setDccSendPortsFirst(unsigned long port)
{
  KonversationApplication::preferences.setDccSendPortsFirst(port);
  if(KonversationApplication::preferences.getDccSendPortsLast() < port)
    KonversationApplication::preferences.setDccSendPortsLast(port);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

unsigned long KonvPrefsDCOP::getDccSendPortsFirst()
{
  return KonversationApplication::preferences.getDccSendPortsFirst();
}

void KonvPrefsDCOP::setDccSendPortsLast(unsigned long port)
{
  KonversationApplication::preferences.setDccSendPortsLast(port);
  if(port < KonversationApplication::preferences.getDccSendPortsFirst())
    KonversationApplication::preferences.setDccSendPortsFirst(port);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

unsigned long KonvPrefsDCOP::getDccSendPortsLast()
{
  return KonversationApplication::preferences.getDccSendPortsLast();
}

void KonvPrefsDCOP::setDccSpecificChatPorts(bool state)
{
  KonversationApplication::preferences.setDccSpecificChatPorts(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccSpecificChatPorts()
{
  return KonversationApplication::preferences.getDccSpecificChatPorts();
}

void KonvPrefsDCOP::setDccChatPortsFirst(unsigned long port)
{
  KonversationApplication::preferences.setDccChatPortsFirst(port);
  if(KonversationApplication::preferences.getDccChatPortsLast() < port)
    KonversationApplication::preferences.setDccChatPortsLast(port);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

unsigned long KonvPrefsDCOP::getDccChatPortsFirst()
{
  return KonversationApplication::preferences.getDccChatPortsFirst();
}

void KonvPrefsDCOP::setDccChatPortsLast(unsigned long port)
{
  KonversationApplication::preferences.setDccChatPortsLast(port);
  if(port < KonversationApplication::preferences.getDccChatPortsFirst())
    KonversationApplication::preferences.setDccChatPortsFirst(port);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

unsigned long KonvPrefsDCOP::getDccChatPortsLast()
{
  return KonversationApplication::preferences.getDccChatPortsLast();
}

void KonvPrefsDCOP::setDccFastSend(bool state)
{
  KonversationApplication::preferences.setDccFastSend(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccFastSend()
{
  return KonversationApplication::preferences.getDccFastSend();
}

void KonvPrefsDCOP::setDccSendTimeout(int sec)
{
  KonversationApplication::preferences.setDccSendTimeout(sec);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

int KonvPrefsDCOP::getDccSendTimeout()
{
  return KonversationApplication::preferences.getDccSendTimeout();
}

void KonvPrefsDCOP::setBlinkingTabs(bool blink)
{
  KonversationApplication::preferences.setBlinkingTabs(blink);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getBlinkingTabs()
{
  return KonversationApplication::preferences.getBlinkingTabs();
}

void KonvPrefsDCOP::setBringToFront(bool state)
{
  KonversationApplication::preferences.setBringToFront(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getBringToFront()
{
  return KonversationApplication::preferences.getBringToFront();
}

void KonvPrefsDCOP::setCloseButtonsOnTabs(bool state)
{
  KonversationApplication::preferences.setCloseButtonsOnTabs(state);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getCloseButtonsOnTabs()  
{
  return KonversationApplication::preferences.getCloseButtonsOnTabs();
}

int KonvPrefsDCOP::getNotifyDelay()
{
  return KonversationApplication::preferences.getNotifyDelay();
}

void KonvPrefsDCOP::setNotifyDelay(int delay)
{
  KonversationApplication::preferences.setNotifyDelay(delay);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getUseNotify()
{
  return KonversationApplication::preferences.getUseNotify();
}

void KonvPrefsDCOP::setUseNotify(bool use)
{
  KonversationApplication::preferences.setUseNotify(use);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QMap<QString, QStringList> KonvPrefsDCOP::getNotifyList()
{
  return KonversationApplication::preferences.getNotifyList();
}

QStringList KonvPrefsDCOP::getNotifyListByGroup(QString groupName)
{
  return KonversationApplication::preferences.getNotifyListByGroup(groupName);
}

QString KonvPrefsDCOP::getNotifyStringByGroup(QString groupName)
{
  return KonversationApplication::preferences.getNotifyStringByGroup(groupName);
}

void KonvPrefsDCOP::setNotifyList(QMap<QString, QStringList> newList)
{
  KonversationApplication::preferences.setNotifyList(newList);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::addNotify(QString groupName, QString newPattern)
{
  return KonversationApplication::preferences.addNotify(groupName, newPattern);
}

bool KonvPrefsDCOP::removeNotify(QString groupName, QString pattern)
{
  return KonversationApplication::preferences.removeNotify(groupName, pattern);
}

void KonvPrefsDCOP::addIgnore(QString newIgnore)
{
  KonversationApplication::preferences.addIgnore(newIgnore);
}

void KonvPrefsDCOP::clearIgnoreList()
{
  KonversationApplication::preferences.clearIgnoreList();
}

//QPtrList<Ignore> getIgnoreList()

void KonvPrefsDCOP::setIgnoreList(QPtrList<Ignore> newList)
{
  KonversationApplication::preferences.setIgnoreList(newList);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setColor(QString colorName,QString color)
{
  KonversationApplication::preferences.setColor(colorName,color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setNickCompleteSuffixStart(QString suffix)
{
  KonversationApplication::preferences.setNickCompleteSuffixStart(suffix);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setNickCompleteSuffixMiddle(QString suffix)
{
  KonversationApplication::preferences.setNickCompleteSuffixMiddle(suffix);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvPrefsDCOP::getNickCompleteSuffixStart()
{
  return KonversationApplication::preferences.getNickCompleteSuffixStart();
}

QString KonvPrefsDCOP::getNickCompleteSuffixMiddle()
{
  return KonversationApplication::preferences.getNickCompleteSuffixMiddle();
}

void KonvPrefsDCOP::setOSDUsage(bool state)
{
	KonversationApplication::preferences.setOSDUsage(state);
	static_cast<KonversationApplication *>(kapp)->saveOptions(true);
	static_cast<KonversationApplication *>(kapp)->osd->setEnabled(state);
}

bool KonvPrefsDCOP::getOSDUsage()
{
	return KonversationApplication::preferences.getOSDUsage();
}

void KonvPrefsDCOP::setOSDOffsetX(int offset)
{
  KonversationApplication::preferences.setOSDOffsetX(offset);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

int KonvPrefsDCOP::getOSDOffsetX()
{
  return KonversationApplication::preferences.getOSDOffsetX();
}

void KonvPrefsDCOP::setOSDOffsetY(int offset)
{
  KonversationApplication::preferences.setOSDOffsetY(offset);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

int KonvPrefsDCOP::getOSDOffsetY()
{
  return KonversationApplication::preferences.getOSDOffsetY();
}

void KonvPrefsDCOP::setOSDAlignment(int alignment)
{
  KonversationApplication::preferences.setOSDAlignment(alignment);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

int KonvPrefsDCOP::getOSDAlignment()
{
  return KonversationApplication::preferences.getOSDAlignment();
}

#include "konvdcop.moc"
