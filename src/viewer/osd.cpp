/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * osd.cpp:   Shows some text in a pretty way independent to the WM
 * begin:     Fre Sep 26 2003
 * copyright: (C) 2004 Christian Muehlhaeuser <chris@chris.de>
 *            (C) 2004-2006 Seb Ruiz <ruiz@kde.org>
 *            (C) 2004, 2005 Max Howell
 *            (C) 2005 G�bor Lehel <illissius@gmail.com>
 *            (C) 2008 Mark Kretschmann <kretschmann@kde.org>
 */

#include "osd.h"
#include "common.h"

#include <KApplication>
#include <KIcon>
#include <KDebug>
#include <klocalizedstring.h>

#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QRegExp>
#include <QTimer>

namespace ShadowEngine
{
    QImage makeShadow( const QPixmap &textPixmap, const QColor &bgColor );
}

OSDWidget::OSDWidget(const QString &appName, QWidget *parent, const char *name )
    : QWidget( parent )
    , m_appName( appName )
    , m_duration( 5000 )
    , m_timer( new QTimer( this ) )
    , m_timerMin( new QTimer( this ) )
    , m_alignment( Middle )
    , m_screen( 0 )
    , m_y( MARGIN )
    , m_drawShadow( true )
{
    Qt::WindowFlags flags;
    flags = Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint;
    // The best of both worlds.  On Windows, setting the widget as a popup avoids a task manager entry.  On linux, a popup steals focus.
    // Therefore we go need to do it platform specific :(
    #ifdef Q_OS_WIN
    flags |= Qt::Popup;
    #else
    flags |= Qt::Window | Qt::X11BypassWindowManagerHint;
    #endif
    setWindowFlags( flags );
    setObjectName( name );
    setFocusPolicy( Qt::NoFocus );
    unsetColors();

    m_timer->setSingleShot( true );

    connect( m_timer,     SIGNAL( timeout() ), SLOT( hide() ) );
    connect( m_timerMin,  SIGNAL( timeout() ), SLOT( minReached() ) );

    //or crashes, KWindowSystem bug I think, crashes in QWidget::icon()
    kapp->setTopWidget( this );
}

OSDWidget::~OSDWidget()
{
    delete m_timer;
    delete m_timerMin;
}

void OSDWidget::show( const QString &text, bool preemptive )
{
    if ( preemptive || !m_timerMin->isActive() )
    {
        m_currentText = Konversation::removeIrcMarkup(text);

        show();
    }
    else textBuffer.append( Konversation::removeIrcMarkup(text) );      //queue
}

void OSDWidget::show() //virtual
{
    if ( !isEnabled() || m_currentText.isEmpty() )
        return;

    const uint M = fontMetrics().width( 'x' );

    const QRect oldGeometry = QRect( pos(), size() );
    const QRect newGeometry = determineMetrics( M );

    if( newGeometry.width() > 0 && newGeometry.height() > 0 )
    {
        m_m = M;
        m_size = newGeometry.size();
        setGeometry( newGeometry );
        QWidget::show();

        if ( m_duration )                             //duration 0 -> stay forever
        {
            m_timer->start( m_duration );          //calls hide()
            m_timerMin->start( 1500 );                    //calls minReached()
        }
    }
    else
        kWarning() << "Attempted to make an invalid sized OSD\n";

    update();
}

void OSDWidget::minReached()                      //SLOT
{
    if ( !textBuffer.isEmpty() )
    {
        show( textBuffer.first(), true );
        textBuffer.removeFirst();

        if( m_duration )
            //timerMin is still running
            m_timer->start( m_duration );
    }
    else m_timerMin->stop();
}

void OSDWidget::setDuration( int ms )
{
    m_duration = ms;

    if( !m_duration ) m_timer->stop();
}

