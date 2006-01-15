/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
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

 protected slots:
  void newAlias();
  void removeAlias();
 protected:
  void setAliases(const QStringList &aliasList);

 private:
  QStringList m_defaultAliasList;

	  

signals:
    void modified();
};
#endif
