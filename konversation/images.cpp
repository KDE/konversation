/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  images.cpp  -  provides images
  begin:     Fri Feb 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kstandarddirs.h>
#include <kdebug.h>

#include "images.h"

Images::Images()
{
  // Get standard directories
  KStandardDirs kstd;

  // Find our own image directory
  QString prefix=kstd.findResource("data","konversation/images/");


  // Setup pixmaps for the LEDs
  redLedOn.setPixmap(prefix+"led_red_on.png",QIconSet::Automatic);
  redLedOff.setPixmap(prefix+"led_red_off.png",QIconSet::Automatic);
  greenLedOn.setPixmap(prefix+"led_green_on.png",QIconSet::Automatic);
  greenLedOff.setPixmap(prefix+"led_green_off.png",QIconSet::Automatic);
  blueLedOn.setPixmap(prefix+"led_blue_on.png",QIconSet::Automatic);
  blueLedOff.setPixmap(prefix+"led_blue_off.png",QIconSet::Automatic);
  yellowLedOn.setPixmap(prefix+"led_yellow_on.png",QIconSet::Automatic);
  yellowLedOff.setPixmap(prefix+"led_yellow_off.png",QIconSet::Automatic);

  bigRedLedOn.setPixmap(prefix+"big_led_red_on.png",QIconSet::Automatic);  // USE_MDI
  bigRedLedOff.setPixmap(prefix+"big_led_red_off.png",QIconSet::Automatic);  // USE_MDI
  bigGreenLedOn.setPixmap(prefix+"big_led_green_on.png",QIconSet::Automatic);  // USE_MDI
  bigGreenLedOff.setPixmap(prefix+"big_led_green_off.png",QIconSet::Automatic);  // USE_MDI
  bigBlueLedOn.setPixmap(prefix+"big_led_blue_on.png",QIconSet::Automatic);  // USE_MDI
  bigBlueLedOff.setPixmap(prefix+"big_led_blue_off.png",QIconSet::Automatic);  // USE_MDI
  bigYellowLedOn.setPixmap(prefix+"big_led_yellow_on.png",QIconSet::Automatic);  // USE_MDI
  bigYellowLedOff.setPixmap(prefix+"big_led_yellow_off.png",QIconSet::Automatic);  // USE_MDI
}

Images::~Images()
{
}

QIconSet Images::getLed(int color,bool on,bool big)
{
  QIconSet led;

  // Return a QIconSet for the desired color and state (on/off)
  if(color <0 || color>3) kdWarning() << "Images::getLed(): color " << color << " out of range!" << endl;
  else
  {
    if(big)
    {
      if(color==0 && on) led=bigRedLedOn;
      if(color==0 && !on) led=bigRedLedOff;
      if(color==1 && on) led=bigGreenLedOn;
      if(color==1 && !on) led=bigGreenLedOff;
      if(color==2 && on) led=bigBlueLedOn;
      if(color==2 && !on) led=bigBlueLedOff;
      if(color==3 && on) led=bigYellowLedOn;
      if(color==3 && !on) led=bigYellowLedOff;
    }
    else
    {
      if(color==0 && on) led=redLedOn;
      if(color==0 && !on) led=redLedOff;
      if(color==1 && on) led=greenLedOn;
      if(color==1 && !on) led=greenLedOff;
      if(color==2 && on) led=blueLedOn;
      if(color==2 && !on) led=blueLedOff;
      if(color==3 && on) led=yellowLedOn;
      if(color==3 && !on) led=yellowLedOff;
    }
  }

  return led;
}

QIconSet Images::getRedLed(bool on)     { return getLed(0,on); }
QIconSet Images::getGreenLed(bool on)   { return getLed(1,on); }
QIconSet Images::getBlueLed(bool on)    { return getLed(2,on); }
QIconSet Images::getYellowLed(bool on)  { return getLed(3,on); }
