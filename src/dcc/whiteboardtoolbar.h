/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WHITEBOARDTOOLBAR_H
#define WHITEBOARDTOOLBAR_H

#include <QWidget>
#include <QHash>

#include "ui_whiteboardtoolbarui.h"

#include "whiteboardglobals.h"

class QPushButton;

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardFontChooser;

        class WhiteBoardToolBar : public QWidget, public Ui::WhiteBoardToolBarUi
        {
            Q_OBJECT

            public:

                enum TextType
                {
                    None = 0,
                    SimpleText,
                    ExtentedText
                };

                explicit WhiteBoardToolBar(QWidget* parent);
                ~WhiteBoardToolBar() override;

                QColor foregroundColor() const;
                QColor backgroundColor() const;

            public Q_SLOTS:
                void enableTool(Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool);
                void disableTool(Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool);

                void setSupportedTextType(Konversation::DCC::WhiteBoardToolBar::TextType textType);
                Konversation::DCC::WhiteBoardToolBar::TextType textType() const;

                void setForegroundColor(const QColor& foregroundColor);
                void setBackgroundColor(const QColor& backgroundColor);

            Q_SIGNALS:
                void toolChanged(Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool);

            // colorchooser signals
                void colorsSwapped (const QColor& newForegroundColor,
                                    const QColor& newBackgroundColor);
                void foregroundColorChanged(const QColor& color);
                void backgroundColorChanged(const QColor& color);
            // colorchooser signals end

            // fontchooser
                void fontChanged(const QFont& font);
            // fontchooser end

                void lineWidthChanged(int width);

                void save(const QString& filename);
                void clear();

            private Q_SLOTS:
                void clearClicked();
                void saveClicked();

                void pencilToggled(bool checked);
                void lineToggled(bool checked);
                void rectangleToggled(bool checked);
                void ellipseToggled(bool checked);
                void textToggled(bool checked);
                void selectionToggled(bool checked);
                void eraseToggled(bool checked);
                void fillToggled(bool checked);
                void arrowToggled(bool checked);
                void colorPickerToggled(bool checked);

                void updateLineWidthPixmap(int lineWidth);

                void formSelectionChanged();

            private:
                enum FormOption
                {
                    Rectangle,
                    Ellipse
                };

                inline void connectToggleButtons();
                inline void disconnectToggleButtons();

                inline void handleToggleButton(QPushButton* button, bool checked, Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool);
                inline void unCheckOtherButtons(QPushButton* button);

                inline void setLineWidthVisible(bool visible);
                inline void setFormOptionVisible(bool visible);
                inline void setFontDialogVisible(bool visible);

                inline void fillFormOptionList(FormOption form);

            private:
                QHash<Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool, QPushButton*> m_toggleButtonHash;
                QPixmap m_lineWidthPixmap;

                QPixmap m_rectanglePixmap;
                QPixmap m_filledRectanglePixmap;

                QPixmap m_ellipsePixmap;
                QPixmap m_filledEllipsePixmap;

                Konversation::DCC::WhiteBoardToolBar::TextType m_textType;
                WhiteBoardFontChooser* m_fontDialog;

                Q_DISABLE_COPY(WhiteBoardToolBar)
        };
    }
}

#endif // WHITEBOARDTOOLBAR_H