QRect OSDWidget::determineMetrics( const int M )
{
    // determine a sensible maximum size, don't cover the whole desktop or cross the screen
    const QSize margin( ( M + MARGIN ) * 2, ( M + MARGIN ) * 2 ); //margins
    const QSize max = QApplication::desktop()->screen( m_screen )->size() - margin;

    // If we don't do that, the boundingRect() might not be suitable for drawText() (Qt issue N67674)
    m_currentText.replace( QRegExp( " +\n" ), "\n" );
    // remove consecutive line breaks
    m_currentText.replace( QRegExp( "\n+" ), "\n" );

    QFont titleFont = font();
    titleFont.setBold(true);
    QFontMetrics titleFm( titleFont );

    // The osd cannot be larger than the screen
    QRect titleRect = titleFm.boundingRect( 0, 0, max.width() - M, titleFm.height(), Qt::AlignLeft, m_appName );
    QRect textRect = fontMetrics().boundingRect( 0, 0, max.width(), max.height(), Qt::AlignCenter | Qt::TextWordWrap, m_currentText );
    textRect.setHeight( textRect.height() + M + M );
    
    if ( textRect.width() < titleRect.width() )
        textRect.setWidth( titleRect.width() );

    textRect.adjust( 0, 0, M*2, titleRect.height() + M );

    // expand in all directions by M
    textRect.adjust( -M, -M, M, M );

    const QSize newSize = textRect.size();
    const QRect screen = QApplication::desktop()->screenGeometry( m_screen );
    QPoint newPos( MARGIN, m_y );

    switch( m_alignment )
    {
        case Left:
            break;

        case Right:
            newPos.rx() = screen.width() - MARGIN - newSize.width();
            break;

        case Center:
            newPos.ry() = ( screen.height() - newSize.height() ) / 2;

            //FALL THROUGH

        case Middle:
            newPos.rx() = ( screen.width() - newSize.width() ) / 2;
            break;
    }

    //ensure we don't dip below the screen
    if ( newPos.y() + newSize.height() > screen.height() - MARGIN )
        newPos.ry() = screen.height() - MARGIN - newSize.height();

    // correct for screen position
    newPos += screen.topLeft();

    return QRect( newPos, textRect.size() );
}

void OSDWidget::paintEvent( QPaintEvent *e )
{
    int M = m_m;
    QSize size = m_size;

    QFont titleFont = font();
    titleFont.setBold(true);

    QPoint point;
    QRect rect( point, size );
    rect.adjust( 0, 0, -1, -1 );

    QColor shadowColor;
    {
        int h, s, v;
        palette().color( QPalette::Normal, QPalette::Foreground ).getHsv( &h, &s, &v );
        shadowColor = v > 128 ? Qt::black : Qt::white;
    }

    int align = Qt::AlignCenter | Qt::TextWordWrap;

    QPainter p( this );
    QBitmap mask( e->rect().size() );
    QPainter maskPainter( &mask );

    p.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing );
    p.setClipRect( e->rect() );

    const qreal xround = 20.0;
    const qreal yround = 20.0;

    // Masking for transparency
    mask.fill( Qt::color0 );
    maskPainter.setBrush( Qt::color1 );
    maskPainter.drawRoundedRect( e->rect(), xround, yround );
    setMask( mask );

    p.drawRoundedRect( e->rect(), xround, yround );

    p.setPen( Qt::white ); // Revert this when the background can be colorized again.
    rect.adjust( M, M, -M, -M );

    int graphicsHeight = 0;

    rect.setBottom( rect.bottom() - graphicsHeight );

    // Draw "shadow" text effect (black outline)
    if( m_drawShadow )
    {
        QPixmap pixmap( rect.size() + QSize( 10, 10 ) );
        pixmap.fill( Qt::black );

        QPainter p2( &pixmap );
        p2.setFont( font() );
        p2.setPen( Qt::white );
        p2.setBrush( Qt::white );
        p2.drawText( QRect( QPoint( 5, 5 ), rect.size() ), align, m_currentText );
        p2.end();

        p.drawImage( rect.topLeft() - QPoint( 5, 5 ), ShadowEngine::makeShadow( pixmap, shadowColor ) );
        
        pixmap.fill( Qt::black );
        pixmap.scaled( QSize(e->rect().width()-1, e->rect().height()-1) + QSize( 10, 10 ) );

        p2.begin( &pixmap );
        p2.setFont( titleFont );
        p2.setPen( Qt::white );
        p2.setBrush( Qt::white );
        p2.drawText( QRect( QPoint( 5, 5 ), QSize(e->rect().width()-1, e->rect().height()-1) ), Qt::AlignLeft, m_appName );
        p2.end();

        p.drawImage( QPoint(M*2, M/2) - QPoint( 5, 5 ), ShadowEngine::makeShadow( pixmap, shadowColor ) );
    }
    p.setPen( palette().color( QPalette::Active, QPalette::WindowText ) );
    //p.setPen( Qt::white ); // This too.
    p.setFont( font() );
    p.drawText( rect, align, m_currentText );

    p.setFont( titleFont );
    p.drawText( M * 2, (M/2), e->rect().width()-1, e->rect().height()-1, Qt::AlignLeft, m_appName );
}

