/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The preferences panel that holds the behaviour settings
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#ifndef PREFSPAGEBEHAVIOUR_H
#define PREFSPAGEBEHAVIOUR_H

#include "prefspage.h"

class QCheckBox;
class KListView;
class KLineEdit;
class QComboBox;
class QSpinBox;

class PrefsPageBehaviour : public PrefsPage
{
  Q_OBJECT
  public:
    PrefsPageBehaviour(QWidget* newParent, Preferences* newPreferences);
    ~PrefsPageBehaviour();

  public slots:
    void applyPreferences();
  
  protected slots:
    void updateCheckBoxes();
  
  private:
    QCheckBox* trayIconCheck;
    QCheckBox* trayOnlyCheck;
    QCheckBox* trayNotifyCheck;
    QCheckBox* trayNotifyOwnNickOnlyCheck;
    QCheckBox* rawLogCheck;
    QCheckBox* showServerList;
    QCheckBox* m_disableNotifyWhileAwayCheck;
    QCheckBox* useCustomBrowserCheck;
    KLineEdit* browserCmdInput;
    KLineEdit* commandCharInput;
    KLineEdit* ctcpVersionInput;    

    QCheckBox* autoReconnectCheck;
    QCheckBox* autoRejoinCheck;
    QCheckBox* autojoinOnInviteCheck;
    QSpinBox* reconnectTimeoutSpin;
        
    KLineEdit* suffixStartInput;
    KLineEdit* suffixMiddleInput;
    QComboBox* completionModeCBox;
    QCheckBox* m_nickCompletionCaseChBox;
};

#endif
