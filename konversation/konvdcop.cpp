// Konversation DCOP interface class
// by Alex Zepeda, March 7, 2003
// $Id$

#include "konversationapplication.h"
#include "konvdcop.h"
#include "identity.h"

#include <kapp.h>
#include <kdebug.h>
#include <dcopclient.h>

#include <qstring.h>


KonvDCOP::KonvDCOP() :
           QObject(0,"Konversation"), DCOPObject("Konversation")
{
  if(!kapp->dcopClient()->isRegistered())
  {
    kapp->dcopClient()->registerAs("konversation");
    kapp->dcopClient()->setDefaultObject(objId());
  }
}

void KonvDCOP::raw(const QString& server,const QString& command)
{
  kdDebug() << "KonvDCOP::raw()" << endl;
  // send raw IRC protocol data
  emit raw(server,command);
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

void KonvDCOP::registerEventHook (const QString &type, const QString &criteria, const QString &app, const QString &object, const QString &signal)
{
  // append
  registered_events.append(new IRCEvent(type, criteria, app, object, signal));
}

void KonvDCOP::unregisterEventHook (int /*id*/)
{
}

IRCEvent::IRCEvent (const QString &a_type, const QString &a_criteria, const QString &a_app, const QString &a_obj, const QString &a_signal)
{
  QString l_type = a_type.lower();

  if (l_type == "join") {
    type = ON_JOIN;
  } else if (l_type == "quit") {
    type = ON_QUIT;
  } else if (l_type == "invite") {
    type = ON_INVITE;
  } else if (l_type == "part") {
    type = ON_PART;
  } else if (l_type == "ctcp") {
    type = ON_CTCP;
  } else if (l_type == "ctcp_reply") {
    type = ON_CTCP_REPLY;
  } else if (l_type == "message") {
    type = ON_MESSAGE;
  } else if (l_type == "notice") {
    type = ON_NOTICE;
  } else if (l_type == "kick") {
    type = ON_KICK;
  } else if (l_type == "topic") {
    type = ON_TOPIC;
  } else if (l_type == "nick") {
    type = ON_NICK_CHANGE;
  } else if (l_type == "mode") {
    type = ON_MODE;
  } else if (l_type = "numeric") {
    type = ON_NUMERIC;
  } else {
    type = ON_ANY;
  }

  appId = a_app;
  objectId = a_obj;
  criteria = a_criteria;
  signal = a_signal;
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
  : QObject(0, "KonvDCOPIdentity"), DCOPObject("KonvDCOPIdentity")
{
  if (!kapp->dcopClient()->isRegistered()) {
    kapp->dcopClient()->registerAs("konversation");
    kapp->dcopClient()->setDefaultObject(objId());
  }
}

void KonvIdentDCOP::setrealName(const QString &/*identity*/, const QString& /*name*/)
{
}

QString KonvIdentDCOP::getrealName(const QString &id_name)
{
  QPtrList<Identity> ids = KonversationApplication::preferences.getIdentityList();

  Identity *identity;
  for (identity = ids.first(); identity; identity = ids.next())
  {
    if (identity->getName()==id_name)
    {
      return identity->getRealName();
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
  : QObject(0, "KonvDCOPPreferences"), DCOPObject("KonvDCOPPreferences")
{
  if (!kapp->dcopClient()->isRegistered()) {
    kapp->dcopClient()->registerAs("konversation");
    kapp->dcopClient()->setDefaultObject(objId());
  }
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

void KonvPrefsDCOP::clearServerList()
{
  KonversationApplication::preferences.clearServerList();
}

void KonvPrefsDCOP::changeServerProperty(int id,int property,const QString& value)
{
  KonversationApplication::preferences.changeServerProperty(id, property, value);
}

void KonvPrefsDCOP::updateServer(int id,const QString& newDefinition)
{
  kdDebug() << "KonvPrefsDCOP::updateServer(" << id << "," << newDefinition << ")" << endl;
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
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::getDccAutoGet()
{
  return KonversationApplication::preferences.getDccAutoGet();
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

void KonvPrefsDCOP::setDccRollback(unsigned long bytes)
{
  KonversationApplication::preferences.setDccRollback(bytes);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

unsigned long KonvPrefsDCOP::getDccRollback()
{
  return KonversationApplication::preferences.getDccRollback();
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

QStringList KonvPrefsDCOP::getNotifyList()
{
  return KonversationApplication::preferences.getNotifyList();
}

QString KonvPrefsDCOP::getNotifyString()
{
  return KonversationApplication::preferences.getNotifyString();
}

void KonvPrefsDCOP::setNotifyList(QStringList newList)
{
  KonversationApplication::preferences.setNotifyList(newList);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvPrefsDCOP::addNotify(QString newPattern)
{
  return KonversationApplication::preferences.addNotify(newPattern);
}

bool KonvPrefsDCOP::removeNotify(QString pattern)
{
  return KonversationApplication::preferences.removeNotify(pattern);
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

void KonvPrefsDCOP::setChannelMessageColor(QString color)
{
  KonversationApplication::preferences.setChannelMessageColor(color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setQueryMessageColor(QString color)
{
  KonversationApplication::preferences.setQueryMessageColor(color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setServerMessageColor(QString color)
{
  KonversationApplication::preferences.setServerMessageColor(color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setActionMessageColor(QString color)
{
  KonversationApplication::preferences.setActionMessageColor(color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setBacklogMessageColor(QString color)
{
  KonversationApplication::preferences.setBacklogMessageColor(color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setLinkMessageColor(QString color)
{
  KonversationApplication::preferences.setLinkMessageColor(color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setCommandMessageColor(QString color)
{
  KonversationApplication::preferences.setCommandMessageColor(color); 
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setTimeColor(QString color)
{
  KonversationApplication::preferences.setTimeColor(color);
  static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

void KonvPrefsDCOP::setTextViewBackground(QString color)
{
  KonversationApplication::preferences.setTextViewBackground(color);
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

#include "konvdcop.moc"
