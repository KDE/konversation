#ifndef EVENT_H
#define EVENT_H

enum EVENT_TYPE {
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

class IRCEvent
{
 public:

  IRCEvent (const QString &type, const QString &criteria, const QString &app, const QString &object, const QString &signal);
  EVENT_TYPE type;
  QString appId, objectId, criteria, signal;
  // Criteria: "sourceregexp:targetregexp:dataregexp"
};


#endif
