/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class implements a special action for changing tab
  begin:     Sun Nov 10 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#include "tabaction.h"

TabAction::TabAction(const QString& text, int index, const KShortcut& cut,
   const QObject* receiver, const char* slot, KActionCollection* parent,
   const char* name) : KAction(text, cut, parent, name)
{
  m_index = index;
  
  if(receiver && slot) {
    connect(this, SIGNAL(activated(int)), receiver, slot);
  }
}

void TabAction::slotActivated()
{
  KAction::slotActivated();
  emit activated(m_index);
}

#include "tabaction.moc"