void OSDWidget::resizeEvent(QResizeEvent *e)
{
    //setMask(m_background->mask());
    QWidget::resizeEvent( e );
}

bool OSDWidget::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::ApplicationPaletteChange:
        //if( !AmarokConfig::osdUseCustomColors() )
        //    unsetColors(); //use new palette's colours
        return true;

    default:
        return QWidget::event( e );
    }
}

void OSDWidget::mousePressEvent( QMouseEvent* )
{
    hide();
}

void OSDWidget::unsetColors()
{
    QPalette p = QApplication::palette();
    QPalette newPal = palette();

    newPal.setColor( QPalette::Active, QPalette::WindowText, p.color( QPalette::Active, QPalette::WindowText ) );
    newPal.setColor( QPalette::Active, QPalette::Window    , p.color( QPalette::Active, QPalette::Window ) );
    setPalette( newPal );
}

void OSDWidget::setOffset( int /*x*/, int y )
{
    //m_offset = QPoint( x, y );
    m_y = y;
}

void OSDWidget::setScreen( int screen )
{
    const int n = QApplication::desktop()->numScreens();
    m_screen = ( screen >= n ) ? n - 1 : screen;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Class OSDPreviewWidget
/////////////////////////////////////////////////////////////////////////////////////////

OSDPreviewWidget::OSDPreviewWidget( const QString &appName, QWidget *parent, const char *name )
        : OSDWidget( appName, parent, name )
        , m_dragging( false )
{
    setObjectName( "osdpreview" );
    m_currentText = i18n( "OSD Preview - drag to reposition" );
    m_duration = 0;
    //m_alignment = static_cast<Alignment>( AmarokConfig::osdAlignment() );
    //m_y = AmarokConfig::osdYOffset();
    QFont f = font();
    f.setPointSize( 16 );
    setFont( f );
    //setTranslucent( AmarokConfig::osdUseTranslucency() );
    show( m_currentText );
}

void OSDPreviewWidget::mousePressEvent( QMouseEvent *event )
{
    m_dragOffset = event->pos();

    if( event->button() == Qt::LeftButton && !m_dragging )
    {
        grabMouse( Qt::SizeAllCursor );
        m_dragging = true;
    }
}

void OSDPreviewWidget::mouseReleaseEvent( QMouseEvent * /*event*/ )
{
    if( m_dragging )
    {
        m_dragging = false;
        releaseMouse();

        // compute current Position && offset
        QDesktopWidget *desktop = QApplication::desktop();
        int currentScreen = desktop->screenNumber( pos() );

        if( currentScreen != -1 )
        {
            // set new data
            m_screen = currentScreen;
            m_y      = QWidget::y();

            emit positionChanged();
        }
    }
}

void OSDPreviewWidget::mouseMoveEvent( QMouseEvent *e )
{
    if( m_dragging && this == mouseGrabber() )
    {
        // Here we implement a "snap-to-grid" like positioning system for the preview widget

        const QRect screen      = QApplication::desktop()->screenGeometry( m_screen );
        const uint  hcenter     = screen.width() / 2;
        const uint  eGlobalPosX = e->globalPos().x() - screen.left();
        const uint  snapZone    = screen.width() / 24;

        QPoint destination = e->globalPos() - m_dragOffset - screen.topLeft();
        int maxY = screen.height() - height() - MARGIN;
        if( destination.y() < MARGIN )
            destination.ry() = MARGIN;
        if( destination.y() > maxY )
            destination.ry() = maxY;

        if( eGlobalPosX < ( hcenter - snapZone ) )
        {
            m_alignment = Left;
            destination.rx() = MARGIN;
        }
        else if( eGlobalPosX > ( hcenter + snapZone ) )
        {
            m_alignment = Right;
            destination.rx() = screen.width() - MARGIN - width();
        }
        else {
            const uint eGlobalPosY = e->globalPos().y() - screen.top();
            const uint vcenter     = screen.height() / 2;

            destination.rx() = hcenter - width() / 2;

            if( eGlobalPosY >= ( vcenter - snapZone ) && eGlobalPosY <= ( vcenter + snapZone ) )
            {
                m_alignment = Center;
                destination.ry() = vcenter - height() / 2;
            }
            else m_alignment = Middle;
        }

        destination += screen.topLeft();

        move( destination );
    }
}

/* Code copied from kshadowengine.cpp
 *
 * Copyright (C) 2003 Laur Ivan <laurivan@eircom.net>
 *
 * Many thanks to:
 *  - Bernardo Hung <deciare@gta.igs.net> for the enhanced shadow
 *    algorithm (currently used)
 *  - Tim Jansen <tim@tjansen.de> for the API updates and fixes.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

namespace ShadowEngine
{
    // Not sure, doesn't work above 10
    static const int    MULTIPLICATION_FACTOR = 3;
    // Multiplication factor for pixels directly above, under, or next to the text
    static const double AXIS_FACTOR = 2.0;
    // Multiplication factor for pixels diagonal to the text
    static const double DIAGONAL_FACTOR = 0.1;
    // Self explanatory
    static const int    MAX_OPACITY = 200;

    double decay( QImage&, int, int );

    QImage makeShadow( const QPixmap& textPixmap, const QColor &bgColor )
    {
        const int w   = textPixmap.width();
        const int h   = textPixmap.height();
        const int bgr = bgColor.red();
        const int bgg = bgColor.green();
        const int bgb = bgColor.blue();

        int alphaShadow;

        // This is the source pixmap
        QImage img = textPixmap.toImage();

        QImage result( w, h, QImage::Format_ARGB32 );
        result.fill( 0 ); // fill with black

        static const int M = 5;
        for( int i = M; i < w - M; i++) {
            for( int j = M; j < h - M; j++ )
            {
                alphaShadow = (int) decay( img, i, j );

                result.setPixel( i,j, qRgba( bgr, bgg , bgb, qMin( MAX_OPACITY, alphaShadow ) ) );
            }
        }

        return result;
    }

    double decay( QImage& source, int i, int j )
    {
        //if ((i < 1) || (j < 1) || (i > source.width() - 2) || (j > source.height() - 2))
        //    return 0;

        double alphaShadow;
        alphaShadow =(qGray(source.pixel(i-1,j-1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i-1,j  )) * AXIS_FACTOR +
                qGray(source.pixel(i-1,j+1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i  ,j-1)) * AXIS_FACTOR +
                0                         +
                qGray(source.pixel(i  ,j+1)) * AXIS_FACTOR +
                qGray(source.pixel(i+1,j-1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i+1,j  )) * AXIS_FACTOR +
                qGray(source.pixel(i+1,j+1)) * DIAGONAL_FACTOR) / MULTIPLICATION_FACTOR;

        return alphaShadow;
    }
}

#include "Osd.moc"
