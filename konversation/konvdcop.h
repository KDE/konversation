// Konversation DCOP interface class
// by Alex Zepeda, March 7, 2003

#ifndef KONV_DCOP_H
#define KONV_DCOP_H "$Id$"

#include <qobject.h>
#include <dcopobject.h>
#include <qptrlist.h>

#include "konviface.h"
#include "event.h"

class KonvDCOP : public QObject, virtual public KonvIface
{
  Q_OBJECT

  public:
    KonvDCOP();
    ~KonvDCOP();
    QPtrList<IRCEvent> registered_events;

  public slots:
    void executeCommand (const QString &);
    void registerEventHook (const QString &type, const QString &criteria, const QString &signal);
    void unregisterEventHook (int id);
};

#endif
