/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  images.h  -  provides images
  begin:     Fri Feb 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IMAGES_H
#define IMAGES_H

#include <qiconset.h>

/*
  @author Dario Abatianni
*/

class Images
{
  public:
    Images();
    ~Images();

    QIconSet getLed(int color,bool on);

    QIconSet getRedLed(bool on);
    QIconSet getGreenLed(bool on);
    QIconSet getBlueLed(bool on);
    QIconSet getYellowLed(bool on);

  protected:
    QIconSet redLedOn;
    QIconSet redLedOff;
    QIconSet greenLedOn;
    QIconSet greenLedOff;
    QIconSet blueLedOn;
    QIconSet blueLedOff;
    QIconSet yellowLedOn;
    QIconSet yellowLedOff;
};

#endif
