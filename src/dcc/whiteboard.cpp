/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboard.h"

#include "whiteboardtoolbar.h"
#include "whiteboardpaintarea.h"

#include <QScrollArea>

#include "kdebug.h"

namespace Konversation
{
    namespace DCC
    {
        WhiteBoard::WhiteBoard(QWidget* parent)
            : QWidget(parent)
        {
            QHBoxLayout *layout = new QHBoxLayout;
            m_toolbar = new WhiteBoardToolBar(this);
            layout->addWidget(m_toolbar);

            QScrollArea* area = new QScrollArea(this);
            m_paintArea = new WhiteBoardPaintArea(area);
            area->setWidgetResizable(true);
            area->setWidget(m_paintArea);

            layout->addWidget(area);
            setLayout(layout);

            connect(m_toolbar, SIGNAL(toolChanged(Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool)),
                    m_paintArea, SLOT(setTool(Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool)));

            connect(m_toolbar, SIGNAL(foregroundColorChanged(const QColor&)),
                    m_paintArea, SLOT(setForegroundColor(const QColor&)));
            connect(m_toolbar, SIGNAL(backgroundColorChanged(const QColor&)),
                    m_paintArea, SLOT(setBackgroundColor(const QColor&)));
            connect(m_toolbar, SIGNAL(colorsSwapped(const QColor&, const QColor&)),
                    m_paintArea, SLOT(swapColors(const QColor&, const QColor&)));

            connect(m_toolbar, SIGNAL(clear()), this, SLOT(clear()));
            connect(m_toolbar, SIGNAL(save(const QString&)), m_paintArea, SLOT(save(const QString&)));
            connect(m_toolbar, SIGNAL(lineWidthChanged(int)), m_paintArea, SLOT(setPenWidth(int)));

            connect(m_paintArea, SIGNAL(drawedPencil(int, const QColor&, const QColor&, int, int, int, int)),
                    this, SLOT(drawedPencil(int, const QColor&, const QColor&, int, int, int, int)));
            connect(m_paintArea, SIGNAL(drawedLine(int, const QColor&, const QColor&, int, int, int, int)),
                    this, SLOT(drawedLine(int, const QColor&, const QColor&, int, int, int, int)));
            connect(m_paintArea, SIGNAL(drawedRectangle(int, const QColor&, int, int, int, int)),
                    this, SLOT(drawedRectangle(int, const QColor&, int, int, int, int)));
            connect(m_paintArea, SIGNAL(drawedFilledRectangle(int, const QColor&, const QColor&, int, int, int, int)),
                    this, SLOT(drawedFilledRectangle(int, const QColor&, const QColor&, int, int, int, int)));
            connect(m_paintArea, SIGNAL(drawedEllipse(int, const QColor&, int, int, int, int)),
                    this, SLOT(drawedEllipse(int, const QColor&, int, int, int, int)));
            connect(m_paintArea, SIGNAL(drawedFilledEllipse(int, const QColor&, const QColor&, int, int, int, int)),
                    this, SLOT(drawedFilledEllipse(int, const QColor&, const QColor&, int, int, int, int)));
            connect(m_paintArea, SIGNAL(drawedArrow(int, const QColor&, int, int, int, int)),
                    this, SLOT(drawedArrow(int, const QColor&, int, int, int, int)));
            connect(m_paintArea, SIGNAL(usedEraser(int, int, int, int, int)),
                    this, SLOT(usedEraser(int, int, int, int, int)));
            connect(m_paintArea, SIGNAL(usedFloodFill(int, int, const QColor&)),
                    this, SLOT(usedFloodFill(int, int, const QColor&)));
        }

        WhiteBoard::~WhiteBoard()
        {
        }

        QStringList WhiteBoard::whiteboardCommands()
        {
            static const QStringList commands = WhiteBoardGlobals::wboardCommandHash().keys();
            return commands;
        }

