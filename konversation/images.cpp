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

#include <qbitmap.h>
#include <qpainter.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <kstandarddirs.h>

#include "images.h"
#include "konversationapplication.h"

Images::Images()
{
  initializeLeds();
  initializeNickIcons();
}

Images::~Images()
{
}

QIconSet Images::getLed(int color,bool on,bool big) const
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

QIconSet Images::getRedLed(bool on)    const { return getLed(0,on); }
QIconSet Images::getGreenLed(bool on)  const { return getLed(1,on); }
QIconSet Images::getBlueLed(bool on)   const { return getLed(2,on); }
QIconSet Images::getYellowLed(bool on) const { return getLed(3,on); }

QPixmap Images::getNickIcon(NickPrivilege privilege,bool isAway) const
{
  return nickIcons[privilege][isAway?1:0];
}

// private functions //

//TODO: there's room for optimization as pahlibar said. (strm)

// the below two functions were taken from kopeteonlinestatus.cpp.
static QBitmap overlayMasks( const QBitmap *under, const QBitmap *over )
{
  if ( !under && !over ) return QBitmap();
  if ( !under ) return *over;
  if ( !over ) return *under;

  QBitmap result = *under;
  bitBlt( &result, 0, 0, over, 0, 0, over->width(), over->height(), Qt::OrROP );
  return result;
}

static QPixmap overlayPixmaps( const QPixmap &under, const QPixmap &over )
{
  if ( over.isNull() ) return under;

  QPixmap result = under;
  result.setMask( overlayMasks( under.mask(), over.mask() ) );

  QPainter p( &result );
  p.drawPixmap( 0, 0, over );
  return result;
}

// LEDs

void Images::initializeLeds()
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

// NickIcons

void Images::initializeNickIcons()
{

  QString iconTheme = KonversationApplication::preferences.getIconTheme();
  QStringList icons = KGlobal::dirs()->findAllResources("data","konversation/themes/"+iconTheme+"/*.png");

  QStringList::Iterator it = icons.begin();

  QPixmap elementAdmin(*it); 
  ++it;
  QPixmap elementAway(*it);
  ++it;
  QPixmap elementHalfOp(*it);
  ++it;
  QPixmap elementNormal(*it);
  ++it;
  QPixmap elementOp(*it);
  ++it;
  QPixmap elementOwner(*it);
  ++it;
  QPixmap elementVoice(*it);
    
  nickIcons[Normal][0] = elementNormal;
  nickIcons[Normal][1] = overlayPixmaps( nickIcons[Normal][0], elementAway );
  
  nickIcons[Voice][0] = overlayPixmaps( elementNormal, elementVoice );
  nickIcons[Voice][1] = overlayPixmaps( nickIcons[Voice][0], elementAway );
  
  nickIcons[HalfOp][0] = overlayPixmaps( elementNormal, elementHalfOp );
  nickIcons[HalfOp][1] = overlayPixmaps( nickIcons[HalfOp][0], elementAway );
  
  nickIcons[Op][0] = overlayPixmaps( elementNormal, elementOp );
  nickIcons[Op][1] = overlayPixmaps( nickIcons[Op][0], elementAway );
  
  nickIcons[Owner][0] = overlayPixmaps( elementNormal, elementOwner );
  nickIcons[Owner][1] = overlayPixmaps( nickIcons[Owner][0], elementAway );
  
  nickIcons[Admin][0] = overlayPixmaps( elementNormal, elementAdmin );
  nickIcons[Admin][1] = overlayPixmaps( nickIcons[Admin][0], elementAway );
  
  /*
  // why doesn't it work?
  nickIcons[Op][0] = elementNormal;
  bitBlt( &nickIcons[Op][0], 0, 0, &elementOp, 0, 0, -1, -1, Qt::CopyROP );
  nickIcons[Op][1] = nickIcons[Op][0];
  bitBlt( &nickIcons[Op][1], 0, 0, &elementAway, 0, 0, -1, -1, Qt::CopyROP );
  */
}
