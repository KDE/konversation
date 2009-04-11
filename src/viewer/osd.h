/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  osd.h   -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser <chris@chris.de>
  copyright: (C) 2008 by Mark Kretschmann <kretschmann@kde.org>
*/

#ifndef OSD_H
#define OSD_H

#include <QHash>
#include <QString>
#include <QWidget> //baseclass

#define OSD_WINDOW_OPACITY 0.8

class OSDWidget : public QWidget
{
    Q_OBJECT

    public:
        enum Alignment { Left, Middle, Center, Right };

        explicit OSDWidget(const QString &appName, QWidget *parent = 0, const char *name = "osd" );
        virtual ~OSDWidget();

        /** resets the colours to defaults */
        void unsetColors();

        void setShadow( bool shadow ) { m_drawShadow = shadow; }
        void setOffset( int x, int y );

    public slots:
        /** calls setText() then show()  */
        void show( const QString &text, bool preemptive=false );

        /** reimplemented, shows the OSD */
        virtual void show();

        /**
                    * For the sake of simplicity, when these settings are
                    * changed they do not take effect until the next time
                    * the OSD is shown!
                    *
                    * To force an update call show();
                    */
        void setDuration( int ms );
        void setTextColor( const QColor &color )
        {
            QPalette palette = this->palette();
            palette.setColor( QPalette::Active, QPalette::WindowText, color );
            setPalette(palette);
        }
        void setOffset( int y ) { m_y = y; }
        void setAlignment( Alignment alignment ) { m_alignment = alignment; }
        void setScreen( int screen );
        void setText( const QString &text ) { m_currentText = text; }
        void setTranslucent( bool enabled ) { setWindowOpacity( enabled ? OSD_WINDOW_OPACITY : 1.0 ); }

    signals:
        void hidden();

    protected slots:
        void minReached();

    protected:

        /** determine new size and position */
        QRect determineMetrics( const int M );

        // Reimplemented from QWidget
        virtual void paintEvent( QPaintEvent* );
        virtual void mousePressEvent( QMouseEvent* );
        virtual void hideEvent( QHideEvent * );
        void resizeEvent( QResizeEvent *e );
        virtual bool event( QEvent* );

        /** distance from screen edge */
        static const int MARGIN = 15;

        QString     m_appName;
        uint        m_m;
        QSize       m_size;
        int         m_duration;
        QTimer      *m_timer;
        QTimer      *m_timerMin;
        QStringList textBuffer;
        Alignment   m_alignment;
        int         m_screen;
        uint        m_y;
        bool        m_drawShadow;
        QString     m_currentText;
};


class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT

public:
    explicit OSDPreviewWidget( const QString &appName, QWidget *parent = 0, const char *name = "osdpreview" );

    int screen() const     { return m_screen; }
    int alignment() const { return m_alignment; }
    int y() const        { return m_y; }

public slots:
    void setTextColor( const QColor &color ) { OSDWidget::setTextColor( color ); doUpdate(); }
    void setFont( const QFont &font ) { OSDWidget::setFont( font ); doUpdate(); }
    void setScreen( int screen ) { OSDWidget::setScreen( screen ); doUpdate(); }
    void setUseCustomColors( const bool use, const QColor &fg )
    {
        if( use ) OSDWidget::setTextColor( fg );
        else      unsetColors();
        doUpdate();
    }
    void setTranslucent( bool enabled ) { setWindowOpacity( enabled ? OSD_WINDOW_OPACITY : 1.0 ); doUpdate(); }

private:
    inline void doUpdate() { if( !isHidden() ) show(); }

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

#endif /* OSD_H*/
