/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WHITEBOARDPAINTAREA_H
#define WHITEBOARDPAINTAREA_H

#include <QWidget>
#include <QPoint>
#include <QColor>

#include "whiteboardglobals.h"

class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class QPixmap;

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardPaintArea : public QWidget
        {
            Q_OBJECT

        public:
            WhiteBoardPaintArea(QWidget* parent = 0);

        public slots:
            void setTool(WhiteBoardGlobals::WhiteBoardTool tool);
            void setForegroundColor(const QColor& color);
            void setBackgroundColor(const QColor& color);
            void setPenWidth(qreal width);


        protected:
            virtual void paintEvent(QPaintEvent * event);
            virtual void resizeEvent(QResizeEvent * event);
            virtual void mousePressEvent(QMouseEvent * event);
            virtual void mouseReleaseEvent(QMouseEvent* event);
            virtual void mouseMoveEvent(QMouseEvent * event);

        private:
            QPixmap* m_imagePixmap;
            QPixmap* m_overlayPixmap;

            bool m_mousePressed;
            QPoint m_lastPos;

            WhiteBoardGlobals::WhiteBoardTool m_tool;

            QColor m_foregroundColor;
            QColor m_backgroundColor;

            qreal m_penWidth;
        };
    }
}

#endif // WHITEBOARDPAINTAREA_H
