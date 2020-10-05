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
        void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
        QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
        QIconEngine* clone() const override;

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


class NickIconSet
{
public:
    enum ElementIndex
    {
        Normal,
        Voice,
        HalfOp,
        Op,
        Owner,
        Admin,
        Away,
        AwayStacked,
        _ElementIndex_COUNT
    };

    bool load(const QString &baseDir);

    bool isNull() const { return m_element[Normal].isNull(); }
    const QString & path(Images::NickPrivilege privilege) const { return m_nickIconPaths[privilege]; }
    const QPixmap & element(NickIconSet::ElementIndex index) const { return m_element[index]; }

    const QString & nickIconAwayOverlayPath() const { return m_nickIconAwayOverlayPath; }

private:
    void clear();

private:
    QPixmap m_element[_ElementIndex_COUNT];
    QString m_nickIconPaths[Images::_NickPrivilege_COUNT];
    QString m_nickIconAwayOverlayPath;
};

void NickIconSet::clear()
{
    for (int i = 0; i < _ElementIndex_COUNT; ++i) {
        m_element[i] = QPixmap();
    }
    for (int i = 0; i < Images::_NickPrivilege_COUNT; ++i) {
        m_nickIconPaths[i].clear();
    }

    m_nickIconAwayOverlayPath.clear();
}

bool NickIconSet::load(const QString &baseDir)
{
    static const struct LoadData
    {
        QLatin1String elementName;
        int pathIndex; //Image::NickPrivilege: ,  -1: none, -2: nickIconAwayOverlayPath
        bool required;
    }
    loadData[_ElementIndex_COUNT] = {
        {QLatin1String("irc_normal"),       Images::Normal, true},
        {QLatin1String("irc_voice"),        Images::Voice,  true},
        {QLatin1String("irc_halfop"),       Images::HalfOp, true},
        {QLatin1String("irc_op"),           Images::Op,     true},
        {QLatin1String("irc_owner"),        Images::Owner,  true},
        {QLatin1String("irc_admin"),        Images::Admin,  true},
        {QLatin1String("irc_away"),         -2,             true},
        {QLatin1String("irc_away_stacked"), -1,             false},
    };

    for (int i = 0; i < _ElementIndex_COUNT; ++i) {
        const LoadData& d = loadData[i];
        const QString path = baseDir + QLatin1Char('/') + d.elementName + QLatin1String(".png");
        // try to load file
        if (!QFile::exists(path)) {
            if (d.required) {
                clear();
                return false;
            }
            continue;
        }
        m_element[i] = QPixmap(path);
        if (m_element[i].isNull()) {
            clear();
            return false;
        }
        // note path of file
        if (d.pathIndex == -2) {
            m_nickIconAwayOverlayPath = path;
        } else if (d.pathIndex != -1) {
            m_nickIconPaths[d.pathIndex] = path;
        }
    }

    return true;
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
    NickIconSet iconSet;

    QString iconTheme = Preferences::self()->iconTheme();
    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("konversation/themes/") + iconTheme, QStandardPaths::LocateDirectory);
    QStringList icons;
    bool isDefaultTheme = (iconTheme == QLatin1String("default"));

    for (const QString& dir : qAsConst(dirs)) {
        if (iconSet.load(dir)) {
            break;
        }
    }

    // fallback to default icon set, if not yet chosen
    if (iconSet.isNull() && !isDefaultTheme) {
        dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/themes/default"), QStandardPaths::LocateDirectory);

        for (const QString& dir : qAsConst(dirs)) {
            if (iconSet.load(dir)) {
                break;
            }
        }
        isDefaultTheme = true;
    }

    if (iconSet.isNull()) {
        return;
    }

    for (int i = 0; i < Images::_NickPrivilege_COUNT; ++i) {
        nickIconPaths[i] = iconSet.path((NickPrivilege)i);
    }
    nickIconAwayPath = iconSet.nickIconAwayOverlayPath();

    nickIcons[Normal][0] = iconSet.element(NickIconSet::Normal);
    nickIcons[Normal][1] = overlayPixmaps( nickIcons[Normal][0], iconSet.element(NickIconSet::Away) );

    nickIcons[Voice][0] = overlayPixmaps( iconSet.element(NickIconSet::Normal), iconSet.element(NickIconSet::Voice) );
    nickIcons[Voice][1] = overlayPixmaps( nickIcons[Voice][0], iconSet.element(NickIconSet::AwayStacked).isNull() ? iconSet.element(NickIconSet::Away) : iconSet.element(NickIconSet::AwayStacked) );

    nickIcons[HalfOp][0] = overlayPixmaps( iconSet.element(NickIconSet::Normal), iconSet.element(NickIconSet::HalfOp) );
    nickIcons[HalfOp][1] = overlayPixmaps( nickIcons[HalfOp][0], iconSet.element(NickIconSet::AwayStacked).isNull() ? iconSet.element(NickIconSet::Away) : iconSet.element(NickIconSet::AwayStacked) );

    nickIcons[Op][0] = overlayPixmaps( iconSet.element(NickIconSet::Normal), iconSet.element(NickIconSet::Op) );
    nickIcons[Op][1] = overlayPixmaps( nickIcons[Op][0], iconSet.element(NickIconSet::AwayStacked).isNull() ? iconSet.element(NickIconSet::Away) : iconSet.element(NickIconSet::AwayStacked) );

    if (isDefaultTheme) {
        nickIcons[Owner][0] = iconSet.element(NickIconSet::Owner);
    } else {
        nickIcons[Owner][0] = overlayPixmaps( iconSet.element(NickIconSet::Normal), iconSet.element(NickIconSet::Owner) );
    }

    nickIcons[Owner][1] = overlayPixmaps( nickIcons[Owner][0], iconSet.element(NickIconSet::Away) );

    if (isDefaultTheme) {
        nickIcons[Admin][0] = iconSet.element(NickIconSet::Admin);
    } else {
        nickIcons[Admin][0] = overlayPixmaps( iconSet.element(NickIconSet::Normal), iconSet.element(NickIconSet::Admin) );
    }

    nickIcons[Admin][1] = overlayPixmaps( nickIcons[Admin][0], iconSet.element(NickIconSet::Away) );
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


