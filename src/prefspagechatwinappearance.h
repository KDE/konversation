/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004-2005 by Peter Simonsson
*/
#ifndef PREFSPAGECHATWINAPPERANCE_H
#define PREFSPAGECHATWINAPPERANCE_H

#include "chatwindowappearance_preferences.h"
class Preferences;

#include <qframe.h>

class PrefsPageChatWinAppearance : public ChatWindowAppearance_Config
{
    Q_OBJECT
        public:
        PrefsPageChatWinAppearance(QWidget* newParent,Preferences* newPreferences);

    public slots:
        void applyPreferences();

    protected slots:
        void setBackgroundImageConfig(bool state);
        void saveBackgroundImage(const QString&);

    protected:
        Preferences* preferences;
};
#endif
