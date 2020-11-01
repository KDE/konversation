/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2010 Bernd Buschinski <b.buschinski@web.de>
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
                ~WhiteBoardFontChooser() override;

                QFont font() const;
                int textStyle() const;

            Q_SIGNALS:
                void fontChanged(const QFont& font);

            protected:
                void closeEvent(QCloseEvent* event) override;

            private Q_SLOTS:
                void currentFontChanged(const QFont& font);
                void pointSizeChanged(int fontPointListIndex);

                void boldToggled(bool checked);
                void italicToggled(bool checked);
                void underlineToggled(bool checked);
                void strikeoutToggled(bool checked);

            private:
                QFont m_font;
                int m_textStyle;

                Q_DISABLE_COPY(WhiteBoardFontChooser)
        };
    }
}

#endif //WHITEBOARDFONTCHOOSER_H
