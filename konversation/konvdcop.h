// Konversation DCOP interface class
// by Alex Zepeda, March 7, 2003

#ifndef KONV_DCOP_H
#define KONV_DCOP_H "$Id$"

#include <qobject.h>
#include <dcopobject.h>
#include <qptrlist.h>

#include "konviface.h"
#include "event.h"

class KonvDCOP : public QObject,virtual public KonvIface
{
  Q_OBJECT

  public:
    KonvDCOP();
    ~KonvDCOP();
    QPtrList<IRCEvent> registered_events;

  signals:
    void dcopSay(const QString& server,const QString& target,const QString& command);
    void dcopError(const QString& string);

  public slots:
    void raw(const QString& server,const QString& command);
    void say(const QString& server,const QString& target,const QString& command);
    void registerEventHook(const QString& type,const QString& criteria,const QString& signal);
    void unregisterEventHook(int id);
    void error(const QString& string);
};

#endif
