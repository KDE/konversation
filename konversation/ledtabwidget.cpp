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
#include <kpushbutton.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <qevent.h>

#include "ledtabwidget.h"
#include "ledtab.h"
#include "ledtabbar.h"
#include "chatwindow.h"
#include "server.h"

LedTabWidget::LedTabWidget(QWidget* parent,const char* name) :
              KTabWidget(parent,name)
{
  setTabBar(new LedTabBar(this,"led_tab_bar"));
  connect(tabBar(),SIGNAL (selected(int)) ,this,SLOT (tabSelected(int)) );
  connect(tabBar(),SIGNAL (moveTabLeft(int)), this,SLOT (moveTabLeft(int)) );
  connect(tabBar(),SIGNAL (moveTabRight(int)), this,SLOT (moveTabRight(int)) );
  connect(tabBar(),SIGNAL (closeTab(int)), this,SLOT (tabClosed(int)) );

#ifndef QT_NO_WHEELEVENT
  connect(tabBar(),SIGNAL (wheel(QWheelEvent*)), this,SLOT (processWheelEvent(QWheelEvent*)) );
#endif

  KPushButton* closeBtn = new KPushButton(this);
  closeBtn->setPixmap(KGlobal::iconLoader()->loadIcon("tab_remove", KIcon::Small));
  closeBtn->resize(22, 22);
  closeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  closeBtn->hide();
  setCornerWidget(closeBtn);
  connect(closeBtn, SIGNAL(clicked()), this, SLOT(tabClosed()));
}

LedTabWidget::~LedTabWidget()
{
}

void LedTabWidget::addTab(ChatWindow* child,const QString& label,int color,bool on,int index)
{
  LedTab* tab=new LedTab(child,label,color,on);

  QTabWidget::insertTab(child,tab,index);

  // This signal will be emitted when the tab is blinking
  connect(tab,SIGNAL(repaintTab(LedTab*)),tabBar(),SLOT(repaintLED(LedTab*)));
  // This signal will be emitted when the chat window changes its name
  connect(child,SIGNAL (nameChanged(ChatWindow*,const QString&)),
              this,SLOT (changeName(ChatWindow*,const QString&)) );
}

void LedTabWidget::setTabOnline(ChatWindow* child,bool state)
{
  LedTabBar* bar=tabBar();
  if(bar==0) kdWarning() << "LedTabWidget::setTabOnline(): bar==0!" << endl;
  else
  {
    LedTab* tab=bar->tab(child);
    if(tab==0) kdWarning() << "LedTabWidget::setTabOnline(): tab==0!" << endl;
    else
    {
      tab->setOnline(state);
    }
  }
}

void LedTabWidget::changeTabState(QWidget* child,bool state,bool important,const QString& labelColor)
{
  LedTabBar* bar=tabBar();
  if(bar==0) kdWarning() << "LedTabWidget::changeTabState(): bar==0!" << endl;
  else
  {
    LedTab* tab=bar->tab(child);
    if(tab==0) kdWarning() << "LedTabWidget::changeTabState(): tab==0!" << endl;
    else
    {
      tab->setOn(state,important);
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

void LedTabWidget::moveTabLeft(int id)
{
  int index=tabBar()->indexOf(id);
  if(index) moveTabToIndex(index,index-1);
}

void LedTabWidget::moveTabRight(int id)
{
  int index=tabBar()->indexOf(id);
  if(index<count()-1) moveTabToIndex(index,index+1);
}

void LedTabWidget::moveTabToIndex(int oldIndex,int newIndex)
{
  int color=tabBar()->tabAt(oldIndex)->getColor();
  // remember page
  ChatWindow* pagePointer=static_cast<ChatWindow*>(page(oldIndex));
  // remember page label
  QString pageLabel=label(oldIndex);
  // remove old tab
  removePage(pagePointer);
  // add new tab at desired position
  addTab(pagePointer,pageLabel,color,false,newIndex);
  // make this the current page
  setCurrentPage(newIndex);
}

void LedTabWidget::tabClosed(int id)
{
  //  if id is -1 then get identifier for currently visible tab
  if(id==-1) id=tabBar()->tab(tabBar()->currentTab())->identifier();

  LedTab* tab=tabBar()->tab(id);
  if(tab==0) kdWarning() << "LedTabWidget::closeTab(): tab==0!" << endl;
  else emit closeTab(tab->getWidget());
}

void LedTabWidget::tabClosed()
{
  tabClosed(-1);
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

#ifndef QT_NO_WHEELEVENT
void LedTabWidget::wheelEvent( QWheelEvent *e )
{
  processWheelEvent(e);
}
#endif

#ifndef QT_NO_WHEELEVENT
void LedTabWidget::processWheelEvent(QWheelEvent *e)
{
  if (e->delta() > 0)
  {
    if ( currentPageIndex() == 0 )
      setCurrentPage(count()-1);
    else
      setCurrentPage(currentPageIndex()-1);
  }
  else
  {
    if ( currentPageIndex()+1 >= count() )
      setCurrentPage(0);
    else
      setCurrentPage(currentPageIndex()+1);
  }
}
#endif

#include "ledtabwidget.moc"
