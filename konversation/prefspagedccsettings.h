/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagedccsettings.h  -  description
  begin:     Wed Oct 23 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef PREFSPAGEDCCSETTINGS_H
#define PREFSPAGEDCCSETTINGS_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class PrefsPageDccSettings : public PrefsPage
{
  Q_OBJECT

  public: 
    PrefsPageDccSettings(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageDccSettings();

  protected slots:
    void folderInputChanged(const QString& newPath);
    void bufferValueChanged(int newBuffer);
    void rollbackValueChanged(int newRollback);
    void autoGetChanged(int state);
    void addSenderChanged(int state);
    void createFolderChanged(int state);
};

#endif
