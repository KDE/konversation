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
*/

#include <kdebug.h>

#include "ledtabwidget.h"
#include "ledtab.h"
#include "ledtabbar.h"
#include "chatwindow.h"

LedTabWidget::LedTabWidget(QWidget* parent,const char* name) :
              QTabWidget(parent,name)
{
  setTabBar(new LedTabBar(this,"led_tab_bar"));
  connect(tabBar(),SIGNAL (selected(int)) ,this,SLOT (tabSelected(int)) );
  connect(tabBar(),SIGNAL (closeTab(int)), this,SLOT (tabClosed(int)) );
}

LedTabWidget::~LedTabWidget()
{
}

void LedTabWidget::addTab(ChatWindow* child,const QString& label,int color,bool on)
{
  LedTab* tab=new LedTab(child,label,color,on);

  QTabWidget::addTab(child,tab);
  // This signal will be emitted when the tab is blinking
  connect(tab,SIGNAL(repaintTab(LedTab*)),tabBar(),SLOT(repaintLED(LedTab*)));
  // This signal will be emitted when the chat window changes its name
  connect(child,SIGNAL (nameChanged(ChatWindow*,const QString&)),
              this,SLOT (changeName(ChatWindow*,const QString&)) );
}

void LedTabWidget::changeTabState(QWidget* child,bool state,const QString& labelColor)
{
  LedTabBar* bar=tabBar();
  if(bar==0) kdWarning() << "LedTabWidget::changeTabState(): bar==0!" << endl;
  else
  {
    LedTab* tab=bar->tab(child);
    if(tab==0) kdWarning() << "LedTabWidget::changeTabState(): tab==0!" << endl;
    else
    {
      tab->setOn(state);
      if(!labelColor.isEmpty() || !state)
        tab->setLabelColor(labelColor);
    }
  }
}

void LedTabWidget::tabSelected(int id)
{
  LedTab* tab=tabBar()->tab(id);
  if(tab==0) kdWarning() << "LedTabWidget::tabSelected(): tab==0!" << endl;
  else
  {
    emit currentChanged(currentPage());
    tab->setOn(false);
  }
}

void LedTabWidget::tabClosed(int id)
{
  //  if id is -1 then get identifier for currently visible tab
  if(id==-1) id=tabBar()->tab(tabBar()->currentTab())->identifier();

  LedTab* tab=tabBar()->tab(id);
  if(tab==0) kdWarning() << "LedTabWidget::closeTab(): tab==0!" << endl;
  else emit closeTab(tab->getWidget());
}

// reimplemented to avoid casts in active code
LedTabBar* LedTabWidget::tabBar()
{
  return static_cast<LedTabBar*>(QTabWidget::tabBar());
}

void LedTabWidget::updateTabs()
{
  // reliably redraw the tab bar by resetting the label of the first tab
  changeTab(page(0),label(0));
  tabBar()->updateTabs();
  update();
}

void LedTabWidget::changeName(ChatWindow* view,const QString& newName)
{
  changeTab(view,newName);
}

#include "ledtabwidget.moc"
