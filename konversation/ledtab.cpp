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

#include <kdebug.h>

#include "konversationapplication.h"
#include "ledtab.h"

LedTab::LedTab(QWidget* newWidget,const QString& label,int newColor,bool state) :
        QTab(label)
{
  // First of all set up the icons
  iconOn=images.getLed(newColor,true);
  iconOff=images.getLed(newColor,false);

  installEventFilter(this);

  color=newColor;
  blinkOn=true;
  widget=newWidget;
  labelColor=QString::null;

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
    // if the user wants us to blink, toggle LED blink status
    if(KonversationApplication::preferences.getBlinkingTabs())
    {
      blinkOn=!blinkOn;
      // draw the new LED
      setIconSet((blinkOn) ? iconOn : iconOff);
    }
    // else LED should be always on
    else
    {
      // only change state when LED was off until now
      if(!blinkOn)
      {
        // switch LED on
        blinkOn=true;
        setIconSet((blinkOn) ? iconOn : iconOff);
      }
    }
  }
}

void LedTab::setOn(bool state)
{
  on=state;
  setIconSet((on) ? iconOn : iconOff);
}

void LedTab::setLabelColor(const QString& newLabelColor)
{
  labelColor=newLabelColor;
  emit repaintTab(this);
}

const QString& LedTab::getLabelColor()
{
  return (blinkOn) ? labelColor : QString::null;
}

void LedTab::setIconSet(const QIconSet& icon)
{
  delete iconSet();
  QTab::setIconSet(icon);
  emit repaintTab(this);
}

QWidget* LedTab::getWidget()
{
  return widget;
}

int LedTab::getColor()
{
  return color;
}

#include "ledtab.moc"
