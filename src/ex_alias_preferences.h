/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef EXALIASPREFERENCES_H
#define EXALIASPREFERENCES_H

#include "alias_preferences.h"

class Alias_Config_Ext : public Alias_Config
{
  Q_OBJECT

 public:
  Alias_Config_Ext(QWidget* parent, const char* name = 0);
  ~Alias_Config_Ext();
  
 protected slots:
  void newAlias();
  void removeAlias();
 public slots:
  void saveAliases();
  
 protected:
  QWidget* parentFrame;

};
#endif
