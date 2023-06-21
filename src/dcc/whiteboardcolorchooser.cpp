/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009-2010 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboardcolorchooser.h"

#include "konversation_log.h"

#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QColorDialog>
#include <qdrawutil.h>

namespace Konversation
{
    namespace DCC
    {
        WhiteBoardColorChooser::WhiteBoardColorChooser(QWidget* parent)
            : QFrame(parent),
              m_foregroundColor(Qt::black),
              m_backgroundColor(Qt::white),
              m_swapPixmap(16,16)
        {
            setFrameStyle(QFrame::NoFrame | QFrame::Plain);
            setMinimumSize(40,40);
            drawSwapPixmap();
        }

        WhiteBoardColorChooser::~WhiteBoardColorChooser()
        {
        }

        QColor WhiteBoardColorChooser::color(const ColorLayer& layer) const
        {
            switch (layer)
            {
            case BackgroundColor:
                return m_backgroundColor;
            case ForegroundColor:
                return m_foregroundColor;
            default:
                Q_ASSERT(false);
                return QColor(Qt::transparent);
            }
        }

        QColor WhiteBoardColorChooser::foregroundColor() const
        {
            return color(ForegroundColor);
        }

        QColor WhiteBoardColorChooser::backgroundColor() const
        {
            return color(BackgroundColor);
        }

        void WhiteBoardColorChooser::setColor(ColorLayer layer, const QColor& color)
        {
            switch (layer)
            {
            case ForegroundColor:
                setForegroundColor(color);
                Q_EMIT foregroundColorChanged(color);
                break;
            case BackgroundColor:
                setBackgroundColor(color);
                Q_EMIT backgroundColorChanged(color);
                break;
            default:
                Q_ASSERT(false);
                return;
            }
        }

        void WhiteBoardColorChooser::setForegroundColor (const QColor& color)
        {
            m_foregroundColor = color;
            update();
        }

        void WhiteBoardColorChooser::setBackgroundColor (const QColor& color)
        {
            m_backgroundColor = color;
            update();
        }

        void WhiteBoardColorChooser::mouseReleaseEvent(QMouseEvent *e)
        {
            // qCDebug(KONVERSATION_LOG) << "epos:"<< e->pos();
            // qCDebug(KONVERSATION_LOG) << "foregroundrect" << foregroundRect();
            // qCDebug(KONVERSATION_LOG) << "backgroundrect" << backgroundRect();
            // qCDebug(KONVERSATION_LOG) << "swap" << swapPixmapRect();

            ColorLayer whichColor = None;

            if (foregroundRect().contains(e->pos()))
            {
                whichColor = ForegroundColor;
                qCDebug(KONVERSATION_LOG) << "> in foreground";
            }
            else if (backgroundRect().contains(e->pos()))
            {
                whichColor = BackgroundColor;
                qCDebug(KONVERSATION_LOG) << "> in background";
            }
            else if (swapPixmapRect().contains(e->pos()))
            {
                qCDebug(KONVERSATION_LOG) << "> in swap";
                QColor oldFore = m_foregroundColor;
                m_foregroundColor = m_backgroundColor;
                m_backgroundColor = oldFore;
                Q_EMIT colorsSwapped(m_foregroundColor, m_backgroundColor);
                update();
                return;
            }

            if (whichColor == ForegroundColor || whichColor == BackgroundColor)
            {
                QColor col = QColorDialog::getColor(color(whichColor), this);
                if (col.isValid())
                {
                    setColor(whichColor, col);
                    update();
                }
            }
        }

