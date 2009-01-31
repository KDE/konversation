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
#include "irccolorchooserui.h"
#include "config/preferences.h"

#include <qlabel.h>
#include <qpixmap.h>

#include <klocale.h>
#include <kcombobox.h>


IRCColorChooser::IRCColorChooser(QWidget* parent, const char* name)
: KDialog(parent)
{
    setButtons( KDialog::Ok|KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    setCaption( i18n("IRC Color Chooser") );
    setModal( true );
    m_view = new IRCColorChooserUI(this);
    setMainWidget(m_view);
    initColors(m_view->m_fgColorCBox);
    initColors(m_view->m_bgColorCBox);
    m_view->m_bgColorCBox->insertItem(i18n("None"), 0);

    connect(m_view->m_fgColorCBox, SIGNAL(activated(int)), this, SLOT(updatePreview()));
    connect(m_view->m_bgColorCBox, SIGNAL(activated(int)), this, SLOT(updatePreview()));
    m_view->m_fgColorCBox->setCurrentIndex(1);
    m_view->m_bgColorCBox->setCurrentIndex(0);
    updatePreview();
}

QString IRCColorChooser::color()
{
    QString s;
    s = "%C" + QString::number(m_view->m_fgColorCBox->currentItem());

    if(m_view->m_bgColorCBox->currentItem() > 0)
    {
        s += ',' + QString::number(m_view->m_bgColorCBox->currentItem() - 1);
    }

    return s;
}

void IRCColorChooser::updatePreview()
{
    QColor bgc;

    if(m_view->m_bgColorCBox->currentItem() > 0)
    {
        bgc = Preferences::self()->ircColorCode(m_view->m_bgColorCBox->currentItem() - 1);
    }
    else
    {
        bgc = Preferences::self()->color(Preferences::TextViewBackground);
    }

    m_view->m_previewLbl->setBackgroundColor(bgc);
    m_view->m_previewLbl->setPaletteForegroundColor(Preferences::self()->ircColorCode(m_view->m_fgColorCBox->currentItem()));
}

void IRCColorChooser::initColors(KComboBox* combo)
{
    QPixmap pix(width(), combo->fontMetrics().height() + 4);

    for (int i =0; i < 15; i++)
    {
        pix.fill(Preferences::self()->ircColorCode(i));
        combo->insertItem(pix, i);
    }
}

#include "irccolorchooser.moc"
