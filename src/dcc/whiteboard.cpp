/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009-2010 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboard.h"

#include "whiteboardtoolbar.h"
#include "whiteboardpaintarea.h"
#include "konversation_log.h"

#include <QScrollArea>

namespace Konversation
{
    namespace DCC
    {
        WhiteBoard::WhiteBoard(QWidget* parent)
            : QWidget(parent)
        {
            auto *layout = new QHBoxLayout;
            m_toolbar = new WhiteBoardToolBar(this);
            layout->addWidget(m_toolbar);

            auto* area = new QScrollArea(this);
            m_paintArea = new WhiteBoardPaintArea(area);
            area->setWidgetResizable(true);
            area->setWidget(m_paintArea);

            layout->addWidget(area);
            setLayout(layout);

            connect(m_toolbar, &WhiteBoardToolBar::toolChanged, m_paintArea, &WhiteBoardPaintArea::setTool);

            connect(m_toolbar, &WhiteBoardToolBar::foregroundColorChanged, m_paintArea, &WhiteBoardPaintArea::setForegroundColor);
            connect(m_toolbar, &WhiteBoardToolBar::backgroundColorChanged, m_paintArea, &WhiteBoardPaintArea::setBackgroundColor);
            connect(m_toolbar, &WhiteBoardToolBar::colorsSwapped, m_paintArea, &WhiteBoardPaintArea::swapColors);
            connect(m_toolbar, &WhiteBoardToolBar::fontChanged, m_paintArea, &WhiteBoardPaintArea::setFont);

            connect(m_toolbar, &WhiteBoardToolBar::clear, this, &WhiteBoard::clear);
            connect(m_toolbar, &WhiteBoardToolBar::save, m_paintArea, &WhiteBoardPaintArea::save);
            connect(m_toolbar, &WhiteBoardToolBar::lineWidthChanged, m_paintArea, &WhiteBoardPaintArea::setPenWidth);

            connect(m_paintArea, &WhiteBoardPaintArea::drawedPencil, this, &WhiteBoard::drawedPencil);
            connect(m_paintArea, &WhiteBoardPaintArea::drawedLine, this, &WhiteBoard::drawedLine);
            connect(m_paintArea, &WhiteBoardPaintArea::drawedRectangle, this, &WhiteBoard::drawedRectangle);
            connect(m_paintArea, &WhiteBoardPaintArea::drawedFilledRectangle, this, &WhiteBoard::drawedFilledRectangle);
            connect(m_paintArea, &WhiteBoardPaintArea::drawedEllipse, this, &WhiteBoard::drawedEllipse);
            connect(m_paintArea, &WhiteBoardPaintArea::drawedFilledEllipse, this, &WhiteBoard::drawedFilledEllipse);
            connect(m_paintArea, &WhiteBoardPaintArea::drawedArrow, this, &WhiteBoard::drawedArrow);
            connect(m_paintArea, &WhiteBoardPaintArea::usedEraser, this, &WhiteBoard::usedEraser);
            connect(m_paintArea, &WhiteBoardPaintArea::usedFloodFill, this, &WhiteBoard::usedFloodFill);
            connect(m_paintArea, &WhiteBoardPaintArea::usedText, this, &WhiteBoard::usedText);
            connect(m_paintArea, &WhiteBoardPaintArea::usedTextExtended, this, &WhiteBoard::usedTextExtended);

            connect(m_paintArea, &WhiteBoardPaintArea::colorPicked, m_toolbar, &WhiteBoardToolBar::setForegroundColor);
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
            // qCDebug(KONVERSATION_LOG) << line;
            if (line.isEmpty() || !line.contains(QLatin1Char(' ')))
                return;

            //fontname can have spaces, as well as text
            QStringList firstSplit;
            firstSplit.append(line.section(QLatin1Char(' '), 0 ,0));
            firstSplit.append(line.section(QLatin1Char(' '), 1));

            if (firstSplit.isEmpty())
                return;

            const QString ctcpCommand(firstSplit.at(0));
            // qCDebug(KONVERSATION_LOG) << "ctcpCommand" << ctcpCommand;
            // qCDebug(KONVERSATION_LOG) << "firstSize" << firstSplit.size();

            if (ctcpCommand == QLatin1String("DR") && firstSplit.size() == 2)
            {
                // DR tooltype,width,pencolor,brushcolor,x1,y1,x2,y2
                const QStringList drArgsList = firstSplit.at(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
                if (drArgsList.size() != 8)
                {
                    qCDebug(KONVERSATION_LOG) << "wrong size:" << drArgsList.size();
                    return;
                }

                bool ok = false;
                QString tmp = drArgsList.at(0);
                int toolType = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse tooltype:" << tmp;
                    return;
                }

                tmp = drArgsList.at(1);
                int lineWidth = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse linewidth:" << tmp;
                    return;
                }

                QColor penColor = parseColor(drArgsList.at(2), &ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse pencolor:" << drArgsList.at(2);
                    return;
                }

                QColor brushColor = parseColor(drArgsList.at(3), &ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse brush:" << drArgsList.at(3);
                    return;
                }

                tmp = drArgsList.at(4);
                int xFrom = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse xFrom:" << tmp;
                    return;
                }

