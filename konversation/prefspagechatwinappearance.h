/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#ifndef PREFSPAGECHATWINAPPERANCE_H
#define PREFSPAGECHATWINAPPERANCE_H

#include <qfont.h>
#include <kurlrequester.h>
#include <qhgroupbox.h>

#include "prefspage.h"

class QLabel;
class QComboBox;
class QCheckBox;

class PrefsPageChatWinAppearance : public PrefsPage
{
  Q_OBJECT
  public:
    PrefsPageChatWinAppearance(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageChatWinAppearance();
  
  public slots:
    void applyPreferences();

  protected slots:
    void textFontClicked();
    void listFontClicked();
    void setBackgroundImageConfig(bool state);
    void saveBackgroundImage(const QString&);

    void timestampingChanged(int state);
            
  protected:
    void updateFonts();

    QLabel* textPreviewLabel;
    QLabel* listPreviewLabel;
    QFont textFont;
    QFont listFont;
    QCheckBox* fixedMOTDCheck;

    QCheckBox* doTimestamping;
    QCheckBox* showDate;
    QLabel* formatLabel;
    QComboBox* timestampFormat;
    
    QCheckBox* autoUserhostCheck;
    QCheckBox* showQuickButtons;
    QCheckBox* showModeButtons;
    QCheckBox* showTopic;
    QCheckBox* m_showNicknameBoxCheck;
    QCheckBox* showBackgroundImage;
    KURLRequester* backgroundURL;
};

#endif
