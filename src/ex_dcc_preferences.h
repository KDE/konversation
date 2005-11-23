/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef EXDCCPREFERENCES_H
#define EXDCCPREFERENCES_H

#include "dcc_preferences.h"

class QComboBox;

class DCC_Config_Ext : public DCC_Settings
{
 public:
  DCC_Config_Ext(QWidget* parent, const char* name);
  ~DCC_Config_Ext();
};
#endif
