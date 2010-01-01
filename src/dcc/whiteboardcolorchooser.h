/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WHITEBOARDCOLORCHOOSER_H
#define WHITEBOARDCOLORCHOOSER_H

#include <QFrame>
#include <QColor>
#include <QRect>
#include <QPixmap>

class QPaintEvent;
class QMouseEvent;

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardColorChooser : public QFrame
        {
            Q_OBJECT

        public:
            enum ColorLayer
            {
                None,
                ForegroundColor,
                BackgroundColor
            };

            WhiteBoardColorChooser(QWidget* parent = 0);
            ~WhiteBoardColorChooser();

            QColor color(const ColorLayer& layer) const;
            QColor foregroundColor() const;
            QColor backgroundColor() const;

        public slots:
            void setColor(ColorLayer layer, const QColor& color);
            void setForegroundColor(const QColor& color);
            void setBackgroundColor(const QColor& color);

        signals:
            void colorsSwapped(const QColor& newForegroundColor,
                               const QColor& newBackgroundColor);
            void foregroundColorChanged(const QColor& color);
            void backgroundColorChanged(const QColor& color);

        protected:
            virtual void mouseReleaseEvent(QMouseEvent *e);
            virtual void paintEvent(QPaintEvent *e);
            virtual void resizeEvent(QResizeEvent *e);

        private:
            inline QRect swapPixmapRect() const;
            inline QRect foregroundBackgroundRect() const;
            inline QRect foregroundRect() const;
            inline QRect backgroundRect() const;

            inline void drawSwapPixmap();

            QColor m_foregroundColor;
            QColor m_backgroundColor;

            QPixmap m_swapPixmap;
        };
    }
}

#endif // WHITEBOARDCOLORCHOOSER_H
