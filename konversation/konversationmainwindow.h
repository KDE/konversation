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

class KStdAction;
class LedTabWidget;

class KonversationMainWindow : public KMainWindow
{
  Q_OBJECT
  
  public:
    KonversationMainWindow();
    ~KonversationMainWindow();

  protected:
    void readOptions();

    LedTabWidget* viewContainer;
    
    KStdAction* showToolBarAction;
    KStdAction* showStatusBarAction;
    KStdAction* showMenuBarAction;

};

#endif
