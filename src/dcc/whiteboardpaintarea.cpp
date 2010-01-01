/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboardpaintarea.h"

#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QDebug>
#include <QPen>

namespace Konversation
{
    namespace DCC
    {
        static const int m_invalidPos = -32546;

        WhiteBoardPaintArea::WhiteBoardPaintArea(QWidget* parent)
            : QWidget(parent),
              m_mousePressed(false),
              m_lastPos(m_invalidPos, m_invalidPos),
              m_tool(WhiteBoardGlobals::Pencil),
              m_foregroundColor(Qt::black),
              m_backgroundColor(Qt::white),
              m_penWidth(1)
        {
            m_imagePixmap = new QPixmap(width(), height());
            m_imagePixmap->fill(Qt::white);
            m_overlayPixmap = new QPixmap(width(), height());
            m_overlayPixmap->fill(Qt::transparent);

            // because not all gfx driver can handle bigger images
            setMaximumSize(2048,2048);
            setCursor(Qt::CrossCursor);
        }

        void WhiteBoardPaintArea::setTool(WhiteBoardGlobals::WhiteBoardTool tool)
        {
            m_tool = tool;
        }

        void WhiteBoardPaintArea::setForegroundColor(const QColor& color)
        {
            m_foregroundColor = color;
        }

        void WhiteBoardPaintArea::setBackgroundColor(const QColor& color)
        {
            m_backgroundColor = color;
        }

        void WhiteBoardPaintArea::setPenWidth(qreal width)
        {
            m_penWidth = width;
        }

        void WhiteBoardPaintArea::paintEvent(QPaintEvent *event)
        {
            QPainter tPaint;
            tPaint.begin(this);

            foreach (const QRect& rect, event->region().rects())
            {
                tPaint.drawPixmap(rect, *m_imagePixmap, rect);

        // Looks cool but doesn't make sense for ColorLines
        // Invert overlapping color
        //        const int tBytePerLine = rect.width() * sizeof(QRgb);
        //        const int tHeight = rect.height();
        //        const int colorMax = 255;
        //        QImage tOriginal = m_imagePixmap->toImage();
        //        QImage tOverlay = m_overlayPixmap->toImage();
        //
        //        for (int i = rect.y(); i < tHeight; ++i)
        //        {
        //            QRgb* tRgb = (QRgb*)tOriginal.scanLine(i);
        //            QRgb* tOverlayLine = (QRgb*)tOverlay.scanLine(i);
        //            for (int j = rect.x()*sizeof(QRgb) ; j < tBytePerLine; j += sizeof(QRgb))
        //            {
        //                int tAlpha = qAlpha(*tOverlayLine);
        //                if (tAlpha > 0)
        //                {
        //                    *tOverlayLine = qRgb(colorMax^qRed(*tRgb),colorMax^qGreen(*tRgb),colorMax^ qBlue(*tRgb));
        //                }
        //                ++tRgb;
        //                ++tOverlayLine;
        //            }
        //        }
        //        tPaint.drawImage(rect, tOverlay, rect);
                tPaint.drawPixmap(rect, *m_overlayPixmap, rect);
            }

            tPaint.end();
        }

        void WhiteBoardPaintArea::resizeEvent(QResizeEvent *event)
        {
            setMinimumSize(event->size());
            QPixmap* oldImg = m_imagePixmap;
            m_imagePixmap = new QPixmap(event->size());
            m_imagePixmap->fill(Qt::white);
            QPainter tPaint(m_imagePixmap);
            tPaint.drawPixmap(0,0,*oldImg);
            tPaint.end();
            delete oldImg;

            delete m_overlayPixmap;
            m_overlayPixmap = new QPixmap(event->size());
            m_overlayPixmap->fill(Qt::transparent);
        }

