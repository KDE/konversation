/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagebehaviour.h  -  The preferences panel that holds the behaviour settings
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#ifndef PREFSPAGECHATWINBEHAVIOR_H
#define PREFSPAGECHATWINBEHAVIOR_H

#include "chatwindowbehaviour_preferences.h"

class QWidget;
class Preferences;

class PrefsPageChatWinBehavior : public ChatwindowBehaviour_Config
{
  Q_OBJECT
  public:
    PrefsPageChatWinBehavior(QWidget* newParent, Preferences* newPreferences);
    ~PrefsPageChatWinBehavior();
  public slots:
    void applyPreferences();
  
  protected slots:
    void sortByStatusChanged(int state);
    void moveUp();
    void moveDown();
      
  private:
    Preferences* preferences;
};

#endif
