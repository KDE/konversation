/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtabbar.h  -  description
  begin:     Sun Feb 24 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef LEDTABBAR_H
#define LEDTABBAR_H

#include <qtabbar.h>

#include "ledtab.h"

/*
  @author Dario Abatianni
*/

class LedTabBar : public QTabBar
{
  public:
    LedTabBar(QWidget* parent,char* name);
    ~LedTabBar();
    LedTab* tab(QWidget* widget);
};

#endif
