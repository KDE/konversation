/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WHITEBOARD_H
#define WHITEBOARD_H

#include <QWidget>

#include "whiteboardglobals.h"

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardToolBar;
        class WhiteBoardPaintArea;

        class WhiteBoard : public QWidget
        {
            Q_OBJECT

            public:
                explicit WhiteBoard(QWidget* parent);
                ~WhiteBoard() override;

                static QStringList whiteboardCommands();

            public Q_SLOTS:
                void receivedWhiteBoardLine(const QString& line);
                void connected();

                void clear();

                void drawedPencil(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                  int xFrom, int yFrom, int xTo, int yTo);
                void drawedLine(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                int xFrom, int yFrom, int xTo, int yTo);
                void drawedRectangle(int lineWidth, const QColor& penColor,
                                     int xFrom, int yFrom, int xTo, int yTo);
                void drawedFilledRectangle(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                           int xFrom, int yFrom, int xTo, int yTo);
                void drawedEllipse(int lineWidth, const QColor& penColor,
                                   int xFrom, int yFrom, int xTo, int yTo);
                void drawedFilledEllipse(int lineWidth, const QColor& penColor, const QColor& brushColor,
                                         int xFrom, int yFrom, int xTo, int yTo);
                void drawedArrow(int lineWidth, const QColor& penColor,
                                 int xFrom, int yFrom, int xTo, int yTo);
                void usedEraser(int lineWidth, int xFrom, int yFrom, int xTo, int yTo);
                void usedFloodFill(int x, int y, const QColor& color);

                void usedText(int x, int y, const QString& text);
                void usedTextExtended(int x, int y, const QFont& font, const QColor& textColor, const QColor& background, const QString& text);

            Q_SIGNALS:
                void rawWhiteBoardCommand(const QString& command);

            private:
                static QColor parseColor(const QString& colorString, bool* ok = nullptr);
                static QString colorToString(const QColor& color);
                static int fontToStyle(const QFont& font);

                inline void emitDRCommand(WhiteBoardGlobals::WhiteBoardTool tool, int lineWidth,
                                         const QColor& penColor, const QColor& brushColor,
                                         int xFrom, int yFrom, int xTo, int yTo);

                inline void emitCan(const QString& canString);
                inline void emitCant(const QString& cantString);
                inline void emitDo(const QString& doString);
                inline void emitDont(const QString& doNotString);

            private:
                WhiteBoardToolBar* m_toolbar;
                WhiteBoardPaintArea* m_paintArea;

                Q_DISABLE_COPY(WhiteBoard)
        };
    }
}

#endif // WHITEBOARD_H
