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

class QFrame;
class QSpinBox;
class QVGroupBox;

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
    void methodToGetOwnIpComboBoxActivated(int methodId);
    void sendPortsFirstSpinValueChanged(int port);
    void sendPortsLastSpinValueChanged(int port);
    void chatPortsFirstSpinValueChanged(int port);
    void chatPortsLastSpinValueChanged(int port);
    void autoResumeStateChanged(int state);
    void autoGetStateChanged(int state);

  protected:
    KLineEdit* dccFolderInput;
    QSpinBox* dccBufferSpin;
    QCheckBox* dccAutoGet;
    QCheckBox* dccAutoResume;
    QCheckBox* dccAddSender;
    QCheckBox* dccCreateFolder;
    QComboBox* dccMethodToGetOwnIpComboBox;
    QFrame* dccSpecificOwnIpFrame;
    KLineEdit* dccSpecificOwnIpInput;
    QCheckBox* dccSpecificSendPortsCheckBox;
    QSpinBox* dccSendPortsFirstSpin;
    QSpinBox* dccSendPortsLastSpin;
    QCheckBox* dccSpecificChatPortsCheckBox;
    QSpinBox* dccChatPortsFirstSpin;
    QSpinBox* dccChatPortsLastSpin;
    QCheckBox* dccFastSend;
    QSpinBox* dccSendTimeoutSpin;
};

#endif
