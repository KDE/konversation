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

#include "ircevent.h"

IRCEvent::IRCEvent (const QString &a_type,
                    const QString &a_criteria,
                    const QString &a_app,
                    const QString &a_obj,
                    const QString &a_signal,
                    int a_id)
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
  } else if (l_type == "numeric") {
    type = ON_NUMERIC;
  } else {
    type = ON_ANY;
  }

  appId = a_app;
  objectId = a_obj;
  criteria = a_criteria;
  signal = a_signal;
  id = a_id;
/*
  kdDebug() << "IRCEvent(): type=" << type  << endl
            << "            criteria=" << criteria << endl
            << "            app=" << a_app << endl
            << "            object=" << a_obj << endl
            << "            signal=" << a_signal << endl
            << "            id=" << id << endl;
*/
}

IRCEvent::~IRCEvent()
{
}

int IRCEvent::hookId()
{
  return id;
}
