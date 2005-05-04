/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  provides images
  begin:     Fri Feb 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IMAGES_H
#define IMAGES_H

#include <qiconset.h>
#include <qpixmap.h>

/*
  @author Dario Abatianni
*/

/**
 * Do not create an instance of this class yourself.
 * use KonversationApplication::instance()->images().
 */

class Images
{
  public:
    enum NickPrivilege
    {
      Normal=0,
      Voice,
      HalfOp,
      Op,
      Owner,
      Admin,
      _NickPrivilege_COUNT
    };
    
    Images();
    virtual ~Images();
    
    QIconSet getLed(int color,bool on,bool big=false) const; // big for USE_MDI tab bar
    
    QIconSet getRedLed(bool on) const;
    QIconSet getGreenLed(bool on) const;
    QIconSet getBlueLed(bool on) const;
    QIconSet getYellowLed(bool on) const;

    QIconSet getKimproxyAway() const;
    QIconSet getKimproxyOnline() const;
    QIconSet getKimproxyOffline() const;
    
    QPixmap getNickIcon(NickPrivilege privilege,bool isAway=false) const;
    void initializeNickIcons();

  protected:
    void initializeLeds();
    void initializeKimifaceIcons();
    
    QIconSet redLedOn;
    QIconSet redLedOff;
    QIconSet greenLedOn;
    QIconSet greenLedOff;
    QIconSet blueLedOn;
    QIconSet blueLedOff;
    QIconSet yellowLedOn;
    QIconSet yellowLedOff;

    QIconSet bigRedLedOn;
    QIconSet bigRedLedOff;
    QIconSet bigGreenLedOn;
    QIconSet bigGreenLedOff;
    QIconSet bigBlueLedOn;
    QIconSet bigBlueLedOff;
    QIconSet bigYellowLedOn;
    QIconSet bigYellowLedOff;

    QIconSet kimproxyAway;
    QIconSet kimproxyOnline;
    QIconSet kimproxyOffline;
    

    QPixmap nickIcons[_NickPrivilege_COUNT][2];  // [privilege][away]
};

#endif
