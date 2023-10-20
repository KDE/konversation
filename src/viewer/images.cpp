/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005-2006 Eike Hein <hein@kde.org>
*/

#include "images.h"
#include "common.h"
#include "application.h"
#include "nickiconset.h"

#include <QIconEngine>
#include <QPainter>

#include <QStandardPaths>


using namespace Konversation;


class LedIconEngine : public QIconEngine
{
    public:
        LedIconEngine(const QColor &color, bool state)
            : QIconEngine(), m_color(color), m_state(state)
        {
        }
        ~LedIconEngine() override = default;

        // QIconEngine
        void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
        QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
        QIconEngine* clone() const override;

    private:
        const QColor m_color;
        const bool m_state;

        Q_DISABLE_COPY(LedIconEngine)
};

void LedIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(mode)
    Q_UNUSED(state)

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
    auto* newEngine = new LedIconEngine(m_color, m_state);

    return newEngine;
}


Images::Images()
    : nickIconSet(new NickIconSet)
{
    initializeLeds();
    initializeNickIcons();
}

Images::~Images()
{
}

int Images::getNickIconSize() const
{
    return nickIconSet->defaultIconSize();
}

QIcon Images::getNickIcon(NickPrivilege privilege,bool isAway) const
{
    return nickIconSet->nickIcon(privilege, isAway ? NickIconSet::UserAway : NickIconSet::UserPresent);
}

QIcon Images::getNickIconAwayOverlay() const
{
    return nickIconSet->nickIconAwayOverlay();
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
    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("konversation/themes/") + iconTheme, QStandardPaths::LocateDirectory);
    const bool isDefaultTheme = (iconTheme == QLatin1String("default"));

    for (const QString& dir : std::as_const(dirs)) {
        if (nickIconSet->load(dir)) {
            break;
        }
    }

    // fallback to default if theme could not be loaded
    if (nickIconSet->isNull() && !isDefaultTheme) {
        dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/themes/default"), QStandardPaths::LocateDirectory);

        for (const QString& dir : std::as_const(dirs)) {
            if (nickIconSet->load(dir)) {
                break;
            }
        }
    }
}

QIcon Images::getLed(const QColor& col,bool state) const
{
    Q_UNUSED(col)
    Q_UNUSED(state)

    return QIcon(new LedIconEngine(col, state));
}

QIcon Images::getServerLed(bool state) const
{
    if (state)
        return m_serverLedOn;
    else
        return m_serverLedOff;
}

QIcon Images::getSystemLed(bool state) const
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

QIcon Images::getMsgsLed(bool state) const
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

QIcon Images::getPrivateLed(bool state) const
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

QIcon Images::getEventsLed() const
{
    if (Preferences::self()->tabNotificationsEventsColor()!=m_eventsColor)
        return getLed(Preferences::self()->tabNotificationsEventsColor(),true);
    else
        return m_eventsLedOn;
}

QIcon Images::getNickLed() const
{
    if (Preferences::self()->tabNotificationsNickColor()!=m_nickColor)
        return getLed(Preferences::self()->tabNotificationsNickColor(),true);
    else
        return m_nickLedOn;
}

QIcon Images::getHighlightsLed() const
{
    if (Preferences::self()->tabNotificationsHighlightsColor()!=m_highlightsColor)
        return getLed(Preferences::self()->tabNotificationsHighlightsColor(),true);
    else
        return m_highlightsLedOn;
}

#include "moc_images.cpp"
