/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  tabaction.h  -  This class implements a special action for changing tab
  begin:     Sun Nov 10 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#ifndef TABACTION_H
#define TABACTION_H

#include <kaction.h>

class TabAction : public KAction
{
  Q_OBJECT
  public:
    TabAction(const QString& text, int index, const KShortcut& cut,
      const QObject *receiver, const char *slot, KActionCollection* parent = 0,
      const char* name = 0);
  
  protected slots:
    virtual void slotActivated();
  
  signals:
    void activated(int);
  
  protected:
    int m_index;
};

#endif
