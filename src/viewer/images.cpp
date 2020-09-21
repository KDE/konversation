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
#include "application.h"

#include <QIconEngine>
#include <QPainter>

#include <QStandardPaths>
#include <QDebug>


using namespace Konversation;


class LedIconEngine : public QIconEngine
{
    public:
        LedIconEngine(const QColor &color, bool state)
            : QIconEngine(), m_color(color), m_state(state)
        {
        }

        // QIconEngine
        void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
        QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
        QIconEngine* clone() const Q_DECL_OVERRIDE;

    private:
        const QColor m_color;
        const bool m_state;
};

void LedIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(mode);
    Q_UNUSED(state);
    QColor color;
    QBrush brush;
    QPen pen;

    if (!m_state)
        color = m_color.darker(180);
    else
        color = m_color;

    int scale = rect.width() / 12;
    int width = rect.width();

    // Set the brush to Qt::SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    brush.setStyle( Qt::SolidPattern );
    brush.setColor( color );
    painter->setBrush( brush );

    // Draw a "flat" LED with the given color
    painter->drawEllipse( scale, scale, width - scale*2, width - scale*2 );

    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth( rect.width() / 12 );

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
        color = color.lighter( light_quote ); // make color lighter
        pen.setColor( color );              // set color as pen color
        painter->setPen( pen );                // select the pen for drawing
        painter->drawEllipse( pos, pos, light_width, light_width ); // draw the ellipse (circle)
        light_width--;
        if (!light_width)
            break;
        painter->drawEllipse( pos, pos, light_width, light_width );
        light_width--;
        if (!light_width)
            break;
        painter->drawEllipse( pos, pos, light_width, light_width );
        pos++; light_width--;
    }

    // Draw border
    pen.setWidth( rect.width() / 12 + 1 );
    color = QColor("#7D7D7D");
    pen.setColor( color );             // Set the pen accordingly
    painter->setPen( pen );               // Select pen for drawing
    brush.setStyle( Qt::NoBrush ); // Switch off the brush
    painter->setBrush( brush );           // This avoids filling of the ellipse
    painter->drawEllipse( 2, 2, rect.width() - 4, rect.width() - 4 );
}

QPixmap LedIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pix(size);
    {
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        paint(&p, QRect(QPoint(0, 0), size), mode, state);
    }
    return pix;
}

QIconEngine* LedIconEngine::clone() const
{
    LedIconEngine* newEngine = new LedIconEngine(m_color, m_state);

    return newEngine;
}


Images::Images()
{
    initializeLeds();
    initializeNickIcons();
}

Images::~Images()
{
}

QPixmap Images::getNickIcon(NickPrivilege privilege,bool isAway) const
{
    return nickIcons[privilege][isAway?1:0];
}

QString Images::getNickIconPath(NickPrivilege privilege) const
{
    return nickIconPaths[privilege];
}

QString Images::getNickIconAwayPath() const
{
    return nickIconAwayPath;
}

