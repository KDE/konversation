/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtabbar.cpp  -  description
  begin:     Sun Feb 24 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qpainter.h>

#include <kdebug.h>

#include "ledtabbar.h"

LedTabBar::LedTabBar(QWidget* parent,const char* name) :
           QTabBar(parent,name)
{
}

LedTabBar::~LedTabBar()
{
}

LedTab* LedTabBar::tab(QWidget* widget)
{
  QPtrList<QTab>* list=tabList();

  // These casts can't be helped, templates don't like casting
  LedTab* tab=(LedTab*) list->first();
  while(tab)
  {
    if(tab->getWidget()==widget) return tab;
    tab=(LedTab*) list->next();
  }

  return 0;
}

// reimplemented to avoid casts in active code
LedTab* LedTabBar::tab(int id)
{
  return (LedTab*) QTabBar::tab(id);
}

// repaint only the needed tab region to avoid flickering
void LedTabBar::repaintLED(LedTab* tab)
{
  repaint(tab->rect(),false);
}

#include "ledtabbar.moc"
