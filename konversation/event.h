#ifndef EVENT_H
#define EVENT_H "$Id$"

enum EVENT_TYPE {
    ON_JOIN,
    ON_PART,
    ON_CTCP,
    ON_CTCP_REPLY,
    ON_PRIVMSG,
    ON_CHANMSG,
    ON_PRIVNOT,
    ON_CHANNOT,
    ON_DUMMY
};

class IRCEvent
{
 public:

  IRCEvent (const QString &type, const QString &criteria, const QString &signal);
  EVENT_TYPE type;
  QString criteria, signal;
  // Criteria: "sourceregexp:targetregexp:dataregexp"
};


#endif
