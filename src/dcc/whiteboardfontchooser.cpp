/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboardfontchooser.h"

#include <QCloseEvent>

#include <KComboBox>
#include <KFontComboBox>

namespace Konversation
{
    namespace DCC
    {
        WhiteBoardFontChooser::WhiteBoardFontChooser(QWidget* parent)
            : QDialog(parent),
              m_textStyle(0)
        {
            setupUi(this);

            m_font = m_fontComboBox->currentFont();
            m_fontPointComboBox->setValidator(new QIntValidator(2,300,m_fontPointComboBox));
            pointSizeChanged(m_fontPointComboBox->currentText());

            m_boldPushButton->setIcon(KIcon("format-text-bold"));
            m_italicPushButton->setIcon(KIcon("format-text-italic"));
            m_strikeoutPushButton->setIcon(KIcon("format-text-strikethrough"));
            m_underlinePushButton->setIcon(KIcon("format-text-underline"));

            connect(m_boldPushButton, SIGNAL(toggled(bool)),
                    this, SLOT(boldToggled(bool)));
            connect(m_italicPushButton, SIGNAL(toggled(bool)),
                    this, SLOT(italicToggled(bool)));
            connect(m_strikeoutPushButton, SIGNAL(toggled(bool)),
                    this, SLOT(strikeoutToggled(bool)));
            connect(m_underlinePushButton, SIGNAL(toggled(bool)),
                    this, SLOT(underlineToggled(bool)));

            connect(m_fontComboBox, SIGNAL(currentFontChanged(QFont)),
                    this, SLOT(currentFontChanged(QFont)));
            connect(m_fontPointComboBox, SIGNAL(currentIndexChanged(QString)),
                    this, SLOT(pointSizeChanged(QString)));
        }

        WhiteBoardFontChooser::~WhiteBoardFontChooser()
        {
        }

        QFont WhiteBoardFontChooser::font() const
        {
            return m_font;
        }

        int WhiteBoardFontChooser::textStyle() const
        {
            return m_textStyle;
        }

        void WhiteBoardFontChooser::closeEvent(QCloseEvent* event)
        {
            //QWidget::closeEvent(event);
            event->ignore();
            hide();
        }

        void WhiteBoardFontChooser::currentFontChanged(const QFont& font)
        {
            m_font.setFamily(font.family());
            emit fontChanged(m_font);
        }

        void WhiteBoardFontChooser::pointSizeChanged(const QString& size)
        {
            bool ok;
            int pointSize = size.toInt(&ok);
            if (!ok)
            {
                return;
            }
            m_font.setPointSize(pointSize);
            emit fontChanged(m_font);
        }

        void WhiteBoardFontChooser::boldToggled(bool checked)
        {
            if (checked)
            {
                m_textStyle |= WhiteBoardGlobals::Bold;
            }
            else
            {
                m_textStyle &= ~WhiteBoardGlobals::Bold;
            }
            m_font.setBold(checked);
            emit fontChanged(m_font);
        }

        void WhiteBoardFontChooser::italicToggled(bool checked)
        {
            if (checked)
            {
                m_textStyle |= WhiteBoardGlobals::Italic;
            }
            else
            {
                m_textStyle &= ~WhiteBoardGlobals::Italic;
            }
            m_font.setItalic(checked);
            emit fontChanged(m_font);
        }

        void WhiteBoardFontChooser::underlineToggled(bool checked)
        {
            if (checked)
            {
                m_textStyle |= WhiteBoardGlobals::Underline;
            }
            else
            {
                m_textStyle &= ~WhiteBoardGlobals::Underline;
            }
            m_font.setUnderline(checked);
            emit fontChanged(m_font);
        }

        void WhiteBoardFontChooser::strikeoutToggled(bool checked)
        {
            if (checked)
            {
                m_textStyle |= WhiteBoardGlobals::Strikeout;
            }
            else
            {
                m_textStyle &= ~WhiteBoardGlobals::Strikeout;
            }
            m_font.setStrikeOut(checked);
            emit fontChanged(m_font);
        }
    }
}
