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
#include <kcolorcombo.h>

/*
  @author Christian Muehlhaeuser
*/

class QCheckBox;
class QLabel;
class KComboBox;
class KIntSpinBox;

class OSDPreviewWidget;

class PrefsPageOSD : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageOSD(QFrame* newParent,Preferences* newPreferences);
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
    void updateFonts();
    QFont osdFont;

    QCheckBox* useOSDCheck;
    QCheckBox* drawShadowsCheck;
    QCheckBox* useCustomColorsCheck;
    QCheckBox* osdShowOwnNick;
    QCheckBox* osdShowChannel;
    QCheckBox* osdShowQuery;
    QCheckBox* osdShowChannelEvent;

    KIntSpinBox* osdDurationSpin;
    KComboBox* osdScreenCombo;
    
    QPushButton* osdFontButton;
    QLabel* osdFontLabel;
    QLabel* osdPreviewLabel;
    QLabel* osdTextColorLabel;
    QLabel* osdBackgroundColorLabel;

    KColorCombo* osdTextColorChooser;
    KColorCombo* osdBackgroundColorChooser;

    QVGroupBox* osdColorsBox;
    QVGroupBox* osdOthersBox;
    QVGroupBox* osdActionsBox;
    
    OSDPreviewWidget* m_pOSDPreview;
    
    bool showingPage;
};

#endif