        void WhiteBoard::receivedWhiteBoardLine(const QString& line)
        {
            // kDebug() << line;
            if (line.isEmpty())
                return;

            const QStringList firstSplit = line.split(' ', QString::SkipEmptyParts);
            if (firstSplit.isEmpty())
                return;

            const QString ctcpCommand(firstSplit.at(0));
            // kDebug() << "ctcpCommand" << ctcpCommand;
            // kDebug() << "firstSize" << firstSplit.size();

            if (ctcpCommand == "DR" && firstSplit.size() == 2)
            {
                // DR tooltype,width,pencolor,brushcolor,x1,y1,x2,y2
                const QStringList drArgsList = firstSplit.at(1).split(',', QString::SkipEmptyParts);
                if (drArgsList.size() != 8)
                {
                    kDebug() << "wrong size:" << drArgsList.size();
                    return;
                }

                bool ok = false;
                QString tmp = drArgsList.at(0);
                int toolType = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse tooltype:" << tmp;
                    return;
                }

                tmp = drArgsList.at(1);
                int lineWidth = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse linewidth:" << tmp;
                    return;
                }

                QColor penColor = parseColor(drArgsList.at(2), &ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse pencolor:" << drArgsList.at(2);
                    return;
                }

                QColor brushColor = parseColor(drArgsList.at(3), &ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse pencolor:" << drArgsList.at(3);
                    return;
                }

                tmp = drArgsList.at(4);
                int xFrom = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse xFrom:" << tmp;
                    return;
                }

