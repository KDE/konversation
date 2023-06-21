/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboardfontchooser.h"

#include <QCloseEvent>

#include <KComboBox>

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
            pointSizeChanged(m_fontPointComboBox->currentIndex());

            m_boldPushButton->setIcon(QIcon::fromTheme(QStringLiteral("format-text-bold")));
            m_italicPushButton->setIcon(QIcon::fromTheme(QStringLiteral("format-text-italic")));
            m_strikeoutPushButton->setIcon(QIcon::fromTheme(QStringLiteral("format-text-strikethrough")));
            m_underlinePushButton->setIcon(QIcon::fromTheme(QStringLiteral("format-text-underline")));

            connect(m_boldPushButton, &QToolButton::toggled, this, &WhiteBoardFontChooser::boldToggled);
            connect(m_italicPushButton, &QToolButton::toggled, this, &WhiteBoardFontChooser::italicToggled);
            connect(m_strikeoutPushButton, &QToolButton::toggled, this, &WhiteBoardFontChooser::strikeoutToggled);
            connect(m_underlinePushButton, &QToolButton::toggled, this, &WhiteBoardFontChooser::underlineToggled);

            connect(m_fontComboBox, &QFontComboBox::currentFontChanged, this, &WhiteBoardFontChooser::currentFontChanged);
            connect(m_fontPointComboBox, QOverload<int>::of(&KComboBox::currentIndexChanged),
                    this, &WhiteBoardFontChooser::pointSizeChanged);
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
            Q_EMIT fontChanged(m_font);
        }

        void WhiteBoardFontChooser::pointSizeChanged(int fontPointListIndex)
        {
            const QString size = m_fontPointComboBox->itemText(fontPointListIndex);

            bool ok;
            int pointSize = size.toInt(&ok);
            if (!ok)
            {
                return;
            }
            m_font.setPointSize(pointSize);
            Q_EMIT fontChanged(m_font);
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
            Q_EMIT fontChanged(m_font);
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
            Q_EMIT fontChanged(m_font);
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
            Q_EMIT fontChanged(m_font);
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
            Q_EMIT fontChanged(m_font);
        }
    }
}

#include "moc_whiteboardfontchooser.cpp"
