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

#ifndef IMAGES_H
#define IMAGES_H

// Qt
#include <QIcon>
#include <QPixmap>
#include <QObject>

class NickIconSet;

/**
 * Do not create an instance of this class yourself.
 * use KonversationApplication::instance()->images().
 */

//TODO FIXME what fuck is the above statement supposed to prove? we don't know how to make a singleton?

class Images : public QObject
{
    Q_OBJECT

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
        ~Images() override;

        QIcon getLed(const QColor& col, bool state = true);

        QIcon getServerLed(bool state);
        QIcon getSystemLed(bool state);
        QIcon getMsgsLed(bool state);
        QIcon getPrivateLed(bool state);
        QIcon getEventsLed();
        QIcon getNickLed();
        QIcon getHighlightsLed();

        QIcon getNickIcon(NickPrivilege privilege,bool isAway=false) const;
        QIcon getNickIconAwayOverlay() const;
        int getNickIconSize() const;
        void initializeNickIcons();

    protected:
        void initializeLeds();

        QIcon m_serverLedOn;
        QIcon m_serverLedOff;
        QIcon m_systemLedOn;
        QIcon m_systemLedOff;
        QIcon m_msgsLedOn;
        QIcon m_msgsLedOff;
        QIcon m_privateLedOn;
        QIcon m_privateLedOff;
        QIcon m_eventsLedOn;
        QIcon m_nickLedOn;
        QIcon m_highlightsLedOn;

        QColor m_serverColor;
        QColor m_systemColor;
        QColor m_msgsColor;
        QColor m_privateColor;
        QColor m_eventsColor;
        QColor m_nickColor;
        QColor m_highlightsColor;

        QScopedPointer<NickIconSet> nickIconSet;
};
#endif
