/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONNOTIFICATIONHANDLER_H
#define KONVERSATIONNOTIFICATIONHANDLER_H

#include <qobject.h>

class ChatWindow;
class KonversationApplication;
class KonversationMainWindow;

namespace Konversation {

class NotificationHandler : public QObject
{
  Q_OBJECT
  public:
    NotificationHandler(KonversationApplication* parent = 0, const char* name = 0);
    ~NotificationHandler();
  
  public slots:
    void message(ChatWindow* chatWin, const QString& fromNick, const QString& message);
    void nick(ChatWindow* chatWin, const QString& fromNick, const QString& message);
    void join(ChatWindow* chatWin, const QString& nick);
    void part(ChatWindow* chatWin, const QString& nick);
  
  protected:
    void startTrayNotification(ChatWindow* chatWin);
  
  private:
    KonversationMainWindow* m_mainWindow;
};

};

#endif
