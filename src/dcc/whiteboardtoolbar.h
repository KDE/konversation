/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WHITEBOARDTOOLBAR_H
#define WHITEBOARDTOOLBAR_H

#include <QWidget>
#include <QHash>

#include "ui_whiteboardtoolbarui.h"

#include "whiteboardglobals.h"

class KPushButton;

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

                WhiteBoardToolBar(QWidget* parent);
                ~WhiteBoardToolBar();

                QColor foregroundColor() const;
                QColor backgroundColor() const;

            public slots:
                void enableTool(Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool);
                void disableTool(Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool);

                void setSupportedTextType(Konversation::DCC::WhiteBoardToolBar::TextType textType);
                Konversation::DCC::WhiteBoardToolBar::TextType textType() const;

                void setForegroundColor(const QColor& foregroundColor);
                void setBackgroundColor(const QColor& backgroundColor);

            signals:
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

            protected slots:
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

                inline void handleToggleButton(KPushButton* button, bool checked, Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool);
                inline void unCheckOtherButtons(KPushButton* button);

                inline void setLineWidthVisible(bool visible);
                inline void setFormOptionVisible(bool visible);
                inline void setFontDialogVisible(bool visible);

                inline void fillFormOptionList(FormOption form);

                QHash<Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool, KPushButton*> m_toggleButtonHash;
                QPixmap m_lineWidthPixmap;

                QPixmap m_rectanglePixmap;
                QPixmap m_filledRectanglePixmap;

                QPixmap m_ellipsePixmap;
                QPixmap m_filledEllipsePixmap;

                Konversation::DCC::WhiteBoardToolBar::TextType m_textType;
                WhiteBoardFontChooser* m_fontDialog;
        };
    }
}

#endif // WHITEBOARDTOOLBAR_H
