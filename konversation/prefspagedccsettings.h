/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagedccsettings.h  -  Provides a user interface to customize DCC settings
  begin:     Wed Oct 23 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef PREFSPAGEDCCSETTINGS_H
#define PREFSPAGEDCCSETTINGS_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class QSpinBox;

class KLineEdit;

class PrefsPageDccSettings : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageDccSettings(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageDccSettings();

  public slots:
    void applyPreferences();

  protected slots:
    void folderButtonClicked();
    void specificSendPortsStateChanged(int state);
    void sendPortsFirstSpinValueChanged(int port);
    void sendPortsLastSpinValueChanged(int port);
    void autoResumeStateChanged(int state);
    void autoGetStateChanged(int state);
    void updateSendPortsWidgets(bool enabled);

  protected:
    KLineEdit* dccFolderInput;
    QSpinBox* dccBufferSpin;
    QSpinBox* dccRollbackSpin;
    QCheckBox* dccSpecificSendPorts;
    QLabel* dccSendPortsFirstLabel;
    QLabel* dccSendPortsLastLabel;
    QSpinBox* dccSendPortsFirstSpin;
    QSpinBox* dccSendPortsLastSpin;
    QCheckBox* dccGetIpFromServer;
    QCheckBox* dccAutoGet;
    QCheckBox* dccAutoResume;
    QCheckBox* dccAddSender;
    QCheckBox* dccCreateFolder;
};

#endif
