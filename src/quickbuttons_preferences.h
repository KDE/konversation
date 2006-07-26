/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef QUICKBUTTONSCONFIG_H
#define QUICKBUTTONSCONFIG_H

#include "quickbuttons_preferencesui.h"
#include "konvisettingspage.h"

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
*/

class QuickButtons_Config : public QuickButtons_ConfigUI, public KonviSettingsPage
{
  Q_OBJECT

  public:
    QuickButtons_Config(QWidget* parent, const char* name=NULL);
    ~QuickButtons_Config();

    virtual void saveSettings();
    virtual void loadSettings();
    virtual void restorePageToDefaults();

    virtual bool hasChanged();

  signals:
    void modified();

  protected slots:
    void entrySelected(QListViewItem* quickButtonEntry);
    void nameChanged(const QString& newName);
    void actionChanged(const QString& newAction);
    void addEntry();
    void removeEntry();

  protected:
    void setButtonsListView(const QStringList &buttonList);

    bool m_newItemSelected;

    QStringList m_oldButtonList;
    QStringList currentButtonList();
};

#endif
