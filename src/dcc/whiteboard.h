/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
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
                WhiteBoard(QWidget* parent);
                ~WhiteBoard();

                static QStringList whiteboardCommands();

            public slots:
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

            signals:
                void rawWhiteBoardCommand(const QString& command);

            private:
                inline QColor parseColor(const QString& colorString, bool* ok = 0);
                inline QString colorToString(const QColor& color);
                inline int fontToStyle(const QFont& font); 

                inline void emitDRCommand(WhiteBoardGlobals::WhiteBoardTool tool, int lineWidth,
                                         const QColor& penColor, const QColor& brushColor,
                                         int xFrom, int yFrom, int xTo, int yTo);

                inline void emitCan(const QString& canString);
                inline void emitCant(const QString& cantString);
                inline void emitDo(const QString& doString);
                inline void emitDont(const QString& doNotString);

                WhiteBoardToolBar* m_toolbar;
                WhiteBoardPaintArea* m_paintArea;
        };
    }
}

#endif // WHITEBOARD_H
