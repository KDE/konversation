/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtabwidget.cpp  -  A tab widget with support for status "LED"s
  begin:     Fri Feb 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include "ledtabwidget.h"
#include "ledtab.h"
#include "ledtabbar.h"

LedTabWidget::LedTabWidget(QWidget* parent,const char* name) :
              QTabWidget(parent,name)
{
  setTabBar(new LedTabBar(this,"led_tab_control"));
  connect(tabBar(),SIGNAL (selected(int)) ,this,SLOT (tabSelected(int)) );
}

LedTabWidget::~LedTabWidget()
{
}

/* Overloaded */
void LedTabWidget::addTab(QWidget* child,const QString& label,int color,bool on,bool blink)
{
  LedTab* tab=new LedTab(child,label,color,on);
  tab->setBlinkEnabled(blink);

  QTabWidget::addTab(child,tab);
  /* This signal will be emitted when the tab is blinking */
  connect(tab,SIGNAL(repaintTab()),tabBar(),SLOT(repaint()));
}

void LedTabWidget::changeTabState(QWidget* child,bool state)
{
  /* Casting terror ... */
  LedTabBar* bar=(LedTabBar*) tabBar();
  LedTab* tab=bar->tab(child);
  tab->setOn(state);
}

void LedTabWidget::tabSelected(int id)
{
  /* Why do I always have to cast? I hate that! */
  LedTab* tab=(LedTab *) tabBar()->tab(id);

  emit currentChanged(currentPage());
  tab->setOn(false);
}
