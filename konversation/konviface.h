// Konversation DCOP interface class
// by Alex Zepeda, March 7, 2003

#ifndef KONV_IFACE_H
#define KONV_IFACE_H "$Id$"

#include <qobject.h>

#include <dcopobject.h>

class KonvIface : virtual public DCOPObject
{
//  K_DCOP

  public:
    KonvIface();
    ~KonvIface();
/*
  k_dcop:
    virtual void executeCommand (const QString &) = 0;
    virtual void registerEventHook (const QString &type, const QString &criteria, const QString &signal) = 0;
    virtual void unregisterEventHook (int id) = 0;
*/
};

#endif
