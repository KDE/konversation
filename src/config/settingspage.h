/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef KONVISETTINGSPAGE_H
#define KONVISETTINGSPAGE_H

#include <QtGlobal>

class KonviSettingsPage
{
  public:
    KonviSettingsPage() = default;
    virtual ~KonviSettingsPage() {}
    virtual void restorePageToDefaults() = 0;  // function called when the user klicks "Default"
    virtual void saveSettings() = 0;           // function called when the user klicks "Ok" or "Apply"
    virtual void loadSettings() = 0;           // function called when the user opens the page

    virtual bool hasChanged() = 0;             // is to return if any non-KConfigXT settings have changed

  private:
    Q_DISABLE_COPY(KonviSettingsPage)
};

#endif