        void WhiteBoardColorChooser::paintEvent(QPaintEvent *e)
        {
            QFrame::paintEvent(e);

            QPainter tPaint;
            tPaint.begin(this);

            tPaint.drawPixmap(swapPixmapRect().topLeft(), m_swapPixmap);
            QRect bgRect = backgroundRect();
            QRect bgRectInside = QRect(bgRect.x () + 2, bgRect.y () + 2,
                                       bgRect.width () - 4, bgRect.height () - 4);
            tPaint.fillRect(bgRectInside, m_backgroundColor);
            qDrawShadePanel(&tPaint, bgRect, palette(),
                            false/*not sunken*/, 2/*lineWidth*/,
                            nullptr/*never fill*/);

            QRect fgRect = foregroundRect();
            QRect fgRectInside = QRect(fgRect.x () + 2, fgRect.y () + 2,
                                       fgRect.width () - 4, fgRect.height () - 4);
            tPaint.fillRect(fgRectInside, m_foregroundColor);
            qDrawShadePanel(&tPaint, fgRect, palette (),
                            false/*not sunken*/, 2/*lineWidth*/,
                            nullptr/*never fill*/);

            tPaint.end();
        }

        void WhiteBoardColorChooser::resizeEvent(QResizeEvent *e)
        {
            Q_UNUSED(e)

            const int minWidthHeight = qMin(width(),height());
            const int swapImageSize = minWidthHeight/3;
            m_swapPixmap = QPixmap(swapImageSize, swapImageSize);
            drawSwapPixmap();
        }

        QRect WhiteBoardColorChooser::swapPixmapRect() const
        {
            return QRect(contentsRect().width() - m_swapPixmap.width(),
                         0,
                         m_swapPixmap.width(),
                         m_swapPixmap.height());
        }

        QRect WhiteBoardColorChooser::foregroundBackgroundRect() const
        {
            QRect cr(contentsRect());
            return QRect(cr.width() / 8.0f,
                         cr.height() / 8.0f,
                         cr.width() * 6.0f / 8.0f,
                         cr.height() * 6.0f / 8.0f);
        }

        QRect WhiteBoardColorChooser::foregroundRect() const
        {
            QRect fbr(foregroundBackgroundRect());
            return QRect(fbr.x(),
                         fbr.y(),
                         fbr.width() * 3.0f / 4.0f,
                         fbr.height() * 3.0f / 4.0f);
        }

        QRect WhiteBoardColorChooser::backgroundRect() const
        {
            QRect fbr(foregroundBackgroundRect());
            return QRect(fbr.x() + fbr.width() / 4.0f,
                         fbr.y() + fbr.height() / 4.0f,
                         fbr.width() * 3.0f / 4.0f,
                         fbr.height() * 3.0f / 4.0f);
        }

        void WhiteBoardColorChooser::drawSwapPixmap()
        {
            const int arrowHeight = m_swapPixmap.height() / 4.0f;
            const int arrowWidth = m_swapPixmap.width() / 2.0f;
            const int imageHeight = m_swapPixmap.height();
            const int imageWidth = m_swapPixmap.width();

            const QPointF arrowLeftPolyGon[3] = {
                 QPointF(0, arrowHeight),
                 QPointF(arrowHeight, 0),
                 QPointF(arrowHeight, arrowHeight*2)
            };
            const QPointF arrowDownPolyGon[3] = {
                 QPointF(imageWidth-arrowWidth, imageHeight-arrowHeight-1),
                 QPointF(imageWidth, imageHeight-arrowHeight-1),
                 QPointF(imageWidth-arrowHeight, imageHeight-1)
            };

            m_swapPixmap.fill(Qt::transparent);

            QPainter tPainter(&m_swapPixmap);
            tPainter.setBrush(Qt::black);
            tPainter.drawPolygon(arrowLeftPolyGon, 3);
            tPainter.drawPolygon(arrowDownPolyGon, 3);

            QPoint tCenterLine(imageWidth-arrowHeight, arrowHeight);

            tPainter.drawLine(tCenterLine,QPoint(arrowHeight, arrowHeight));
            tPainter.drawLine(tCenterLine,QPoint(imageWidth-arrowHeight, imageWidth-arrowHeight));

            tPainter.end();
        }

    }
}

#include "moc_whiteboardcolorchooser.cpp"
