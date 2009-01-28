/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005-2006 Eike Hein <hein@kde.org>
*/

#include "images.h"
#include "common.h"
#include "konversationapplication.h"

#include <qbitmap.h>
#include <qpainter.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <QPixmap>

#include <kdebug.h>
#include <kiconloader.h>
#include <kstandarddirs.h>


using namespace Konversation;

Images::Images()
{
    initializeLeds();
    initializeNickIcons();
    initializeKimifaceIcons();

    m_closeIcon = KGlobal::iconLoader()->loadIcon("fileclose",KIcon::Small);
    m_disabledCloseIcon = KGlobal::iconLoader()->loadIconSet("fileclose",KIcon::Small).pixmap(QIcon::Small, false);
}

Images::~Images()
{
}

QIcon Images::getKimproxyAway() const { return kimproxyAway; }
QIcon Images::getKimproxyOnline() const { return kimproxyOnline; }
QIcon Images::getKimproxyOffline() const { return kimproxyOffline; }

QPixmap Images::getNickIcon(NickPrivilege privilege,bool isAway) const
{
    return nickIcons[privilege][isAway?1:0];
}

void Images::initializeLeds()
{
    m_serverColor = "steelblue";
    m_systemColor = Preferences::tabNotificationsSystemColor();
    m_msgsColor = Preferences::tabNotificationsMsgsColor();
    m_privateColor = Preferences::tabNotificationsPrivateColor();
    m_eventsColor = Preferences::tabNotificationsEventsColor();
    m_nickColor = Preferences::tabNotificationsNickColor();
    m_highlightsColor = Preferences::tabNotificationsHighlightsColor();

    // m_serverLedOn = getLed(m_serverColor,true);
    m_serverLedOff = getLed(m_serverColor,false);
    m_systemLedOn = getLed(m_systemColor,true);
    m_systemLedOff = getLed(m_systemColor,false);
    m_msgsLedOn = getLed(m_msgsColor,true);
    m_msgsLedOff = getLed(m_msgsColor,false);
    m_privateLedOn = getLed(m_privateColor,true);
    m_privateLedOff = getLed(m_privateColor,false);
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

void Images::updateIcons()
{
    m_closeIcon = KGlobal::iconLoader()->loadIcon("fileclose",KIcon::Small);
    m_disabledCloseIcon = KGlobal::iconLoader()->loadIconSet("fileclose",KIcon::Small).pixmap(QIcon::Small, false);
}

QIcon Images::getLed(QColor col,bool state)
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

    QIcon result;
    result.setPixmap(dest,QIcon::Automatic);
    return dest;
}

QIcon Images::getServerLed(bool state)
{
    if (state)
        return m_serverLedOn;
    else
        return m_serverLedOff;
}

QIcon Images::getSystemLed(bool state)
{
    if (Preferences::tabNotificationsSystemColor()!=m_systemColor)
    {
        if (state)
            return getLed(Preferences::tabNotificationsSystemColor(),true);
        else
            return getLed(Preferences::tabNotificationsSystemColor(),false);
    }
    else
    {
        if (state)
            return m_systemLedOn;
        else
            return m_systemLedOff;
    }
}

QIcon Images::getMsgsLed(bool state)
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

QIcon Images::getPrivateLed(bool state)
{
    if (Preferences::tabNotificationsPrivateColor()!=m_privateColor)
    {
        if (state)
            return getLed(Preferences::tabNotificationsPrivateColor(),true);
        else
            return getLed(Preferences::tabNotificationsPrivateColor(),false);
    }
    else
    {
        if (state)
            return m_privateLedOn;
        else
            return m_privateLedOff;
    }
}

QIcon Images::getEventsLed()
{
    if (Preferences::tabNotificationsEventsColor()!=m_eventsColor)
        return getLed(Preferences::tabNotificationsEventsColor(),true);
    else
        return m_eventsLedOn;
}

QIcon Images::getNickLed()
{
    if (Preferences::tabNotificationsNickColor()!=m_nickColor)
        return getLed(Preferences::tabNotificationsNickColor(),true);
    else
        return m_nickLedOn;
}

QIcon Images::getHighlightsLed()
{
    if (Preferences::tabNotificationsHighlightsColor()!=m_highlightsColor)
        return getLed(Preferences::tabNotificationsHighlightsColor(),true);
    else
        return m_highlightsLedOn;
}

#include "images.moc"
