//
// C++ Interface: quickbuttonsconfigcontroller
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef QUICKBUTTONSCONFIGCONTROLLER_H
#define QUICKBUTTONSCONFIGCONTROLLER_H

#include "quickbuttons_preferences.h"
#include "konvisettingspage.h"

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
*/

class QuickButtonsConfigController : public QuickButtons_Config, public KonviSettingsPage
{
  Q_OBJECT

  public:
    QuickButtonsConfigController(QWidget* parent, const char* name=NULL);
    ~QuickButtonsConfigController();

    virtual void saveSettings();
    virtual void loadSettings();
    virtual void restorePageToDefaults();

  signals:
    void modified();

  protected slots:
    void entrySelected(QListViewItem* quickButtonEntry);
    void nameChanged(const QString& newName);
    void actionChanged(const QString& newAction);

  protected:
    bool newItemSelected;
};

#endif