void Images::initializeLeds()
{
    m_serverColor = "steelblue";
    m_systemColor = Preferences::self()->tabNotificationsSystemColor();
    m_msgsColor = Preferences::self()->tabNotificationsMsgsColor();
    m_privateColor = Preferences::self()->tabNotificationsPrivateColor();
    m_eventsColor = Preferences::self()->tabNotificationsEventsColor();
    m_nickColor = Preferences::self()->tabNotificationsNickColor();
    m_highlightsColor = Preferences::self()->tabNotificationsHighlightsColor();

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

// NickIcons

void Images::initializeNickIcons()
{

    QString iconTheme = Preferences::self()->iconTheme();
    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "konversation/themes/"+iconTheme, QStandardPaths::LocateDirectory);
    QStringList icons;
    bool isDefaultTheme = (iconTheme == QLatin1String("default"));

    foreach(const QString& dir, dirs)
    {
        QDir themedir(dir);

        foreach(const QString& file, themedir.entryList(QStringList() << "*.png", QDir::Files))
        {
            icons.append(dir + QLatin1Char('/') + file);
        }
    }

    if( icons.count() < 7 ) // Sanity
    {
        dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/themes/default"), QStandardPaths::LocateDirectory);

        foreach(const QString& dir, dirs)
        {
            QDir themedir(dir);

            foreach(const QString& file, themedir.entryList(QStringList() << "*.png", QDir::Files))
            {
                icons.append(dir + QLatin1Char('/') + file);
            }
        }
        isDefaultTheme = true;
    }
    if ( icons.count() < 7 || icons.count() > 8) // Sanity
        return;
    icons.sort();
    QStringList::ConstIterator it = icons.constBegin();

    /* The list is sorted alphabetically. */

    QPixmap elementAdmin(*it);
    nickIconPaths[Admin] = *it;
    ++it;
    QPixmap elementAway(*it);
    nickIconAwayPath = *it;
    ++it;
    QPixmap elementAwayStacked;
    if (icons.count() == 8 && isDefaultTheme) {
        elementAwayStacked = (*it);
        ++it;
    }
    QPixmap elementHalfOp(*it);
    nickIconPaths[HalfOp] = *it;
    ++it;
    QPixmap elementNormal(*it);
    nickIconPaths[Normal] = *it;
    ++it;
    QPixmap elementOp(*it);
    nickIconPaths[Op] = *it;
    ++it;
    QPixmap elementOwner(*it);
    nickIconPaths[Owner] = *it;
    ++it;
    QPixmap elementVoice(*it);
    nickIconPaths[Voice] = *it;

    nickIcons[Normal][0] = elementNormal;
    nickIcons[Normal][1] = overlayPixmaps( nickIcons[Normal][0], elementAway );

    nickIcons[Voice][0] = overlayPixmaps( elementNormal, elementVoice );
    nickIcons[Voice][1] = overlayPixmaps( nickIcons[Voice][0], elementAwayStacked.isNull() ? elementAway : elementAwayStacked );

    nickIcons[HalfOp][0] = overlayPixmaps( elementNormal, elementHalfOp );
    nickIcons[HalfOp][1] = overlayPixmaps( nickIcons[HalfOp][0], elementAwayStacked.isNull() ? elementAway : elementAwayStacked );

    nickIcons[Op][0] = overlayPixmaps( elementNormal, elementOp );
    nickIcons[Op][1] = overlayPixmaps( nickIcons[Op][0], elementAwayStacked.isNull() ? elementAway : elementAwayStacked );

    if (isDefaultTheme) {
        nickIcons[Owner][0] = elementOwner;
    } else {
        nickIcons[Owner][0] = overlayPixmaps( elementNormal, elementOwner );
    }

    nickIcons[Owner][1] = overlayPixmaps( nickIcons[Owner][0], elementAway );

    if (isDefaultTheme) {
        nickIcons[Admin][0] = elementAdmin;
    } else {
        nickIcons[Admin][0] = overlayPixmaps( elementNormal, elementAdmin );
    }

    nickIcons[Admin][1] = overlayPixmaps( nickIcons[Admin][0], elementAway );
}

QIcon Images::getLed(const QColor& col,bool state)
{
    Q_UNUSED(col)
    Q_UNUSED(state)

    return QIcon(new LedIconEngine(col, state));
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
    if (Preferences::self()->tabNotificationsSystemColor()!=m_systemColor)
    {
        if (state)
            return getLed(Preferences::self()->tabNotificationsSystemColor(),true);
        else
            return getLed(Preferences::self()->tabNotificationsSystemColor(),false);
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
    if (Preferences::self()->tabNotificationsMsgsColor()!=m_msgsColor)
    {
        if (state)
            return getLed(Preferences::self()->tabNotificationsMsgsColor(),true);
        else
            return getLed(Preferences::self()->tabNotificationsMsgsColor(),false);
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
    if (Preferences::self()->tabNotificationsPrivateColor()!=m_privateColor)
    {
        if (state)
            return getLed(Preferences::self()->tabNotificationsPrivateColor(),true);
        else
            return getLed(Preferences::self()->tabNotificationsPrivateColor(),false);
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
    if (Preferences::self()->tabNotificationsEventsColor()!=m_eventsColor)
        return getLed(Preferences::self()->tabNotificationsEventsColor(),true);
    else
        return m_eventsLedOn;
}

QIcon Images::getNickLed()
{
    if (Preferences::self()->tabNotificationsNickColor()!=m_nickColor)
        return getLed(Preferences::self()->tabNotificationsNickColor(),true);
    else
        return m_nickLedOn;
}

QIcon Images::getHighlightsLed()
{
    if (Preferences::self()->tabNotificationsHighlightsColor()!=m_highlightsColor)
        return getLed(Preferences::self()->tabNotificationsHighlightsColor(),true);
    else
        return m_highlightsLedOn;
}


