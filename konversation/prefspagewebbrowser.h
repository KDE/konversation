/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagewebbrowser.h    -  Configuration tab for Web Browser
  begin:     Thu Jan 8 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

#ifndef PREFSPAGEWEBBROWSER_H
#define PREFSPAGEWEBBROWSER_H

#include <qradiobutton.h>
#include <klineedit.h>

#include "prefspage.h"

/*
  @author Gary Cramblitt
*/

class PrefsPageWebBrowser : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageWebBrowser(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageWebBrowser();

  protected slots:
    void wbActionsBoxClickedSlot();

  public slots:
    void applyPreferences();

  protected:
    QRadioButton* wbUseKdeDefault;
    QRadioButton* wbUseCustomCmd;
    KLineEdit* wbCustomCmd;
};

#endif
