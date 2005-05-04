/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Configuration tab for the OSD Widget
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/

#ifndef PREFSPAGEOSD_H
#define PREFSPAGEOSD_H

#include "osd_preferences.h"

class OSDPreviewWidget;
class Preferences;

class PrefsPageOSD : public OSD_Config
{
  Q_OBJECT

  public:
    PrefsPageOSD(QWidget* newParent,Preferences* newPreferences);
    ~PrefsPageOSD();
    
    void aboutToShow();  // called from PrefsDialog when the page is about to be shown
    void aboutToHide();  // called from PrefsDialog when the page is about to be hidden

  protected slots:
    void slotOSDEnabledChanged(bool on);
    void slotCustomColorsChanged(bool on);
    void slotTextColorChanged(const QColor& color);
    void slotBackgroundColorChanged(const QColor& color);
    void slotScreenChanged(int index);
    void slotDrawShadowChanged(bool on);
    
    void osdFontClicked();
    
    void slotPositionChanged();

  public slots:
    void applyPreferences();

  protected:
    Preferences* preferences;
    void updateFonts();
    QFont osdFont;
    OSDPreviewWidget* m_pOSDPreview;
    bool showingPage;
};

#endif