        void WhiteBoardPaintArea::mousePressEvent(QMouseEvent *event)
        {

            if (event->buttons() & Qt::RightButton)
            {
                m_mousePressed = false;
                m_overlayPixmap->fill(Qt::transparent);
                update();
            }
            else if (event->buttons() & Qt::LeftButton)
            {
                m_mousePressed = true;
                mouseMoveEvent(event);
            }
        }

        void WhiteBoardPaintArea::mouseReleaseEvent(QMouseEvent *event)
        {
            if (event->buttons() & Qt::LeftButton)
            {
                m_mousePressed = false;
                m_lastPos.setX(m_invalidPos);
                m_lastPos.setY(m_invalidPos);
                QPainter tPainter(m_imagePixmap);
                tPainter.drawPixmap(0,0, *m_overlayPixmap);
                tPainter.end();
                m_overlayPixmap->fill(Qt::transparent);
                update();
            }
        }

        void WhiteBoardPaintArea::mouseMoveEvent(QMouseEvent *event)
        {
            if (m_mousePressed)
            {
                QPainter tPainter(m_overlayPixmap);
                tPainter.setPen(QPen(m_foregroundColor, m_penWidth));

                switch (m_tool)
                {
                case WhiteBoardGlobals::Pencil:
                    {
                        if (m_lastPos.x() > m_invalidPos && m_lastPos.y() > m_invalidPos)
                        {
                            tPainter.drawLine(m_lastPos, event->pos());
                        }
                        else
                        {
                            tPainter.drawPoint(event->pos());
                        }
                    }
                    m_lastPos = event->pos();
                    break;

                case WhiteBoardGlobals::FilledEllipse:
                    tPainter.setBrush(m_backgroundColor);
                case WhiteBoardGlobals::Ellipse:
                    {
                        if (m_lastPos.x() > m_invalidPos && m_lastPos.y() > m_invalidPos)
                        {
                            m_overlayPixmap->fill(Qt::transparent);
                            const int xStart = m_lastPos.x();
                            const int yStart = m_lastPos.y();
                            const int xTo = event->pos().x() - xStart;
                            const int yTo = event->pos().y() - yStart;
                            tPainter.drawEllipse(xStart, yStart, xTo, yTo);
                        }
                        else
                        {
                            tPainter.drawPoint(event->pos());
                            m_lastPos = event->pos();
                        }
                    }
                    break;

                case WhiteBoardGlobals::FilledRectangle:
                    tPainter.setBrush(m_backgroundColor);
                case WhiteBoardGlobals::Rectangle:
                    {
                        if (m_lastPos.x() > m_invalidPos && m_lastPos.y() > m_invalidPos)
                        {
                            m_overlayPixmap->fill(Qt::transparent);
                            const int xStart = m_lastPos.x();
                            const int yStart = m_lastPos.y();
                            const int xTo = event->pos().x() - xStart;
                            const int yTo = event->pos().y() - yStart;
                            tPainter.drawRect(xStart, yStart, xTo, yTo);
                        }
                        else
                        {
                            tPainter.drawPoint(event->pos());
                            m_lastPos = event->pos();
                        }
                    }
                    break;

                case WhiteBoardGlobals::Line:
                    {
                        if (m_lastPos.x() > m_invalidPos && m_lastPos.y() > m_invalidPos)
                        {
                            m_overlayPixmap->fill(Qt::transparent);
                            tPainter.drawLine(m_lastPos, event->pos());
                        }
                        else
                        {
                            tPainter.drawPoint(event->pos());
                            m_lastPos = event->pos();
                        }
                    }
                    break;

                case WhiteBoardGlobals::Eraser:
                    {
                        tPainter.setPen(QPen(Qt::white, m_penWidth));
                        if (m_lastPos.x() > m_invalidPos && m_lastPos.y() > m_invalidPos)
                        {
                            tPainter.drawLine(m_lastPos, event->pos());
                        }
                        else
                        {
                            tPainter.drawPoint(event->pos());
                            m_lastPos = event->pos();
                        }
                    }
                    break;
                }
                tPainter.end();

                update();
            }
        }

    }
}