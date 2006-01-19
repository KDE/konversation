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
  copyright: (C) 2005 by Eike Hein
  email:     sho@eikehein.com
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

    m_closeIcon = KGlobal::iconLoader()->loadIcon("fileclose",KIcon::Small);
}

Images::~Images()
{
}

QIconSet Images::getKimproxyAway() const { return kimproxyAway; }
QIconSet Images::getKimproxyOnline() const { return kimproxyOnline; }
QIconSet Images::getKimproxyOffline() const { return kimproxyOffline; }

QPixmap Images::getNickIcon(NickPrivilege privilege,bool isAway) const
{
    return nickIcons[privilege][isAway?1:0];
}

void Images::initializeLeds()
{
    m_serverColor = "steelblue";
    m_msgsColor = Preferences::tabNotificationsMsgsColor();
    m_eventsColor = Preferences::tabNotificationsEventsColor();
    m_nickColor = Preferences::tabNotificationsNickColor();
    m_highlightsColor = Preferences::tabNotificationsHighlightsColor();

    m_serverLedOn = getLed(m_serverColor,true);
    m_serverLedOff= getLed(m_serverColor,false);
    m_msgsLedOn= getLed(m_msgsColor,true);
    m_msgsLedOff= getLed(m_msgsColor,false);
    m_eventsLedOn = getLed(m_eventsColor,true);
    m_nickLedOn = getLed(m_nickColor,true);
    m_highlightsLedOn = getLed(m_highlightsColor,true);
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

    if( icons.count() < 7 ) // Sanity
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

QIconSet Images::getLed(QColor col,bool state)
{
    QColor color;
    QPainter paint;
    QBrush brush;
    QPen pen;

    if (state==false)
        color = col.dark(180);
    else
        color = col;

    int scale = 3;
    int width = 12 * scale;

    QPixmap *tmpMap = 0;

    tmpMap = new QPixmap(width + 6, width + 6);
    QWidget tmpWidget;
    tmpMap->fill(tmpWidget.paletteBackgroundColor());
    paint.begin(tmpMap);

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    brush.setStyle( QBrush::SolidPattern );
    brush.setColor( color );
    paint.setBrush( brush );

    // Draw a "flat" LED with the given color
    paint.drawEllipse( scale, scale, width - scale*2, width - scale*2 );

    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth( 2 * scale );

    // shrink the light on the LED to a size about 2/3 of the complete LED
    int pos = width/5 + 1;
    int light_width = width;
    light_width *= 2;
    light_width /= 3;

    // Calculate the LEDs "light factor"
    int light_quote = (130*2/(light_width?light_width:1))+100;

    // Draw the bright spot on the LED
    while (light_width)
    {
        color = color.light( light_quote ); // make color lighter
        pen.setColor( color );              // set color as pen color
        paint.setPen( pen );                // select the pen for drawing
        paint.drawEllipse( pos, pos, light_width, light_width ); // draw the ellipse (circle)
        light_width--;
        if (!light_width)
            break;
        paint.drawEllipse( pos, pos, light_width, light_width );
        light_width--;
        if (!light_width)
            break;
        paint.drawEllipse( pos, pos, light_width, light_width );
        pos++; light_width--;
    }

    // Draw border
    pen.setWidth( scale + 1 );
    color = QColor("#7D7D7D");
    pen.setColor( color );             // Set the pen accordingly
    paint.setPen( pen );               // Select pen for drawing
    brush.setStyle( QBrush::NoBrush ); // Switch off the brush
    paint.setBrush( brush );           // This avoids filling of the ellipse
    paint.drawEllipse( 2, 2, width, width );
    paint.end();

    tmpMap->setMask(tmpMap->createHeuristicMask(true));

    // painting done
    QImage i = tmpMap->convertToImage();
    delete tmpMap;

    i.setAlphaBuffer(true);
    width /= 3;
    i = i.smoothScale(width, width);

    QPixmap dest = i;

    QIconSet result;
    result.setPixmap(dest,QIconSet::Automatic);
    return dest;
}

QIconSet Images::getServerLed(bool state)
{
    if (state)
        return m_serverLedOn;
    else
        return m_serverLedOff;
}

QIconSet Images::getMsgsLed(bool state)
{
    if (Preferences::tabNotificationsMsgsColor()!=m_msgsColor)
    {
        if (state)
            return getLed(Preferences::tabNotificationsMsgsColor(),true);
        else
            return getLed(Preferences::tabNotificationsMsgsColor(),false);
    }
    else
    {
        if (state)
            return m_msgsLedOn;
        else
            return m_msgsLedOff;
    }
}

QIconSet Images::getEventsLed()
{
    if (Preferences::tabNotificationsEventsColor()!=m_eventsColor)
        return getLed(Preferences::tabNotificationsEventsColor(),true);
    else
        return m_eventsLedOn;
}

QIconSet Images::getNickLed()
{
    if (Preferences::tabNotificationsNickColor()!=m_nickColor)
        return getLed(Preferences::tabNotificationsNickColor(),true);
    else
        return m_nickLedOn;
}

QIconSet Images::getHighlightsLed()
{
    if (Preferences::tabNotificationsHighlightsColor()!=m_highlightsColor)
        return getLed(Preferences::tabNotificationsHighlightsColor(),true);
    else
        return m_highlightsLedOn;
}
