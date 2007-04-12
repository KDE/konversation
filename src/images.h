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

#include <qiconset.h>
#include <qpixmap.h>
#include <qobject.h>


/**
 * Do not create an instance of this class yourself.
 * use KonversationApplication::instance()->images().
 */

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
        virtual ~Images();

        QPixmap getCloseIcon() { return m_closeIcon; }
        QPixmap getDisabledCloseIcon() { return m_disabledCloseIcon; }

        QIconSet getLed(QColor col,bool state = true);

        QIconSet getServerLed(bool state);
        QIconSet getSystemLed(bool state);
        QIconSet getMsgsLed(bool state);
        QIconSet getPrivateLed(bool state);
        QIconSet getEventsLed();
        QIconSet getNickLed();
        QIconSet getHighlightsLed();

        QIconSet getKimproxyAway() const;
        QIconSet getKimproxyOnline() const;
        QIconSet getKimproxyOffline() const;

        QPixmap getNickIcon(NickPrivilege privilege,bool isAway=false) const;
        void initializeNickIcons();

    public slots:
        void updateIcons();

    protected:
        void initializeLeds();
        void initializeKimifaceIcons();

        QPixmap m_closeIcon;
        QPixmap m_disabledCloseIcon;

        QIconSet m_serverLedOn;
        QIconSet m_serverLedOff;
        QIconSet m_systemLedOn;
        QIconSet m_systemLedOff;
        QIconSet m_msgsLedOn;
        QIconSet m_msgsLedOff;
        QIconSet m_privateLedOn;
        QIconSet m_privateLedOff;
        QIconSet m_eventsLedOn;
        QIconSet m_nickLedOn;
        QIconSet m_highlightsLedOn;

        QColor m_serverColor;
        QColor m_systemColor;
        QColor m_msgsColor;
        QColor m_privateColor;
        QColor m_eventsColor;
        QColor m_nickColor;
        QColor m_highlightsColor;

        QIconSet kimproxyAway;
        QIconSet kimproxyOnline;
        QIconSet kimproxyOffline;

                                                  // [privilege][away]
        QPixmap nickIcons[_NickPrivilege_COUNT][2];
};
#endif
