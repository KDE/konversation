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

//is this still needed?
#ifdef Q_CC_MSVC
#    define _USE_MATH_DEFINES
#endif

#include <cmath>

#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QPen>
#include <QStack>

#include <kdebug.h>

namespace Konversation
{
    namespace DCC
    {
        static const int InvalidLastPos = -32546;

        WhiteBoardPaintArea::WhiteBoardPaintArea(QWidget* parent)
            : QWidget(parent),
              m_mousePressed(false),
              m_lastPos(InvalidLastPos, InvalidLastPos),
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
            setMaximumSize(WhiteBoardGlobals::MaxImageSize, WhiteBoardGlobals::MaxImageSize);
            setMinimumSize(100,100);
            setCursor(Qt::CrossCursor);
        }

        void WhiteBoardPaintArea::setTool(WhiteBoardGlobals::WhiteBoardTool tool)
        {
            //kDebug() << "newtool" << tool;
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

        void WhiteBoardPaintArea::swapColors(const QColor& newForeground, const QColor& newBackground)
        {
            m_foregroundColor = newForeground;
            m_backgroundColor = newBackground;
        }

        void WhiteBoardPaintArea::setPenWidth(int width)
        {
            m_penWidth = width;
        }

        void WhiteBoardPaintArea::clear()
        {
            m_imagePixmap->fill(Qt::white);
            m_overlayPixmap->fill(Qt::transparent);
            m_mousePressed = false;
            makeLastPosInvalid();
            update();
        }

        void WhiteBoardPaintArea::drawLine(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                           int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, m_tool));
            tPaint.setBrush(brushColor);
            tPaint.drawLine(xFrom, yFrom, xTo, yTo);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawRectangle(int lineWidth, const QColor& penColor,
                                                int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, m_tool));
            tPaint.drawRect(xFrom, yFrom, xTo-xFrom, yTo-yFrom);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawFilledRectangle(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                                      int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, m_tool));
            tPaint.setBrush(brushColor);
            tPaint.drawRect(xFrom, yFrom, xTo-xFrom, yTo-yFrom);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawEllipse(int lineWidth, const QColor& penColor,
                                              int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, m_tool));
            tPaint.drawEllipse(xFrom, yFrom, xTo-xFrom, yTo-yFrom);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawFilledEllipse(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                                    int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, m_tool));
            tPaint.setBrush(brushColor);
            tPaint.drawEllipse(xFrom, yFrom, xTo-xFrom, yTo-yFrom);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawArrow(int lineWidth, const QColor& penColor, int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, m_tool));
            arrow(&tPaint, xFrom, yFrom, xTo, yTo);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::useEraser(int lineWidth, int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(Qt::white, lineWidth, m_tool));
            tPaint.drawLine(xFrom, yFrom, xTo, yTo);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::useFloodFill (int x, int y, const QColor& color)
        {
            checkImageSize(x, y, 0, 0, 1);
            floodfill(x, y, color);
            update();
        }

        void WhiteBoardPaintArea::useBlt(int x1src, int y1src, int x2src, int y2src, int xdest, int ydest)
        {
            //TODO optimize me
            checkImageSize(x2src, y2src, xdest, ydest, 1);
            QPixmap copyPix = m_imagePixmap->copy(x1src, y1src, x2src-x1src, y2src-y1src);
            QPainter tPaint(m_imagePixmap);
            tPaint.drawPixmap(xdest, ydest, copyPix);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::save(const QString& fileName)
        {
            m_imagePixmap->save(fileName);
        }

        void WhiteBoardPaintArea::paintEvent(QPaintEvent *event)
        {
            //kDebug();
            QPainter tPaint;
            tPaint.begin(this);

            foreach (const QRect& rect, event->region().rects())
            {
                tPaint.drawPixmap(rect, *m_imagePixmap, rect);

        // Looks cool but doesn't make sense for ColoredLines
        // Invert overlapping color
        //        const int tBytePerLine = rect.width() * sizeof(QRgb);
        //        const int tHeight = rect.height();
        //        static const int colorMax = 255;
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
            kDebug() << "newsize:" << event->size();
            kDebug() << "newcontent:" << contentsRect().width() << " " << contentsRect().height();
            resizeImage(contentsRect().width(), contentsRect().height());

            kDebug() << "setminsize";
            const int maxWidth = qMin(qMax(event->size().width(), minimumSize().width()), WhiteBoardGlobals::MaxImageSize);
            const int maxHeight = qMin(qMax(event->size().height(), minimumSize().height()), WhiteBoardGlobals::MaxImageSize);
            setMinimumSize(maxWidth, maxHeight);
        }

        void WhiteBoardPaintArea::mousePressEvent(QMouseEvent *event)
        {
            // pencil is unabled to cancel as its always directly send
            if (event->buttons() & Qt::RightButton && m_tool != WhiteBoardGlobals::Pencil)
            {
                m_mousePressed = false;
                m_overlayPixmap->fill(Qt::transparent);
                makeLastPosInvalid();
                update();
            }
            else if (event->buttons() & Qt::LeftButton)
            {
                makeLastPosInvalid();
                m_mousePressed = true;
                if (m_tool == WhiteBoardGlobals::FloodFill)
                {
                    QCursor oldCur = cursor();
                    setCursor(Qt::WaitCursor);
                    floodfill(event->pos().x(), event->pos().y(), m_foregroundColor);
                    setCursor(oldCur);
                    emit usedFloodFill(event->pos().x(), event->pos().y(), m_foregroundColor);
                }
                else
                {
                    mouseMoveEvent(event);
                }
            }
        }

        void WhiteBoardPaintArea::mouseReleaseEvent(QMouseEvent *event)
        {
            if (event->button() == Qt::LeftButton)
            {
                m_mousePressed = false;
                switch (m_tool)
                {
                case WhiteBoardGlobals::Line:
                    emit drawedLine(m_penWidth, m_foregroundColor, m_backgroundColor,
                                    m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;

                case WhiteBoardGlobals::Pencil:
                case WhiteBoardGlobals::Eraser:
                case WhiteBoardGlobals::FloodFill:
                case WhiteBoardGlobals::Stamp:
                    break;

                case WhiteBoardGlobals::Rectangle:
                    emit drawedRectangle(m_penWidth, m_foregroundColor,
                                         m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::FilledRectangle:
                    emit drawedFilledRectangle(m_penWidth, m_foregroundColor, m_backgroundColor,
                                               m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::Ellipse:
                    emit drawedEllipse(m_penWidth, m_foregroundColor,
                                       m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::FilledEllipse:
                    emit drawedFilledEllipse(m_penWidth, m_foregroundColor, m_backgroundColor,
                                             m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::Arrow:
                    emit drawedArrow(m_penWidth, m_foregroundColor,
                                     m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                }

                QPainter tPainter(m_imagePixmap);
                tPainter.drawPixmap(0,0, *m_overlayPixmap);
                tPainter.end();
                m_overlayPixmap->fill(Qt::transparent);
                makeLastPosInvalid();
                update();
            }
        }

        void WhiteBoardPaintArea::mouseMoveEvent(QMouseEvent *event)
        {
            if (m_mousePressed)
            {
                if (isLastPosValid())
                {
                    checkImageSize(event->pos().x(), event->pos().y(), m_lastPos.x(), m_lastPos.y(), m_penWidth);
                }
                else
                {
                    checkImageSize(event->pos().x(), event->pos().y(), 0, 0, m_penWidth);
                }

                QPainter tPainter(m_overlayPixmap);
                tPainter.setPen(getPen(m_foregroundColor, m_penWidth, m_tool));

                switch (m_tool)
                {
                case WhiteBoardGlobals::Pencil:
                    {
                        if (isLastPosValid())
                        {
                            tPainter.drawLine(m_lastPos, event->pos());
                            emit drawedPencil(m_penWidth, m_foregroundColor, m_backgroundColor,
                                              m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                        }
                        else
                        {
                            tPainter.drawPoint(event->pos());
                            emit drawedLine(m_penWidth, m_foregroundColor, m_backgroundColor,
                                            event->pos().x(), event->pos().y(), event->pos().x(), event->pos().y());
                        }
                    }
                    m_lastPos = event->pos();
                    break;

                case WhiteBoardGlobals::FilledEllipse:
                    tPainter.setBrush(m_backgroundColor);
                case WhiteBoardGlobals::Ellipse:
                    {
                        if (isLastPosValid())
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
                        if (isLastPosValid())
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
                        if (isLastPosValid())
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
                        tPainter.setPen(getPen(Qt::white, m_penWidth, m_tool));
                        if (isLastPosValid())
                        {
                            tPainter.drawLine(m_lastPos, event->pos());
                            emit usedEraser(m_penWidth, m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                        }
                        else
                        {
                            kDebug() << "draw eraser point";
                            tPainter.drawPoint(event->pos());
                            emit usedEraser(m_penWidth, event->pos().x(), event->pos().y(), event->pos().x(), event->pos().y());
                        }
                    }
                    m_lastPos = event->pos();
                    break;

                case WhiteBoardGlobals::FloodFill:
                    //handle in mousePressEvent, otherwise the would always trigger a floodfill on move
                    break;

                case WhiteBoardGlobals::Arrow:
                    {
                        if (isLastPosValid())
                        {
                            m_overlayPixmap->fill(Qt::transparent);
                            arrow(&tPainter, m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                        }
                        else
                        {
                            m_lastPos = event->pos();
                        }
                    }
                    break;

                }
                tPainter.end();

                update();
            }
        }

        void WhiteBoardPaintArea::makeLastPosInvalid()
        {
            m_lastPos.setX(InvalidLastPos);
            m_lastPos.setY(InvalidLastPos);
        }

        bool WhiteBoardPaintArea::isLastPosValid()
        {
            if (m_lastPos.x() > InvalidLastPos && m_lastPos.y() > InvalidLastPos)
            {
                return true;
            }
            return false;
        }

        void WhiteBoardPaintArea::checkImageSize(int x1, int y1, int x2, int y2, int penWidth)
        {
            kDebug();
            if (width() == WhiteBoardGlobals::MaxImageSize && height() == WhiteBoardGlobals::MaxImageSize)
            {
                return;
            }

            const int maxX = qMax(qMin(qMax(x1+penWidth,x2+penWidth), WhiteBoardGlobals::MaxImageSize), width());
            const int maxY = qMax(qMin(qMax(y1+penWidth,y2+penWidth), WhiteBoardGlobals::MaxImageSize), height());

            if (maxX > width() || maxY > height())
            {
                // if the image is not visible we receive no resizeEvent
                resizeImage(maxX, maxY);
                resize(maxX, maxY);
            }
        }

        void WhiteBoardPaintArea::resizeImage(int width, int height)
        {
            if (m_imagePixmap->height() < height || m_imagePixmap->width() < width)
            {
                QPixmap* oldImg = m_imagePixmap;
                m_imagePixmap = new QPixmap(width, height);
                m_imagePixmap->fill(Qt::white);
                QPainter tPaint(m_imagePixmap);
                tPaint.drawPixmap(0,0,*oldImg);
                tPaint.end();
                delete oldImg;

                QPixmap* oldOverlay = m_overlayPixmap;
                m_overlayPixmap = new QPixmap(width, height);
                m_overlayPixmap->fill(Qt::transparent);
                QPainter tPaintOverlay(m_overlayPixmap);
                tPaintOverlay.drawPixmap(0,0,*oldOverlay);
                tPaintOverlay.end();
                delete oldOverlay;
            }
        }

        QPen WhiteBoardPaintArea::getPen(const QColor& color, int lineWidth, WhiteBoardGlobals::WhiteBoardTool tool)
        {
            switch (tool)
            {
                case WhiteBoardGlobals::Pencil:
                case WhiteBoardGlobals::Line:
                case WhiteBoardGlobals::Eraser:
                case WhiteBoardGlobals::Arrow:
                    return QPen(color, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

                default:
                    return QPen(color, lineWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            }
        }

        struct StackCoords
        {
            int x;
            int y;
        };

        void WhiteBoardPaintArea::floodfill(int x, int y, const QColor& fillColor)
        {
            // A recursiv "fast" floodfill *can* cause a stackoverflow
            // so we use a "slow" stack + extra data.
            // TODO optimize me, mass QImage::pixel use is slow
            QImage image = m_imagePixmap->toImage();
            const int width = image.width();
            const int height = image.height();
            if (x < 0 || y < 0 || x >= width || y >= height)
            {
                return;
            }
            QRgb originalColor = image.pixel(x,y);
            QRgb color = fillColor.rgb();

            if (color == originalColor)
            {
                //already filled
                return;
            }

            QStack<StackCoords> tStack;
            StackCoords t;
            t.x = x;
            t.y = y;

            tStack.push(t);
            while (!tStack.isEmpty())
            {
                StackCoords co = tStack.pop();
                image.setPixel(co.x, co.y, color);

                StackCoords tLeft;
                tLeft.x = co.x-1;
                tLeft.y = co.y;
                if (tLeft.x >= 0 && image.pixel(tLeft.x, tLeft.y) == originalColor)
                {
                    tStack.push(tLeft);
                }

                StackCoords tRight;
                tRight.x = co.x+1;
                tRight.y = co.y;
                if (tRight.x < width && image.pixel(tRight.x, tRight.y) == originalColor)
                {
                    tStack.push(tRight);
                }

                StackCoords tUp;
                tUp.x = co.x;
                tUp.y = co.y-1;
                if (tUp.y >= 0 && image.pixel(tUp.x, tUp.y) == originalColor)
                {
                    tStack.push(tUp);
                }

                StackCoords tDown;
                tDown.x = co.x;
                tDown.y = co.y+1;
                if (tDown.y < height && image.pixel(tDown.x, tDown.y) == originalColor)
                {
                    tStack.push(tDown);
                }
            }
            delete m_imagePixmap;
            m_imagePixmap = new QPixmap(QPixmap::fromImage(image));
        }

        void WhiteBoardPaintArea::arrow(QPainter* painter, int x1, int y1, int x2, int y2)
        {
            //eh? this got somehow out of control
            //TODO simplify me
            const int yDiff = y1 - y2;
            const int xDiff = x1 - x2;
            qreal angle;
            if (xDiff == 0 && yDiff == 0)
            {
                return;
            }
            else if (xDiff == 0 && yDiff > 0)
            {
                angle = M_PI;
            }
            else if (xDiff > 0 && yDiff == 0)
            {
                angle = 2*M_PI/4*3;
            }
            else if (xDiff == 0 && yDiff < 0)
            {
                angle = 0;
            }
            else if (xDiff < 0 && yDiff == 0)
            {
                angle = M_PI/2;
            }
            else
            {
                angle = atan(qreal(yDiff)/qreal(xDiff));
            }

            if (xDiff > 0.0 && yDiff > 0.0)
            {
                angle = (M_PI/2) - angle + M_PI;
            }
            else if (xDiff > 0.0 && yDiff < 0.0)
            {
                angle = (2*M_PI/4*3) - angle;
            }
            else if (xDiff < 0.0 && yDiff < 0.0)
            {
                angle = (M_PI/2) - angle;
            }
            else if (xDiff < 0.0 && yDiff > 0.0)
            {
                angle = (M_PI/2) - angle;
            }

            const int length = 15;

            angle -= M_PI;
            qreal radDiff = qreal(2)* M_PI / qreal(360) * 20;
            qreal tRightAngle = angle + radDiff;
            const int x1Arrow = sin(tRightAngle)*length + x2;
            const int y1Arrow = cos(tRightAngle)*length + y2;

            qreal tLeftAngle = angle - radDiff;
            const int x2Arrow = sin(tLeftAngle)*length + x2;
            const int y2Arrow = cos(tLeftAngle)*length + y2;

            painter->drawLine(x1, y1, x2, y2);
            painter->drawLine(x1Arrow, y1Arrow, x2, y2);
            painter->drawLine(x2Arrow, y2Arrow, x2, y2);
        }
    }
}