                tmp = drArgsList.at(5);
                int yFrom = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse yFrom:" << tmp;
                    return;
                }

                tmp = drArgsList.at(6);
                int xTo = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse xTo:" << tmp;
                    return;
                }

                tmp = drArgsList.at(7);
                int yTo = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "unabled to parse yTo:" << tmp;
                    return;
                }

                // vIRC can spam us with invalid coords that vIRC itself just ignores
                if (xFrom < 0 && yFrom < 0 && xTo < 0 && yTo < 0)
                {
                    return;
                }

                switch (toolType)
                {
                case WhiteBoardGlobals::Line:
                case WhiteBoardGlobals::Pencil:
                    // kDebug() << "drawing a line" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawLine(lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::Rectangle:
                    // kDebug() << "drawing a rectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawRectangle(lineWidth, penColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::FilledRectangle:
                    // kDebug() << "drawing a filledrectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawFilledRectangle(lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::Ellipse:
                    // kDebug() << "drawing a rectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawEllipse(lineWidth, penColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::FilledEllipse:
                    // kDebug() << "drawing a filledrectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawFilledEllipse(lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::Eraser:
                    // kDebug() << "drawing a Eraser" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->useEraser(lineWidth, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::FloodFill:
                    // kDebug() << "drawing a FloodFill" << xFrom << yFrom;
                    m_paintArea->useFloodFill(xFrom, yFrom, penColor);
                    break;
                case WhiteBoardGlobals::Arrow:
                    // kDebug() << "drawing an Arrow" << xFrom << yFrom;
                    m_paintArea->drawArrow(lineWidth, penColor, xFrom, yFrom, xTo, yTo);
                    break;
                }
            }
            else if (ctcpCommand == "CLS" && firstSplit.size() == 1)
            {
                //CLS
                m_paintArea->clear();
            }
            else if (ctcpCommand == "CAN" && firstSplit.size() == 2)
            {
                //TODO implement me
                const QString& can = firstSplit.at(1);
                if (can == "can-wb2")
                {
                    //no we currently can't, I lied
                    emitDo(can);
                }
            }
            else if (ctcpCommand == "BLT" && firstSplit.size() == 2)
            {
                // BLT x1src,y1src,x2src,y2src,xdest,ydest
                const QStringList drArgsList = firstSplit.at(1).split(',', QString::SkipEmptyParts);
                if (drArgsList.size() != 6)
                {
                    kDebug() << "blt wrong size:" << drArgsList.size();
                    return;
                }

                bool ok = false;
                bool finalOk = true;
                int x1src = drArgsList.at(0).toInt(&ok);
                finalOk &= ok;
                int y1src = drArgsList.at(1).toInt(&ok);
                finalOk &= ok;
                int x2src = drArgsList.at(2).toInt(&ok);
                finalOk &= ok;
                int y2src = drArgsList.at(3).toInt(&ok);
                finalOk &= ok;
                int xdest = drArgsList.at(4).toInt(&ok);
                finalOk &= ok;
                int ydest = drArgsList.at(5).toInt(&ok);
                finalOk &= ok;

                if (!finalOk)
                {
                    kDebug() << "blt unabled to parse coords:" << firstSplit.at(1);
                    return;
                }

                if (x2src <= x1src || y2src <= y1src)
                {
                    kDebug() << "blt coords invalid:" << firstSplit.at(1);
                    return;
                }
                m_paintArea->useBlt(x1src, y1src, x2src, y2src, xdest, ydest);
            }
        }

        void WhiteBoard::clear()
        {
            kDebug();
            m_paintArea->clear();
            static const QString cls = QString("\x01""CLS\x01");
            emit rawWhiteBoardCommand(cls);
        }

        void WhiteBoard::drawedPencil(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                      int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::Pencil, lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::drawedLine(int lineWidth, const QColor& penColor, const QColor& brushColor,
                        int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::Line, lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::drawedRectangle(int lineWidth, const QColor& penColor, int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::Rectangle, lineWidth, penColor, QColor(0,0,0), xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::drawedFilledRectangle(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                               int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::FilledRectangle, lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::drawedEllipse(int lineWidth, const QColor& penColor, int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::Ellipse, lineWidth, penColor, QColor(0,0,0), xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::drawedFilledEllipse(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                               int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::FilledEllipse, lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::drawedArrow(int lineWidth, const QColor& penColor, int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::Arrow, lineWidth, penColor, QColor(0,0,0), xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::usedEraser(int lineWidth, int xFrom, int yFrom, int xTo, int yTo)
        {
            emitDRCommand(WhiteBoardGlobals::Eraser, lineWidth, QColor(0,0,0), QColor(0,0,0), xFrom, yFrom, xTo, yTo);
        }

        void WhiteBoard::usedFloodFill(int x, int y, const QColor& color)
        {
            emitDRCommand(WhiteBoardGlobals::FloodFill, 0, color, QColor(0,0,0), x, y, 0, 0);
        }

        QColor WhiteBoard::parseColor(const QString& colorString, bool* ok)
        {
            bool tOk = false;
            int colorInt = colorString.toInt(&tOk);
            if (!tOk || colorInt > 0xFFFFFF)
            {
                if (ok)
                {
                    *ok = false;
                }
                return QColor();
            }
            int r = colorInt & 0x0000FF;
            int g = (colorInt & 0x00FF00) >> 8;
            int b = (colorInt & 0xFF0000) >> 16;

            if (ok)
            {
                *ok = true;
            }

            return QColor(r,g,b);
        }

        QString WhiteBoard::colorToString(const QColor& color)
        {
            return QString::number((color.blue()<<16) | (color.green()<<8) | (color.red()));
        }

        void WhiteBoard::connected()
        {
            emitCan("use-wb2");
        }

        void WhiteBoard::emitDRCommand(WhiteBoardGlobals::WhiteBoardTool tool, int lineWidth,
                                       const QColor& penColor, const QColor& brushColor,
                                       int xFrom, int yFrom, int xTo, int yTo)
        {
            if (xFrom < 0 && yFrom < 0 && xTo < 0 && yTo < 0)
            {
                return;
            }

            QString drLineCommand("\x01""DR %2,%3,%4,%5,%6,%7,%8,%9\x01");
            drLineCommand = drLineCommand.arg(QString::number(tool)).arg(
                                          QString::number(lineWidth)).arg(colorToString(penColor)).arg(colorToString(brushColor)).arg(
                                          QString::number(xFrom)).arg(QString::number(yFrom)).arg(QString::number(xTo)).arg(QString::number(yTo));
            kDebug() << drLineCommand;
            emit rawWhiteBoardCommand(drLineCommand);
        }

        void WhiteBoard::emitCan(const QString& canString)
        {
            static const QString can = QString("\x01""CAN %1\x01");
            emit rawWhiteBoardCommand(can.arg(canString));
        }

        void WhiteBoard::emitCant(const QString& cantString)
        {
            static const QString cant = QString("\x01""CANT %1\x01");
            emit rawWhiteBoardCommand(cant.arg(cantString));
        }

        void WhiteBoard::emitDo(const QString& doString)
        {
            static const QString doCommand = QString("\x01""DO %1\x01");
            emit rawWhiteBoardCommand(doCommand.arg(doString));
        }

        void WhiteBoard::emitDont(const QString& dontString)
        {
            static const QString dont = QString("\x01""DONT %1\x01");
            emit rawWhiteBoardCommand(dont.arg(dontString));
        }

    }
}
