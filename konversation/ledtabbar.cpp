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
*/

#include <iostream>

#include "ledtabbar.h"

LedTabBar::LedTabBar(QWidget* parent,char* name) :
           QTabBar(parent,name)
{
}

LedTabBar::~LedTabBar()
{
}

LedTab* LedTabBar::tab(QWidget* widget)
{
  QList<QTab>* list=tabList();
  /* Again cast ... Grrrr */
  LedTab* tab=(LedTab*) list->first();

  while(tab)
  {
    if(tab->getWidget()==widget) return tab;
    /* Again cast ... Grrrr */
    tab=(LedTab*) list->next();
  }

  return 0;
}
