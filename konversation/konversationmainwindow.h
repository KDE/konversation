/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationmainwindow.h  -  The main window where all other views go
  begin:     Don Apr 17 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef _KONVERSATIONMAINWINDOW_H_
#define _KONVERSATIONMAINWINDOW_H_

#include <kmainwindow.h>

/*
 Dario Abatianni
*/

class KToggleAction;

class LedTabWidget;
class Server;
class StatusPanel;

class KonversationMainWindow : public KMainWindow
{
  Q_OBJECT
  
  public:
    KonversationMainWindow();
    ~KonversationMainWindow();

    StatusPanel* addStatusView(Server* server);

  protected slots:
    void showToolbar();
    void showStatusbar();
    void showMenubar();
  
  protected:
    void readOptions();
    void addView(QWidget* view,int color,const QString& label,bool on=true);
    void showView(QWidget* view);
    void newText(QWidget* view);

    LedTabWidget* getViewContainer();

    LedTabWidget* viewContainer;

    KToggleAction* showToolBarAction;
    KToggleAction* showStatusBarAction;
    KToggleAction* showMenuBarAction;
};

#endif
