/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagegeneralsettings.h  -  Provides a user interface to customize general settings
  begin:     Fre Nov 15 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef PREFSPAGEGENERALSETTINGS_H
#define PREFSPAGEGENERALSETTINGS_H

#include <prefspage.h>

/*
  @author Dario Abatianni
*/

class QLabel;
class QCheckBox;
class QSpinBox;

class KLineEdit;

class PrefsPageGeneralSettings : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageGeneralSettings(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageGeneralSettings();

  public slots:
    void applyPreferences();

  protected slots:
    void autoReconnectChanged(int state);

  protected:
    KLineEdit* commandCharInput;
    KLineEdit* channelActionInput;
    KLineEdit* notifyActionInput;

    QCheckBox* autoReconnectCheck;
    QCheckBox* autoRejoinCheck;
    QCheckBox* autojoinOnInviteCheck;
    QCheckBox* tabPlacementCheck;
    QCheckBox* blinkingTabsCheck;
    QCheckBox* bringToFrontCheck;
    QCheckBox* fixedMOTDCheck;
    QCheckBox* beepCheck;
    QCheckBox* rawLogCheck;
    QCheckBox* trayIconCheck;
    QCheckBox* trayNotifyCheck;

    QLabel* reconnectTimeoutLabel;
    QSpinBox* reconnectTimeoutSpin;
};

#endif
