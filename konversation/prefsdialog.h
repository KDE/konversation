/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefsdialog.h  -  This class holds the subpages for the preferences dialog
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef PREFSDIALOG_H
#define PREFSDIALOG_H

#include <qpushbutton.h>
#include <qtabwidget.h>

#include <kdialogbase.h>
#include <klistview.h>

#include "preferences.h"
#include "prefspage.h"

/*
 *@author Dario Abatianni
*/

class PrefsDialog : public KDialogBase
{
  Q_OBJECT

  public:
    PrefsDialog(Preferences* preferences,bool noServer);
    ~PrefsDialog();

  signals:
    void connectToServer(int id);
    void prefsChanged();
    void closed();

  protected slots:
    void connectRequest(int id);

    void slotOk();
    void slotApply();
    void slotCancel();

  protected:
    Preferences* preferences;
//    PrefsPage*   scriptsPage;

    void setPreferences(Preferences* newPrefs);
};

#endif
