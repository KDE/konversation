/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtab.cpp  -  Tab for QTabWidget with activity LED
  begin:     Fri Feb 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "ledtab.h"

LedTab::LedTab(QWidget* newWidget,const QString& text,int newColor,bool state) :
        QTab(text)
{
  /* First of all set up the icons */
  iconOn=images.getLed(newColor,true);
  iconOff=images.getLed(newColor,false);

  color=newColor;
  blinkOn=true;
  widget=newWidget;

  setOn(state);

  connect(&blinkTimer,SIGNAL(timeout()),this,SLOT(blinkTimeout()));

  blinkTimer.start(500);
}

LedTab::~LedTab()
{
}

void LedTab::blinkTimeout()
{
  if(on)
  {
    /* if the user wants us to blink, toggle LED blink status */
    if(doBlink) blinkOn=!blinkOn;
    /* else LED should be always on */
    else blinkOn=true;
    /* draw the new LED */
    setIconSet((blinkOn) ? iconOn : iconOff);
  }
}

void LedTab::setOn(bool state)
{
  on=state;
  setIconSet((on) ? iconOn : iconOff);
}

void LedTab::setIconSet(const QIconSet& icon)
{
  delete iconSet();
  QTab::setIconSet(icon);
  emit repaintTab();
}

QWidget* LedTab::getWidget()
{
  return widget;
}
