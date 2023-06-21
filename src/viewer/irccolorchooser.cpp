/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Peter Simonsson <psn@linux.se>
*/

#include "irccolorchooser.h"
#include "config/preferences.h"

#include <QPixmap>

#include <KComboBox>
#include <KLocalizedString>

IRCColorChooser::IRCColorChooser(QWidget* parent)
: QDialog(parent)
{
    setWindowTitle( i18n("IRC Color Chooser") );
    setModal( true );
    m_ui.setupUi(this);
    initColors(m_ui.m_fgColorCBox);
    initColors(m_ui.m_bgColorCBox);
    m_ui.m_bgColorCBox->insertItem(0, i18n("None"));

    connect(m_ui.m_fgColorCBox, QOverload<int>::of(&KComboBox::activated), this, &IRCColorChooser::updatePreview);
    connect(m_ui.m_bgColorCBox, QOverload<int>::of(&KComboBox::activated), this, &IRCColorChooser::updatePreview);
    m_ui.m_fgColorCBox->setCurrentIndex(1);
    m_ui.m_bgColorCBox->setCurrentIndex(0);

    m_ui.m_previewLbl->setAutoFillBackground(true);

    connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &IRCColorChooser::accept);
    connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &IRCColorChooser::reject);

    updatePreview();
}

QString IRCColorChooser::color() const
{
    QString s;
    s = QLatin1String("%C") + QString::number(m_ui.m_fgColorCBox->currentIndex()).rightJustified(2, QLatin1Char('0'));

    if(m_ui.m_bgColorCBox->currentIndex() > 0)
    {
        s += QLatin1Char(',') + QString::number(m_ui.m_bgColorCBox->currentIndex() - 1).rightJustified(2, QLatin1Char('0'));
    }

    return s;
}

void IRCColorChooser::updatePreview()
{
    QColor bgc;

    if(m_ui.m_bgColorCBox->currentIndex() > 0)
    {
        bgc = Preferences::self()->ircColorCode(m_ui.m_bgColorCBox->currentIndex() - 1);
    }
    else
    {
        bgc = Preferences::self()->color(Preferences::TextViewBackground);
    }

    QPalette p = m_ui.m_previewLbl->palette();
    p.setColor(backgroundRole(), bgc);
    p.setColor(foregroundRole(), Preferences::self()->ircColorCode(m_ui.m_fgColorCBox->currentIndex()));
    m_ui.m_previewLbl->setPalette(p);
}

void IRCColorChooser::initColors(KComboBox* combo)
{
    combo->setIconSize(QSize(combo->width(), combo->fontMetrics().height()));

    QPixmap pix(combo->width(), combo->fontMetrics().height());

    for (int i =0; i < 16; i++)
    {
        pix.fill(Preferences::self()->ircColorCode(i));
        combo->insertItem(i, QIcon(pix), QString());
    }
}

#include "moc_irccolorchooser.cpp"
