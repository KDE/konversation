/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009-2010 Bernd Buschinski <b.buschinski@web.de>
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

            connect(m_toolbar, SIGNAL(foregroundColorChanged(QColor)),
                    m_paintArea, SLOT(setForegroundColor(QColor)));
            connect(m_toolbar, SIGNAL(backgroundColorChanged(QColor)),
                    m_paintArea, SLOT(setBackgroundColor(QColor)));
            connect(m_toolbar, SIGNAL(colorsSwapped(QColor,QColor)),
                    m_paintArea, SLOT(swapColors(QColor,QColor)));
            connect(m_toolbar, SIGNAL(fontChanged(QFont)),
                    m_paintArea, SLOT(setFont(QFont)));

            connect(m_toolbar, SIGNAL(clear()), this, SLOT(clear()));
            connect(m_toolbar, SIGNAL(save(QString)), m_paintArea, SLOT(save(QString)));
            connect(m_toolbar, SIGNAL(lineWidthChanged(int)), m_paintArea, SLOT(setPenWidth(int)));

            connect(m_paintArea, SIGNAL(drawedPencil(int,QColor,QColor,int,int,int,int)),
                    this, SLOT(drawedPencil(int,QColor,QColor,int,int,int,int)));
            connect(m_paintArea, SIGNAL(drawedLine(int,QColor,QColor,int,int,int,int)),
                    this, SLOT(drawedLine(int,QColor,QColor,int,int,int,int)));
            connect(m_paintArea, SIGNAL(drawedRectangle(int,QColor,int,int,int,int)),
                    this, SLOT(drawedRectangle(int,QColor,int,int,int,int)));
            connect(m_paintArea, SIGNAL(drawedFilledRectangle(int,QColor,QColor,int,int,int,int)),
                    this, SLOT(drawedFilledRectangle(int,QColor,QColor,int,int,int,int)));
            connect(m_paintArea, SIGNAL(drawedEllipse(int,QColor,int,int,int,int)),
                    this, SLOT(drawedEllipse(int,QColor,int,int,int,int)));
            connect(m_paintArea, SIGNAL(drawedFilledEllipse(int,QColor,QColor,int,int,int,int)),
                    this, SLOT(drawedFilledEllipse(int,QColor,QColor,int,int,int,int)));
            connect(m_paintArea, SIGNAL(drawedArrow(int,QColor,int,int,int,int)),
                    this, SLOT(drawedArrow(int,QColor,int,int,int,int)));
            connect(m_paintArea, SIGNAL(usedEraser(int,int,int,int,int)),
                    this, SLOT(usedEraser(int,int,int,int,int)));
            connect(m_paintArea, SIGNAL(usedFloodFill(int,int,QColor)),
                    this, SLOT(usedFloodFill(int,int,QColor)));
            connect(m_paintArea, SIGNAL(usedText(int,int,QString)),
                    this, SLOT(usedText(int,int,QString)));
            connect(m_paintArea, SIGNAL(usedTextExtended(int,int,QFont,QColor,QColor,QString)),
                    this, SLOT(usedTextExtended(int,int,QFont,QColor,QColor,QString)));

            connect(m_paintArea, SIGNAL(colorPicked(QColor)),
                    m_toolbar, SLOT(setForegroundColor(QColor)));
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
            if (line.isEmpty() || !line.contains(' '))
                return;

            //fontname can have spaces, as well as text
            QStringList firstSplit;
            firstSplit.append(line.section(' ', 0 ,0));
            firstSplit.append(line.section(' ', 1));

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
                    kDebug() << "unabled to parse brush:" << drArgsList.at(3);
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
                    // kDebug() << "drawing a line" << lineWidth << xFrom << yFrom << xTo << yTo;
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
            else if (ctcpCommand == "TXT" && firstSplit.size() == 2)
            {
                QStringList txtArgsList = firstSplit.at(1).split(',', QString::KeepEmptyParts);
                if (txtArgsList.size() < 3)
                {
                    kDebug() << "txt wrong size:" << txtArgsList.size();
                    return;
                }

                QString tmp = txtArgsList.at(0);
                bool ok;
                int x1 = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "txt unabled to parse x1:" << tmp;
                    return;
                }

                tmp = txtArgsList.at(1);
                int y1 = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "txt unabled to parse y1:" << tmp;
                    return;
                }

                txtArgsList.removeFirst();
                txtArgsList.removeFirst();
                QString text(txtArgsList.join(","));

                m_paintArea->useText(x1,y1,text);
            }
            else if (ctcpCommand == "TXTEX" && firstSplit.size() == 2)
            {
                QStringList txtArgsList = firstSplit.at(1).split(',', QString::KeepEmptyParts);
                if (txtArgsList.size() < 8)
                {
                    kDebug() << "txtex wrong size:" << txtArgsList.size();
                    return;
                }

                QString tmp = txtArgsList.at(0);
                bool ok;
                int x1 = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "txtex unabled to parse x1:" << tmp;
                    return;
                }

                tmp = txtArgsList.at(1);
                int y1 = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "txtex unabled to parse y1:" << tmp;
                    return;
                }

                QString fontName = txtArgsList.at(2);

                tmp = txtArgsList.at(3);
                int fontSize = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "txtex unabled to parse fontsize:" << tmp;
                    return;
                }

                tmp = txtArgsList.at(4);
                int fontStyle = tmp.toInt(&ok);
                if (!ok)
                {
                    kDebug() << "txtex unabled to parse fontstyle:" << tmp;
                    return;
                }

                QColor penColor = parseColor(txtArgsList.at(5), &ok);
                if (!ok)
                {
                    kDebug() << "txtex unabled to parse pencolor:" << txtArgsList.at(5);
                    return;
                }

                QColor brushColor = parseColor(txtArgsList.at(6), &ok);
                if (!ok)
                {
                    kDebug() << "txtex unabled to parse brush:" << txtArgsList.at(6);
                    return;
                }

                QFont tFont(fontName, fontSize);
                if (fontStyle & WhiteBoardGlobals::Underline)
                    tFont.setUnderline(true);

                if (fontStyle & WhiteBoardGlobals::Strikeout)
                    tFont.setStrikeOut(true);

                if (fontStyle & WhiteBoardGlobals::Italic)
                    tFont.setItalic(true);

                if (fontStyle & WhiteBoardGlobals::Bold)
                    tFont.setBold(true);

                txtArgsList.removeFirst(); // x1
                txtArgsList.removeFirst(); // y1
                txtArgsList.removeFirst(); // fontname
                txtArgsList.removeFirst(); // fontsize
                txtArgsList.removeFirst(); // fontstyle
                txtArgsList.removeFirst(); // textcolor
                txtArgsList.removeFirst(); // bgcolor
                QString text(txtArgsList.join(","));
                // kDebug() << "TXTEX" << text << fontSize << fontName;
                m_paintArea->useTextExtended(x1,y1,tFont,penColor,brushColor,text);
            }
            else if (ctcpCommand == "CLS" && firstSplit.size() == 1)
            {
                //CLS
                m_paintArea->clear();
            }
            else if (ctcpCommand == "CAN" && firstSplit.size() == 2)
            {
                //TODO implement me
                const QString& can = firstSplit.at(1).toLower();
                if (can == "use-wb2")
                {
                    //no we currently can't, I lied
                    emitDo("use-wb2");
                    return;
                }
                else if (can == "txtex")
                {
                    emitDo("TXTEX");
                    m_toolbar->setSupportedTextType(WhiteBoardToolBar::ExtentedText);
                    return;
                }
                kDebug() << "unhandled CAN" << firstSplit.at(1);
            }
            else if (ctcpCommand == "CANT" && firstSplit.size() == 2)
            {
                //TODO implement me
                const QString& cannot = firstSplit.at(1).toLower();
                if (cannot == "txtex")
                {
                    m_toolbar->setSupportedTextType(WhiteBoardToolBar::SimpleText);
                    return;
                }
                kDebug() << "unhandled CANT" << firstSplit.at(1);
            }
            else if (ctcpCommand == "DO" && firstSplit.size() == 2)
            {
                //TODO implement me
                const QString& doString = firstSplit.at(1).toLower();
                if (doString == "txtex")
                {
                    m_toolbar->setSupportedTextType(WhiteBoardToolBar::ExtentedText);
                    return;
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

        void WhiteBoard::usedText(int x, int y, const QString& text)
        {
            //TXT x,y,text
            static const QString txtLineCommand("\x01""TXT %1,%2,%3\x01");

            emit rawWhiteBoardCommand(txtLineCommand.arg(x).arg(y).arg(text));
        }

        void WhiteBoard::usedTextExtended(int x, int y, const QFont& font, const QColor& textColor, const QColor& background, const QString& text)
        {
            //TXTEX x,y,fontname,ptsize,style,textcolor,bgcolor,text
            static const QString txtexLineCommand("\x01""TXTEX %1,%2,%3,%4,%5,%6,%7,%8\x01");

            QString fontname = font.family();
            QString fontSize = QString::number(font.pointSize());
            QString fontStyle = QString::number(fontToStyle(font));

            emit rawWhiteBoardCommand(txtexLineCommand.arg(x).arg(y).arg(
                                                           fontname).arg(fontSize).arg(fontStyle).arg(colorToString(textColor)).arg(
                                                           colorToString(background)).arg(text));
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

        int WhiteBoard::fontToStyle(const QFont& font)
        {
            int style = 0;
            if (font.underline())
                style |= WhiteBoardGlobals::Underline;

            if (font.bold())
                style |= WhiteBoardGlobals::Bold;

            if (font.strikeOut())
                style |= WhiteBoardGlobals::Strikeout;

            if (font.italic())
                style |= WhiteBoardGlobals::Italic;

            return style;
        }

        void WhiteBoard::connected()
        {
            emitCan("use-wb2");
            emitCan("TXTEX");
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
            drLineCommand = drLineCommand.arg(tool).arg(
                                          lineWidth).arg(colorToString(penColor)).arg(colorToString(brushColor)).arg(
                                          xFrom).arg(yFrom).arg(xTo).arg(yTo);
            // kDebug() << drLineCommand;
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

        void WhiteBoard::emitDont(const QString& doNotString)
        {
            static const QString doNot = QString("\x01""DONT %1\x01"); //krazy:exclude=spelling
            emit rawWhiteBoardCommand(doNot.arg(doNotString));
        }

    }
}
