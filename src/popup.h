/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Sun May 29 2005
  copyright: (C) 2005 by Isaac Clerencia
  email:     isaac@warp.es
*/


#ifndef POPUP_H
#define POPUP_H

#include <kpassivepopup.h>

#include "chatwindow.h"
#include "konversationmainwindow.h"
/*
  @author Isaac Clerencia
*/

class Popup: public KPassivePopup
{
  Q_OBJECT

  public:
    Popup(KonversationMainWindow* mainWindow, ChatWindow* view, const QString& message);
    ~Popup();
    
  private slots:
    void focusTab();
  
  private:
    KonversationMainWindow* m_mainWindow;
    ChatWindow* m_view;
};

#endif
