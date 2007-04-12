/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  Copyright (C) 2003 Christian Muehlhaeuser <chris@chris.de>
  Copyright (C) 2004 Michael Goettsche <michael.goettsche@kdemail.net>
*/

#ifndef OSD_H
#define OSD_H

#include <qpixmap.h>                              //stack allocated
#include <qtimer.h>                               //stack allocated
#include <qwidget.h>                              //baseclass


class QFont;
class QString;
class QStringList;
class QTimer;
class MetaBundle;

class OSDWidget : public QWidget
{
    Q_OBJECT

    public:
        enum Alignment { Left, Middle, Center, Right };

        explicit OSDWidget(const QString &appName, QWidget *parent = 0, const char *name = "osd");
        void setDuration(int ms);
        void setFont(QFont newfont);
        void setShadow(bool shadow);
        void setTextColor(const QColor &newcolor);
        void setBackgroundColor(const QColor &newColor);
        void setOffset( int x, int y );
        void setAlignment(Alignment);
        void setScreen(uint screen);
        void setText(const QString &text) { m_currentText = text; refresh(); }

        void unsetColors();

        int screen()    { return m_screen; }
        int alignment() { return m_alignment; }
        int y()         { return m_y; }

    signals:
        void hidden();

    public slots:
        //TODO rename show, scrap removeOSD, just use hide() <- easier to learn
        void showOSD(const QString&, bool preemptive=false );
        void removeOSD()                          //inlined as is convenience function
        {
            hide();
        }

    protected slots:
        void minReached();

    protected:
        /* render text into osdBuffer */
        void renderOSDText(const QString &text);
        void mousePressEvent( QMouseEvent* );
        bool event(QEvent*);

        void show();

        /* call to reposition a new OSD text or when position attributes change */
        void reposition( QSize newSize = QSize() );

        /* called after most set*() calls to update the OSD */
        void refresh();

        enum KDesktopLockStatus { NotLocked=0, Locked=1, DCOPError=2 };
        static KDesktopLockStatus isKDesktopLockRunning();

        static const int MARGIN = 15;

        QString     m_appName;
        int         m_duration;
        QTimer      timer;
        QTimer      timerMin;
        QPixmap     osdBuffer;
        QStringList textBuffer;
        QString     m_currentText;
        bool        m_shadow;

        Alignment   m_alignment;
        int         m_screen;
        uint        m_y;

        bool m_dirty;                             //if dirty we will be re-rendered before we are shown
};

// do not pollute OSDWidget with this preview stuff
class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT

    public:
        explicit OSDPreviewWidget( const QString &appName, QWidget *parent = 0, const char *name = "osdpreview" );

        static QPoint m_previewOffset;

        signals:
        void positionChanged();

    protected:
        void mousePressEvent( QMouseEvent * );
        void mouseReleaseEvent( QMouseEvent * );
        void mouseMoveEvent( QMouseEvent * );

    private:
        bool   m_dragging;
        QPoint m_dragOffset;
};
#endif                                            /*OSD_H*/
