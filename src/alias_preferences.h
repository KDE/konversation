/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef EXALIASPREFERENCES_H
#define EXALIASPREFERENCES_H

#include "alias_preferencesui.h"
#include "konvisettingspage.h"

class Alias_Config : public Alias_ConfigUI, public KonviSettingsPage
{
  Q_OBJECT

 public:
  Alias_Config(QWidget* parent, const char* name = 0);
  ~Alias_Config();

  virtual void restorePageToDefaults();
  virtual void saveSettings();
  virtual void loadSettings();

  virtual bool hasChanged();

 protected slots:
  void newAlias();
  void removeAlias();
  void itemRenamed(QListViewItem* item);

 protected:
  void setAliases(const QStringList &aliasList);
  QStringList currentList();

 private:
  QStringList m_defaultAliasList;  // default aliases
  QStringList m_oldAliasList;      // alias list before last Apply

signals:
    void modified();
};
#endif
