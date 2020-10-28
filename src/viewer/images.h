/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005-2006 Eike Hein <hein@kde.org>
*/

#ifndef IMAGES_H
#define IMAGES_H

// Qt
#include <QIcon>
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

        QIcon getLed(const QColor& col, bool state = true) const;

        QIcon getServerLed(bool state) const;
        QIcon getSystemLed(bool state) const;
        QIcon getMsgsLed(bool state) const;
        QIcon getPrivateLed(bool state) const;
        QIcon getEventsLed() const;
        QIcon getNickLed() const;
        QIcon getHighlightsLed() const;

        QIcon getNickIcon(NickPrivilege privilege,bool isAway=false) const;
        QIcon getNickIconAwayOverlay() const;
        int getNickIconSize() const;
        void initializeNickIcons();

    private:
        void initializeLeds();

    private:
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
