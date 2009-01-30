/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef KONVISETTINGSPAGE_H
#define KONVISETTINGSPAGE_H

class KonviSettingsPage
{
  public:
    virtual ~KonviSettingsPage() {}
    virtual void restorePageToDefaults() = 0;  // function called when the user klicks "Default"
    virtual void saveSettings() = 0;           // function called when the user klicks "Ok" or "Apply"
    virtual void loadSettings() = 0;           // function called when the user opens the page

    virtual bool hasChanged() = 0;             // is to return if any non-KConfigXT settings have changed
};

#endif


