/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtabwidget.h  -  Extension of QTabWidget to support status LEDs
  begin:     Fri Feb 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef LEDTABWIDGET_H
#define LEDTABWIDGET_H

#include <ktabwidget.h>

#include "images.h"

/*
  @author Dario Abatianni
*/

class ChatWindow;
class LedTabBar;

class LedTabWidget : public KTabWidget
{
  Q_OBJECT

  public:
    LedTabWidget(QWidget* parent,const char* name);
    ~LedTabWidget();

    void addTab(ChatWindow* child,const QString& label,int color,bool on,int index=-1);
    void changeTabState(QWidget* child,bool state,bool important,const QString& labelColor);
    void updateTabs();

  signals:
    void closeTab(QWidget* view);

  public slots:
    void setTabOnline(ChatWindow* child,bool state);

  protected slots:
    void moveTabLeft(int id);
    void moveTabRight(int id);
    void tabSelected(int id);
    void tabClosed(int id);
    void tabClosed();
    void changeName(ChatWindow* view,const QString& newName);

#ifndef QT_NO_WHEELEVENT
    void processWheelEvent(QWheelEvent *e);
#endif

  protected:
    void moveTabToIndex(int oldIndex,int newIndex);
    LedTabBar* tabBar();

    Images images;

#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *e);
#endif

};

#endif
