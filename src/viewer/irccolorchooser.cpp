/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dialog used to add irc colors to your messages
  begin:     Wed 9 July 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#include "irccolorchooser.h"
#include "config/preferences.h"

#include <qlabel.h>
#include <qpixmap.h>

#include <klocale.h>
#include <kcombobox.h>


IRCColorChooser::IRCColorChooser(QWidget* parent)
: KDialog(parent)
{
    setButtons( KDialog::Ok|KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    setCaption( i18n("IRC Color Chooser") );
    setModal( true );
    m_ui.setupUi(mainWidget());
    initColors(m_ui.m_fgColorCBox);
    initColors(m_ui.m_bgColorCBox);
    m_ui.m_bgColorCBox->insertItem(0, i18n("None"));

    connect(m_ui.m_fgColorCBox, SIGNAL(activated(int)), this, SLOT(updatePreview()));
    connect(m_ui.m_bgColorCBox, SIGNAL(activated(int)), this, SLOT(updatePreview()));
    m_ui.m_fgColorCBox->setCurrentIndex(1);
    m_ui.m_bgColorCBox->setCurrentIndex(0);

    m_ui.m_previewLbl->setAutoFillBackground(true);

    updatePreview();
}

QString IRCColorChooser::color()
{
    QString s;
    s = "%C" + QString::number(m_ui.m_fgColorCBox->currentIndex());

    if(m_ui.m_bgColorCBox->currentIndex() > 0)
    {
        s += ',' + QString::number(m_ui.m_bgColorCBox->currentIndex() - 1);
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

    for (int i =0; i < 15; i++)
    {
        pix.fill(Preferences::self()->ircColorCode(i));
        combo->insertItem(i, QIcon(pix), QString());
    }
}

#include "irccolorchooser.moc"
