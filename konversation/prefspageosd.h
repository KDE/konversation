/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageosd.h    -  Configuration tab for the OSD Widget
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/

#ifndef PREFSPAGEOSD_H
#define PREFSPAGEOSD_H

#include "prefspage.h"
#include <qvgroupbox.h>

/*
  @author Christian Muehlhaeuser
*/

class QCheckBox;
class QLabel;

class PrefsPageOSD : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageOSD(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageOSD();

  protected slots:
    void osdFontClicked();
    void osdUsageChanged(int state);

  public slots:
    void applyPreferences();

  protected:
    void updateFonts();
    QFont osdFont;

    QCheckBox* useOSDCheck;
    QCheckBox* osdShowOwnNick;
    QCheckBox* osdShowChannel;
    QCheckBox* osdShowQuery;
    QCheckBox* osdShowChannelEvent;

    QPushButton* osdFontButton;
    QLabel* osdFontLabel;
    QLabel* osdPreviewLabel;

    QVGroupBox* osdActionsBox;
};

#endif
