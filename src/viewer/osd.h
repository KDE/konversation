/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Christian Muehlhaeuser <chris@chris.de>
    SPDX-FileCopyrightText: 2008 Mark Kretschmann <kretschmann@kde.org>
*/

#ifndef OSD_H
#define OSD_H

#include <QHash>
#include <QString>
#include <QWidget> //baseclass

constexpr qreal OSD_WINDOW_OPACITY = 0.8;

class OSDWidget : public QWidget
{
    Q_OBJECT

    public:
        enum Alignment { Left, Middle, Center, Right };

        explicit OSDWidget(const QString &appName, QWidget *parent = nullptr,
                           const QString &name = QStringLiteral("osd"));
        ~OSDWidget() override;

        /** resets the colours to defaults */
        void unsetColors();

        void setShadow( bool shadow ) { m_drawShadow = shadow; }
        void setOffset( int x, int y );

    public Q_SLOTS:
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

    protected:
        // Reimplemented from QWidget
        void paintEvent( QPaintEvent* ) override;
        void mousePressEvent( QMouseEvent* ) override;
        void resizeEvent( QResizeEvent *e ) override;
        bool event( QEvent* ) override;

    private Q_SLOTS:
        void minReached();

    private:
        /** determine new size and position */
        QRect determineMetrics( const int M );

    protected: // accessed by OSDPreviewWidget
        /** distance from screen edge */
        static const int MARGIN = 15;

        int         m_duration;
        Alignment   m_alignment;
        int         m_screen;
        int         m_y;
        QString     m_currentText;

    private:
        QString     m_appName;
        int         m_m;
        QSize       m_size;
        QTimer      *m_timer;
        QTimer      *m_timerMin;
        QStringList textBuffer;
        bool        m_drawShadow;

        Q_DISABLE_COPY(OSDWidget)
};


class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT

public:
    explicit OSDPreviewWidget(const QString &appName, QWidget *parent = nullptr);
    ~OSDPreviewWidget() override = default;

    int screen() const     { return m_screen; }
    int alignment() const { return m_alignment; }
    int y() const        { return m_y; }

public Q_SLOTS:
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

Q_SIGNALS:
    void positionChanged();

protected:
    void mousePressEvent( QMouseEvent * ) override;
    void mouseReleaseEvent( QMouseEvent * ) override;
    void mouseMoveEvent( QMouseEvent * ) override;

private:
    bool   m_dragging;
    QPoint m_dragOffset;

    Q_DISABLE_COPY(OSDPreviewWidget)
};

#endif /* OSD_H*/
