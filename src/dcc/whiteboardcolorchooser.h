/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
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

            explicit WhiteBoardColorChooser(QWidget* parent = nullptr);
            ~WhiteBoardColorChooser() override;

            QColor color(const ColorLayer& layer) const;
            QColor foregroundColor() const;
            QColor backgroundColor() const;

        public Q_SLOTS:
            void setColor(ColorLayer layer, const QColor& color);
            void setForegroundColor(const QColor& color);
            void setBackgroundColor(const QColor& color);

        Q_SIGNALS:
            void colorsSwapped(const QColor& newForegroundColor,
                               const QColor& newBackgroundColor);
            void foregroundColorChanged(const QColor& color);
            void backgroundColorChanged(const QColor& color);

        protected:
            void mouseReleaseEvent(QMouseEvent *e) override;
            void paintEvent(QPaintEvent *e) override;
            void resizeEvent(QResizeEvent *e) override;

        private:
            inline QRect swapPixmapRect() const;
            inline QRect foregroundBackgroundRect() const;
            inline QRect foregroundRect() const;
            inline QRect backgroundRect() const;

            inline void drawSwapPixmap();

        private:
            QColor m_foregroundColor;
            QColor m_backgroundColor;

            QPixmap m_swapPixmap;

            Q_DISABLE_COPY(WhiteBoardColorChooser)
        };
    }
}

#endif // WHITEBOARDCOLORCHOOSER_H
