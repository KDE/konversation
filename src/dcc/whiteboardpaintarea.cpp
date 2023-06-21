/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009-2010 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboardpaintarea.h"

#include "konversation_log.h"

#include <cmath>

#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QStack>

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

        WhiteBoardPaintArea::~WhiteBoardPaintArea()
        {
            delete m_imagePixmap;
            delete m_overlayPixmap;
        }

        void WhiteBoardPaintArea::setTool(WhiteBoardGlobals::WhiteBoardTool tool)
        {
            //qCDebug(KONVERSATION_LOG) << "newtool" << tool;
            if (tool != m_tool && (m_tool == WhiteBoardGlobals::Text || m_tool == WhiteBoardGlobals::TextExtended))
            {
                finishText();
                makeLastPosInvalid();
                update();
            }
            m_tool = tool;
        }

        void WhiteBoardPaintArea::setForegroundColor(const QColor& color)
        {
            m_foregroundColor = color;
            if (m_tool == WhiteBoardGlobals::Text || m_tool == WhiteBoardGlobals::TextExtended)
            {
                if (isLastPosValid())
                {
                    m_overlayPixmap->fill(Qt::transparent);
                    text(m_overlayPixmap, m_font, m_foregroundColor, m_backgroundColor, m_lastPos.x(), m_lastPos.y(), m_writtenText, true, m_tool);
                    update();
                }
            }
        }

        void WhiteBoardPaintArea::setBackgroundColor(const QColor& color)
        {
            m_backgroundColor = color;
        }

        void WhiteBoardPaintArea::swapColors(const QColor& newForeground, const QColor& newBackground)
        {
            m_foregroundColor = newForeground;
            m_backgroundColor = newBackground;
            if (m_tool == WhiteBoardGlobals::Text || m_tool == WhiteBoardGlobals::TextExtended)
            {
                if (isLastPosValid())
                {
                    m_overlayPixmap->fill(Qt::transparent);
                    text(m_overlayPixmap, m_font, m_foregroundColor, m_backgroundColor, m_lastPos.x(), m_lastPos.y(), m_writtenText, true, m_tool);
                    update();
                }
            }
        }

        void WhiteBoardPaintArea::setPenWidth(int width)
        {
            m_penWidth = width;
        }

        void WhiteBoardPaintArea::setFont(const QFont& font)
        {
            m_font = font;
            if (isLastPosValid() && m_tool == WhiteBoardGlobals::TextExtended)
            {
                m_overlayPixmap->fill(Qt::transparent);
                text(m_overlayPixmap, m_font, m_foregroundColor, m_backgroundColor, m_lastPos.x(), m_lastPos.y(), m_writtenText, true, m_tool);
                update();
            }
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
            tPaint.setPen(getPen(penColor, lineWidth, WhiteBoardGlobals::Line));
            tPaint.setBrush(brushColor);

            // drawLine with lineWidth=1 creates a dot
            // but drawLine with lineWith>1 creates nothing (with Qt 4.6.1, Bug?)
            // check if start==endPos and use drawPoint in this case
            if (xFrom == xTo && yFrom == yTo)
            {
                tPaint.drawPoint(xFrom, yFrom);
            }
            else
            {
                tPaint.drawLine(xFrom, yFrom, xTo, yTo);
            }
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawRectangle(int lineWidth, const QColor& penColor,
                                                int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, WhiteBoardGlobals::Rectangle));
            tPaint.drawRect(xFrom, yFrom, xTo-xFrom, yTo-yFrom);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawFilledRectangle(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                                      int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, WhiteBoardGlobals::FilledRectangle));
            tPaint.setBrush(brushColor);
            drawRect(&tPaint, xFrom, yFrom, xTo, yTo);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawEllipse(int lineWidth, const QColor& penColor,
                                              int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, WhiteBoardGlobals::Ellipse));
            tPaint.drawEllipse(xFrom, yFrom, xTo-xFrom, yTo-yFrom);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawFilledEllipse(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                                    int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, WhiteBoardGlobals::FilledEllipse));
            tPaint.setBrush(brushColor);
            tPaint.drawEllipse(xFrom, yFrom, xTo-xFrom, yTo-yFrom);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::drawArrow(int lineWidth, const QColor& penColor, int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(penColor, lineWidth, WhiteBoardGlobals::Arrow));
            arrow(&tPaint, xFrom, yFrom, xTo, yTo);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::useEraser(int lineWidth, int xFrom, int yFrom, int xTo, int yTo)
        {
            checkImageSize(xFrom, yFrom, xTo, yTo, lineWidth);
            QPainter tPaint(m_imagePixmap);
            tPaint.setPen(getPen(Qt::white, lineWidth, WhiteBoardGlobals::Eraser));
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
            QPixmap copyPix(m_imagePixmap->copy(x1src, y1src, x2src-x1src, y2src-y1src));
            QPainter tPaint(m_imagePixmap);
            tPaint.drawPixmap(xdest, ydest, copyPix);
            tPaint.end();
            update();
        }

        void WhiteBoardPaintArea::useText(int x1, int y1, const QString& textString)
        {
            text(m_imagePixmap, QFont(), Qt::black, Qt::white, x1, y1, textString, false, WhiteBoardGlobals::Text);
            update();
        }

        void WhiteBoardPaintArea::useTextExtended(int x1, int y1, const QFont& font, const QColor& foreGround, const QColor& backGround, const QString& textString)
        {
            text(m_imagePixmap, font, foreGround, backGround, x1, y1, textString, false, WhiteBoardGlobals::TextExtended);
            update();
        }

        void WhiteBoardPaintArea::save(const QString& fileName)
        {
            m_imagePixmap->save(fileName);
        }

        void WhiteBoardPaintArea::paintEvent(QPaintEvent *event)
        {
            //qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            QPainter tPaint;
            tPaint.begin(this);

            const auto region = event->region();
            for (const QRect& rect : region) {
                tPaint.drawPixmap(rect, *m_imagePixmap, rect);

                tPaint.drawPixmap(rect, *m_overlayPixmap, rect);
            }

            tPaint.end();
        }

        void WhiteBoardPaintArea::resizeEvent(QResizeEvent *event)
        {
            Q_UNUSED(event)
            // qCDebug(KONVERSATION_LOG) << "newsize:" << event->size();
            // qCDebug(KONVERSATION_LOG) << "newcontent:" << contentsRect().width() << " " << contentsRect().height();
            resizeImage(contentsRect().width(), contentsRect().height());
        }

        void WhiteBoardPaintArea::mousePressEvent(QMouseEvent *event)
        {
            // pencil is unabled to cancel as its always directly send
            if (event->buttons() & Qt::RightButton && m_tool != WhiteBoardGlobals::Pencil)
            {
                m_mousePressed = false;
                m_overlayPixmap->fill(Qt::transparent);
                m_writtenText.clear();
                makeLastPosInvalid();
                update();
            }
            else if (event->buttons() & Qt::LeftButton)
            {
                m_mousePressed = true;
                if (m_tool == WhiteBoardGlobals::FloodFill)
                {
                    makeLastPosInvalid();
                    QCursor oldCur = cursor();
                    setCursor(Qt::WaitCursor);
                    floodfill(event->pos().x(), event->pos().y(), m_foregroundColor);
                    setCursor(oldCur);
                    Q_EMIT usedFloodFill(event->pos().x(), event->pos().y(), m_foregroundColor);
                    update();
                }
                else if (m_tool == WhiteBoardGlobals::TextExtended || m_tool == WhiteBoardGlobals::Text)
                {
                    m_mousePressed = false;
                    m_overlayPixmap->fill(Qt::transparent);
                    if (isLastPosValid())
                    {
                        finishText();
                        m_lastPos = event->pos();
                        text(m_overlayPixmap, m_font, m_foregroundColor, m_backgroundColor, event->pos().x(), event->pos().y(), m_writtenText, true, m_tool);
                        setFocus();
                    }
                    else
                    {
                        m_lastPos = event->pos();
                        text(m_overlayPixmap, m_font, m_foregroundColor, m_backgroundColor, event->pos().x(), event->pos().y(), m_writtenText, true, m_tool);
                        setFocus();
                    }
                    update();
                }
                else
                {
                    makeLastPosInvalid();
                    mouseMoveEvent(event);
                }
            }
        }

        void WhiteBoardPaintArea::mouseReleaseEvent(QMouseEvent *event)
        {
            if (event->button() == Qt::LeftButton && m_mousePressed)
            {
                m_mousePressed = false;
                switch (m_tool)
                {
                case WhiteBoardGlobals::Line:
                    Q_EMIT drawedLine(m_penWidth, m_foregroundColor, m_backgroundColor,
                                    m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;

                case WhiteBoardGlobals::Pencil:
                case WhiteBoardGlobals::Eraser:
                case WhiteBoardGlobals::FloodFill:
                case WhiteBoardGlobals::Stamp:
                case WhiteBoardGlobals::ColorPicker:
                    break;

                case WhiteBoardGlobals::Rectangle:
                    Q_EMIT drawedRectangle(m_penWidth, m_foregroundColor,
                                         m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::FilledRectangle:
                    Q_EMIT drawedFilledRectangle(m_penWidth, m_foregroundColor, m_backgroundColor,
                                               m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::Ellipse:
                    Q_EMIT drawedEllipse(m_penWidth, m_foregroundColor,
                                       m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::FilledEllipse:
                    Q_EMIT drawedFilledEllipse(m_penWidth, m_foregroundColor, m_backgroundColor,
                                             m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::Arrow:
                    Q_EMIT drawedArrow(m_penWidth, m_foregroundColor,
                                     m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                    break;
                case WhiteBoardGlobals::Text:
                case WhiteBoardGlobals::TextExtended:
                    //don't finish writing here, finish it on tool change or when clicking somewhere else
                    return;

                case WhiteBoardGlobals::Selection:
                    qCDebug(KONVERSATION_LOG) << "TODO implement whiteboard Selection";
                    return;
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
                            Q_EMIT drawedPencil(m_penWidth, m_foregroundColor, m_backgroundColor,
                                              m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                        }
                        else
                        {
                            tPainter.drawPoint(event->pos());
                            Q_EMIT drawedLine(m_penWidth, m_foregroundColor, m_backgroundColor,
                                            event->pos().x(), event->pos().y(), event->pos().x(), event->pos().y());
                        }
                    }
                    m_lastPos = event->pos();
                    break;

                case WhiteBoardGlobals::FilledEllipse:
                    tPainter.setBrush(m_backgroundColor);
                    // fallthrough, same as WhiteBoardGlobals::Ellipse just
                    // with extra color
                    Q_FALLTHROUGH();
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
                    // fallthrough, same as WhiteBoardGlobals::Rectangle just
                    // with extra color
                    Q_FALLTHROUGH();
                case WhiteBoardGlobals::Rectangle:
                    {
                        if (isLastPosValid())
                        {
                            m_overlayPixmap->fill(Qt::transparent);
                            drawRect(&tPainter, m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
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
                            Q_EMIT usedEraser(m_penWidth, m_lastPos.x(), m_lastPos.y(), event->pos().x(), event->pos().y());
                        }
                        else
                        {
                            qCDebug(KONVERSATION_LOG) << "draw eraser point";
                            tPainter.drawPoint(event->pos());
                            Q_EMIT usedEraser(m_penWidth, event->pos().x(), event->pos().y(), event->pos().x(), event->pos().y());
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

                case WhiteBoardGlobals::TextExtended:
                case WhiteBoardGlobals::Text:
                    //handle in mousePressEvent
                    break;

                case WhiteBoardGlobals::ColorPicker:
                    {
                        QImage image = m_imagePixmap->toImage();
                        QColor fgColor(image.pixel(event->pos().x(), event->pos().y()));
                        m_foregroundColor = fgColor;
                        Q_EMIT colorPicked(fgColor);
                    }
                    break;
                case WhiteBoardGlobals::Stamp:
                case WhiteBoardGlobals::Selection:
                    qCDebug(KONVERSATION_LOG) << "TODO implement whiteboard Selection/Stamp";
                    break;
                }
                tPainter.end();

                update();
            }
        }

        void WhiteBoardPaintArea::keyPressEvent(QKeyEvent* event)
        {
            // qCDebug(KONVERSATION_LOG) << event->text() << event->text().length() << int(event->text()[0].toLatin1());

            if ((m_tool == WhiteBoardGlobals::Text || m_tool == WhiteBoardGlobals::TextExtended) && isLastPosValid())
            {
                if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
                {
                    finishText();
                    update();
                    return;
                }

                if (event->key() != Qt::Key_Backspace)
                {
                    QString eventText = event->text();
                    //chars before 32(space) are not printable
                    if (eventText.length() == 1 && !eventText.at(0).isPrint())
                    {
                        return;
                    }
                    m_writtenText += eventText;
                }
                else
                {
                    m_writtenText.chop(1);
                }
                m_overlayPixmap->fill(Qt::transparent);
                text(m_overlayPixmap, m_font, m_foregroundColor, m_backgroundColor, m_lastPos.x(), m_lastPos.y(), m_writtenText, true, m_tool);
                update();
            }
            QWidget::keyPressEvent(event);
        }

        void WhiteBoardPaintArea::makeLastPosInvalid()
        {
            m_lastPos.setX(InvalidLastPos);
            m_lastPos.setY(InvalidLastPos);
        }

        bool WhiteBoardPaintArea::isLastPosValid() const
        {
            if (m_lastPos.x() > InvalidLastPos && m_lastPos.y() > InvalidLastPos)
            {
                return true;
            }
            return false;
        }

        void WhiteBoardPaintArea::checkImageSize(int x1, int y1, int x2, int y2, int penWidth)
        {
            // qCDebug(KONVERSATION_LOG) << x1 << y1 << x2 << y2;
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
                tPaint.drawPixmap(0, 0, *oldImg);
                tPaint.end();
                delete oldImg;

                QPixmap* oldOverlay = m_overlayPixmap;
                m_overlayPixmap = new QPixmap(width, height);
                m_overlayPixmap->fill(Qt::transparent);
                QPainter tPaintOverlay(m_overlayPixmap);
                tPaintOverlay.drawPixmap(0, 0, *oldOverlay);
                tPaintOverlay.end();
                delete oldOverlay;

                const int maxWidth = qMin(qMax(width, minimumSize().width()), WhiteBoardGlobals::MaxImageSize);
                const int maxHeight = qMin(qMax(height, minimumSize().height()), WhiteBoardGlobals::MaxImageSize);
                setMinimumSize(maxWidth, maxHeight);
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
                angle = 2.0*M_PI/4.0*3.0;
            }
            else if (xDiff == 0 && yDiff < 0)
            {
                angle = 0;
            }
            else if (xDiff < 0 && yDiff == 0)
            {
                angle = M_PI/2.0;
            }
            else
            {
                angle = atan(qreal(yDiff)/qreal(xDiff));

                if (xDiff > 0.0 && yDiff > 0.0)
                {
                    angle = (M_PI/2.0) - angle + M_PI;
                }
                else if (xDiff > 0.0 && yDiff < 0.0)
                {
                    angle = (2.0*M_PI/4.0*3.0) - angle;
                }
                else if (xDiff < 0.0)
                {
                    angle = (M_PI/2.0) - angle;
                }
            }

            static const int arrowLength = 9;

            angle -= M_PI;
            static const qreal radDiff = qreal(2)* M_PI / qreal(360) * 22;
            qreal tRightAngle = angle + radDiff;
            const int x1Arrow = static_cast<int>(sin(tRightAngle) * arrowLength + x2);
            const int y1Arrow = static_cast<int>(cos(tRightAngle) * arrowLength + y2);

            qreal tLeftAngle = angle - radDiff;
            const int x2Arrow = static_cast<int>(sin(tLeftAngle) * arrowLength + x2);
            const int y2Arrow = static_cast<int>(cos(tLeftAngle) * arrowLength + y2);

            painter->drawLine(x1, y1, x2, y2);
            painter->drawLine(x1Arrow, y1Arrow, x2, y2);
            painter->drawLine(x2Arrow, y2Arrow, x2, y2);
        }

        void WhiteBoardPaintArea::text(QPaintDevice* device, const QFont& font, const QColor& foreGround,
                                       const QColor& backGround, int x1, int y1, const QString& textString,
                                       bool drawSelection, Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool)
        {
            QFontMetrics tMetrics(font);
            QSize tSize = tMetrics.size(Qt::TextSingleLine, textString);

            //HACK if size changes device is invalid
            bool isOverlay = (device == m_overlayPixmap);
            checkImageSize(x1,y1,x1+tSize.width(), y1+tSize.height());

            if (isOverlay)
            {
                device = m_overlayPixmap;
            }
            else
            {
                device = m_imagePixmap;
            }

            QPainter tPaint(device);
            tPaint.setFont(font);
            if (tool == WhiteBoardGlobals::TextExtended)
            {
                tPaint.setBrush(backGround);

                //paint background, but I don't like it
//                 tPaint.setPen(getPen(backGround, 1, WhiteBoardGlobals::FilledRectangle));
//                 tPaint.drawRect(x1, y1, tSize.width(), tSize.height());

                tPaint.setPen(getPen(foreGround, 1, tool));
            }

            tPaint.drawText(x1, y1+tSize.height(), textString);
            if (drawSelection)
            {
                //draw white and blue dash to make it also visible on blue background
                tPaint.setBrush(Qt::transparent);
                QPen tPen = getPen(Qt::blue, 1, tool);
                tPen.setStyle(Qt::CustomDashLine);

                static const qreal dashLength = 4;
                const QVector<qreal> dashes = { dashLength, dashLength };
                tPen.setDashPattern(dashes);

                tPaint.setPen(tPen);
                tPaint.drawRect(x1-1,y1-1, tSize.width()+1, tSize.height()+1);

                tPen.setDashOffset(dashLength);
                tPen.setColor(Qt::white);

                tPaint.setPen(tPen);
                tPaint.drawRect(x1-1,y1-1, tSize.width()+1, tSize.height()+1);
            }
            tPaint.end();
        }

        void WhiteBoardPaintArea::finishText()
        {
            m_overlayPixmap->fill(Qt::transparent);
            if (m_writtenText.isEmpty())
            {
                return;
            }

            text(m_imagePixmap, m_font, m_foregroundColor, m_backgroundColor, m_lastPos.x(), m_lastPos.y(), m_writtenText, false, m_tool);
            if (m_tool == WhiteBoardGlobals::TextExtended)
            {
                Q_EMIT usedTextExtended(m_lastPos.x(), m_lastPos.y(), m_font, m_foregroundColor, m_backgroundColor, m_writtenText);
            }
            else if (m_tool == WhiteBoardGlobals::Text)
            {
                Q_EMIT usedText(m_lastPos.x(), m_lastPos.y(), m_writtenText);
            }
            m_writtenText.clear();
        }

        void WhiteBoardPaintArea::drawRect(QPainter* painter, int xFrom, int yFrom, int xTo, int yTo)
        {
            // if Length is smaller zero then the filled brush is too big by one pixel the left and top
            // the border(pen) is correct though, is it a Qt bug? (Qt-4.6.2 + raster)
            int xLength = xTo - xFrom;
            int yLength = yTo - yFrom;
            if (xLength < 0)
            {
                xLength = -xLength;
            }
            if (yLength < 0)
            {
                yLength = -yLength;
            }
            painter->drawRect(qMin(xFrom, xTo), qMin(yFrom, yTo), xLength, yLength);
        }
    }
}

#include "moc_whiteboardpaintarea.cpp"
