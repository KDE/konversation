/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircevent.h  -  DCOP event dispatcher
  begin:     Tue Oct 14 2003
  copyright: (C) 2003 by Dario Abatianni, based on work by Alex Zepeda, March 7, 2003
  email:     eisfuchs@tigress.com
*/

#ifndef IRCEVENT_H
#define IRCEVENT_H

/*
  @author Dario Abatianni, Alex Zepeda
*/

enum EVENT_TYPE
{
  ON_JOIN,
  ON_QUIT,
  ON_INVITE,
  ON_PART,
  ON_CTCP,
  ON_CTCP_REPLY,
  ON_MESSAGE,
  ON_NOTICE,
  ON_KICK,
  ON_TOPIC,
  ON_NICK_CHANGE,
  ON_MODE,
  ON_NUMERIC,
  ON_ANY
};

#include <qstring.h>

class IRCEvent
{
  public:
    IRCEvent (const QString &type, const QString &criteria, const QString &app, const QString &object, const QString &signal, int id);
    ~IRCEvent();

    EVENT_TYPE type;
    QString appId, objectId, criteria, signal;
    // Criteria: "sourceregexp:targetregexp:dataregexp"
    int hookId();

  protected:
    int id;
};

#endif
