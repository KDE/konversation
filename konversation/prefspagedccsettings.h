/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides a user interface to customize DCC settings
  begin:     Wed Oct 23 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef PREFSPAGEDCCSETTINGS_H
#define PREFSPAGEDCCSETTINGS_H

#include "dcc_preferences.h"
class Preferences;

/*
  @author Dario Abatianni
*/

class PrefsPageDccSettings : public DCC_Settings
{
  Q_OBJECT

  public:
    PrefsPageDccSettings(QWidget* newParent,Preferences* newPreferences);

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
    Preferences* preferences;
};

#endif
