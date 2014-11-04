/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2010 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WHITEBOARDFONTCHOOSER_H
#define WHITEBOARDFONTCHOOSER_H

#include "ui_whiteboardfontchooserui.h"

#include "whiteboardglobals.h"

#include <QDialog>

class QCloseEvent;

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardFontChooser : public QDialog, public Ui::WhiteBoardFontChooserUi
        {
            Q_OBJECT

            public:
                explicit WhiteBoardFontChooser(QWidget* parent);
                ~WhiteBoardFontChooser();

                QFont font() const;
                int textStyle() const;

            Q_SIGNALS:
                void fontChanged(const QFont& font);

            protected:
                virtual void closeEvent(QCloseEvent* event);

            protected Q_SLOTS:
                void currentFontChanged(const QFont& font);
                void pointSizeChanged(const QString& size);

                void boldToggled(bool checked);
                void italicToggled(bool checked);
                void underlineToggled(bool checked);
                void strikeoutToggled(bool checked);

            private:
                QFont m_font;
                int m_textStyle;
        };
    }
}

#endif //WHITEBOARDFONTCHOOSER_H