                tmp = drArgsList.at(5);
                int yFrom = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse yFrom:" << tmp;
                    return;
                }

                tmp = drArgsList.at(6);
                int xTo = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse xTo:" << tmp;
                    return;
                }

                tmp = drArgsList.at(7);
                int yTo = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "unabled to parse yTo:" << tmp;
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
                    // qCDebug(KONVERSATION_LOG) << "drawing a line" << lineWidth << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawLine(lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::Rectangle:
                    // qCDebug(KONVERSATION_LOG) << "drawing a rectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawRectangle(lineWidth, penColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::FilledRectangle:
                    // qCDebug(KONVERSATION_LOG) << "drawing a filledrectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawFilledRectangle(lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::Ellipse:
                    // qCDebug(KONVERSATION_LOG) << "drawing a rectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawEllipse(lineWidth, penColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::FilledEllipse:
                    // qCDebug(KONVERSATION_LOG) << "drawing a filledrectangle" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->drawFilledEllipse(lineWidth, penColor, brushColor, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::Eraser:
                    // qCDebug(KONVERSATION_LOG) << "drawing a Eraser" << xFrom << yFrom << xTo << yTo;
                    m_paintArea->useEraser(lineWidth, xFrom, yFrom, xTo, yTo);
                    break;
                case WhiteBoardGlobals::FloodFill:
                    // qCDebug(KONVERSATION_LOG) << "drawing a FloodFill" << xFrom << yFrom;
                    m_paintArea->useFloodFill(xFrom, yFrom, penColor);
                    break;
                case WhiteBoardGlobals::Arrow:
                    // qCDebug(KONVERSATION_LOG) << "drawing an Arrow" << xFrom << yFrom;
                    m_paintArea->drawArrow(lineWidth, penColor, xFrom, yFrom, xTo, yTo);
                    break;
                }
            }
            else if (ctcpCommand == QLatin1String("TXT") && firstSplit.size() == 2)
            {
                QStringList txtArgsList = firstSplit.at(1).split(QLatin1Char(','), Qt::KeepEmptyParts);
                if (txtArgsList.size() < 3)
                {
                    qCDebug(KONVERSATION_LOG) << "txt wrong size:" << txtArgsList.size();
                    return;
                }

                QString tmp = txtArgsList.at(0);
                bool ok;
                int x1 = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txt unabled to parse x1:" << tmp;
                    return;
                }

                tmp = txtArgsList.at(1);
                int y1 = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txt unabled to parse y1:" << tmp;
                    return;
                }

                txtArgsList.removeFirst();
                txtArgsList.removeFirst();
                QString text(txtArgsList.join(QLatin1Char(',')));

                m_paintArea->useText(x1,y1,text);
            }
            else if (ctcpCommand == QLatin1String("TXTEX") && firstSplit.size() == 2)
            {
                QStringList txtArgsList = firstSplit.at(1).split(QLatin1Char(','), Qt::KeepEmptyParts);
                if (txtArgsList.size() < 8)
                {
                    qCDebug(KONVERSATION_LOG) << "txtex wrong size:" << txtArgsList.size();
                    return;
                }

                QString tmp = txtArgsList.at(0);
                bool ok;
                int x1 = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txtex unabled to parse x1:" << tmp;
                    return;
                }

                tmp = txtArgsList.at(1);
                int y1 = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txtex unabled to parse y1:" << tmp;
                    return;
                }

                QString fontName = txtArgsList.at(2);

                tmp = txtArgsList.at(3);
                int fontSize = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txtex unabled to parse fontsize:" << tmp;
                    return;
                }

                tmp = txtArgsList.at(4);
                int fontStyle = tmp.toInt(&ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txtex unabled to parse fontstyle:" << tmp;
                    return;
                }

                QColor penColor = parseColor(txtArgsList.at(5), &ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txtex unabled to parse pencolor:" << txtArgsList.at(5);
                    return;
                }

                QColor brushColor = parseColor(txtArgsList.at(6), &ok);
                if (!ok)
                {
                    qCDebug(KONVERSATION_LOG) << "txtex unabled to parse brush:" << txtArgsList.at(6);
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
                QString text(txtArgsList.join(QLatin1Char(',')));
                // qCDebug(KONVERSATION_LOG) << "TXTEX" << text << fontSize << fontName;
                m_paintArea->useTextExtended(x1,y1,tFont,penColor,brushColor,text);
            }
            else if (ctcpCommand == QLatin1String("CLS") && firstSplit.size() == 1)
            {
                //CLS
                m_paintArea->clear();
            }
            else if (ctcpCommand == QLatin1String("CAN") && firstSplit.size() == 2)
            {
                //TODO implement me
                const QString& can = firstSplit.at(1).toLower();
                if (can == QLatin1String("use-wb2"))
                {
                    //no we currently can't, I lied
                    emitDo(QStringLiteral("use-wb2"));
                    return;
                }
                else if (can == QLatin1String("txtex"))
                {
                    emitDo(QStringLiteral("TXTEX"));
                    m_toolbar->setSupportedTextType(WhiteBoardToolBar::ExtentedText);
                    return;
                }
                qCDebug(KONVERSATION_LOG) << "unhandled CAN" << firstSplit.at(1);
            }
            else if (ctcpCommand == QLatin1String("CANT") && firstSplit.size() == 2)
            {
                //TODO implement me
                const QString& cannot = firstSplit.at(1).toLower();
                if (cannot == QLatin1String("txtex"))
                {
                    m_toolbar->setSupportedTextType(WhiteBoardToolBar::SimpleText);
                    return;
                }
                qCDebug(KONVERSATION_LOG) << "unhandled CANT" << firstSplit.at(1);
            }
            else if (ctcpCommand == QLatin1String("DO") && firstSplit.size() == 2)
            {
                //TODO implement me
                const QString& doString = firstSplit.at(1).toLower();
                if (doString == QLatin1String("txtex"))
                {
                    m_toolbar->setSupportedTextType(WhiteBoardToolBar::ExtentedText);
                    return;
                }
            }
            else if (ctcpCommand == QLatin1String("BLT") && firstSplit.size() == 2)
            {
                // BLT x1src,y1src,x2src,y2src,xdest,ydest
                const QStringList drArgsList = firstSplit.at(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
                if (drArgsList.size() != 6)
                {
                    qCDebug(KONVERSATION_LOG) << "blt wrong size:" << drArgsList.size();
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
                    qCDebug(KONVERSATION_LOG) << "blt unabled to parse coords:" << firstSplit.at(1);
                    return;
                }

                if (x2src <= x1src || y2src <= y1src)
                {
                    qCDebug(KONVERSATION_LOG) << "blt coords invalid:" << firstSplit.at(1);
                    return;
                }
                m_paintArea->useBlt(x1src, y1src, x2src, y2src, xdest, ydest);
            }
        }

        void WhiteBoard::clear()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            m_paintArea->clear();
            const QString cls = QStringLiteral("\x01""CLS\x01");
            Q_EMIT rawWhiteBoardCommand(cls);
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
            const QString txtLineCommand = QStringLiteral("\x01""TXT %1,%2,%3\x01");

            Q_EMIT rawWhiteBoardCommand(txtLineCommand.arg(x).arg(y).arg(text));
        }

        void WhiteBoard::usedTextExtended(int x, int y, const QFont& font, const QColor& textColor, const QColor& background, const QString& text)
        {
            //TXTEX x,y,fontname,ptsize,style,textcolor,bgcolor,text
            const QString txtexLineCommand = QStringLiteral("\x01""TXTEX %1,%2,%3,%4,%5,%6,%7,%8\x01");

            QString fontname = font.family();
            QString fontSize = QString::number(font.pointSize());
            QString fontStyle = QString::number(fontToStyle(font));

            Q_EMIT rawWhiteBoardCommand(txtexLineCommand.arg(x).arg(y).arg(
                                                           fontname, fontSize, fontStyle, colorToString(textColor),
                                                           colorToString(background), text));
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
            emitCan(QStringLiteral("use-wb2"));
            emitCan(QStringLiteral("TXTEX"));
        }

        void WhiteBoard::emitDRCommand(WhiteBoardGlobals::WhiteBoardTool tool, int lineWidth,
                                       const QColor& penColor, const QColor& brushColor,
                                       int xFrom, int yFrom, int xTo, int yTo)
        {
            if (xFrom < 0 && yFrom < 0 && xTo < 0 && yTo < 0)
            {
                return;
            }

            QString drLineCommand = QStringLiteral("\x01""DR %2,%3,%4,%5,%6,%7,%8,%9\x01");
            drLineCommand = drLineCommand.arg(tool).arg(
                                          lineWidth).arg(colorToString(penColor), colorToString(brushColor)).arg(
                                          xFrom).arg(yFrom).arg(xTo).arg(yTo);
            // qCDebug(KONVERSATION_LOG) << drLineCommand;
            Q_EMIT rawWhiteBoardCommand(drLineCommand);
        }

        void WhiteBoard::emitCan(const QString& canString)
        {
            const QString can = QStringLiteral("\x01""CAN %1\x01");
            Q_EMIT rawWhiteBoardCommand(can.arg(canString));
        }

        void WhiteBoard::emitCant(const QString& cantString)
        {
            const QString cant = QStringLiteral("\x01""CANT %1\x01");
            Q_EMIT rawWhiteBoardCommand(cant.arg(cantString));
        }

        void WhiteBoard::emitDo(const QString& doString)
        {
            const QString doCommand = QStringLiteral("\x01""DO %1\x01");
            Q_EMIT rawWhiteBoardCommand(doCommand.arg(doString));
        }

        void WhiteBoard::emitDont(const QString& doNotString)
        {
            const QString doNot = QStringLiteral("\x01""DONT %1\x01"); //krazy:exclude=spelling
            Q_EMIT rawWhiteBoardCommand(doNot.arg(doNotString));
        }

    }
}

#include "moc_whiteboard.cpp"
