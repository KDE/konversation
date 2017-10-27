/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Oleg Chernkovsiy <kanedias@xaker.ru>
*/

#include "settingspage.h"

// Make at least one of KonviSettingsPage class functions appear out-of-line
// to prevent its vtable being emitted in every translation unit
// i.e. make this translation unit home for this vtable
KonviSettingsPage::~KonviSettingsPage() = default;
