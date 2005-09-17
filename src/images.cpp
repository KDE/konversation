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

#include <qbitmap.h>
#include <qpainter.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <kstandarddirs.h>

#include "images.h"
#include "common.h"
#include "konversationapplication.h"

using namespace Konversation;

Images::Images()
{
    initializeLeds();
    initializeNickIcons();
    initializeKimifaceIcons();
}

Images::~Images()
{
}

QIconSet Images::getLed(int color,bool on) const
{
    QIconSet led;

    // Return a QIconSet for the desired color and state (on/off)
    if(color <0 || color>3)
        kdWarning() << "Images::getLed(): color " << color << " out of range!" << endl;
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

    return led;
}

QIconSet Images::getRedLed(bool on)    const { return getLed(0,on); }
QIconSet Images::getGreenLed(bool on)  const { return getLed(1,on); }
QIconSet Images::getBlueLed(bool on)   const { return getLed(2,on); }
QIconSet Images::getYellowLed(bool on) const { return getLed(3,on); }

QIconSet Images::getKimproxyAway() const { return kimproxyAway; }
QIconSet Images::getKimproxyOnline() const { return kimproxyOnline; }
QIconSet Images::getKimproxyOffline() const { return kimproxyOffline; }

QPixmap Images::getNickIcon(NickPrivilege privilege,bool isAway) const
{
    return nickIcons[privilege][isAway?1:0];
}

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

}

void Images::initializeKimifaceIcons()
{
    kimproxyAway = KGlobal::iconLoader()->loadIconSet("kimproxyaway",KIcon::Small);
    kimproxyOnline = KGlobal::iconLoader()->loadIconSet("kimproxyonline",KIcon::Small);
    kimproxyOffline = KGlobal::iconLoader()->loadIconSet("kimproxyoffline",KIcon::Small);
}

// NickIcons

void Images::initializeNickIcons()
{

    QString iconTheme = Preferences::iconTheme();
    QStringList icons = KGlobal::dirs()->findAllResources("data","konversation/themes/"+iconTheme+"/*.png");

    if( icons.count() < 7 )                       // Sanity
        icons = KGlobal::dirs()->findAllResources("data","konversation/themes/default/*.png");

    icons.sort();
    QStringList::ConstIterator it = icons.begin();

    /* The list is sorted alphabetically. */

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
