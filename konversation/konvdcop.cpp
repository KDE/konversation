// Konversation DCOP interface class
// by Alex Zepeda, March 7, 2003

#include "konvdcop.h"

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

KonvDCOP::~KonvDCOP()
{
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

void KonvDCOP::error(const QString& string)
{
  kdDebug() << "KonvDCOP::error()" << endl;
  emit dcopError(string);
}

void KonvDCOP::registerEventHook(const QString& type,const QString& criteria,const QString& signal)
{
  // append
  registered_events.append(new IRCEvent(type, criteria, signal));
}

void KonvDCOP::unregisterEventHook(int id)
{
  kdDebug() << "KonvDCOP::unregisterEventHook(" << id << ")" << endl;
}

IRCEvent::IRCEvent(const QString& a_type,const QString& a_criteria,const QString& a_signal)
{
  QString l_type = a_type.lower();

  if (l_type == "join") {
    type = ON_JOIN;
  } else if (l_type == "part") {
    type = ON_PART;
  } else if (l_type == "ctcp") {
    type = ON_CTCP;
  } else if (l_type == "ctcp_reply") {
    type = ON_CTCP_REPLY;
  } else if (l_type == "private_message") {
    type = ON_PRIVMSG;
  } else if (l_type == "channel_message") {
    type = ON_CHANMSG;
  } else if (l_type == "private_notice") {
    type = ON_PRIVNOT;
  } else if (l_type == "channel_notice") {
    type = ON_CHANNOT;
  } else {
    type = ON_DUMMY;
  }

  criteria = a_criteria;
  signal = a_signal;
}

#include "konvdcop.moc"
