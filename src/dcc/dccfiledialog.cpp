/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2011 Bernd Buschinski <b.buschinski@web.de>
*/

#include "dccfiledialog.h"

#include <QCheckBox>

#include <KLocalizedString>
#include <KDebug>
#include <kabstractfilewidget.h>

#include <preferences.h>

DccFileDialog::DccFileDialog(const KUrl& startDir, const QString& filter, QWidget* parent, QWidget* widget)
    : KFileDialog(startDir, filter, parent, widget)
{
    m_checkBox = new QCheckBox(i18nc("passive dcc send", "Passive Send"));
    fileWidget()->setCustomWidget(m_checkBox);
    
    m_checkBox->setCheckState(Preferences::self()->dccPassiveSend() ? Qt::Checked : Qt::Unchecked);
}

KUrl::List DccFileDialog::getOpenUrls(const KUrl& startDir, const QString& filter, const QString& caption)
{
    setStartDir(startDir);
    setFilter(filter);
    setOperationMode( KFileDialog::Opening );
    setMode( KFile::Files | KFile::ExistingOnly );
    setCaption(caption.isEmpty() ? i18n("Open") : caption);

    exec();
    return selectedUrls();
}

bool DccFileDialog::passiveSend()
{
    return m_checkBox->isChecked();
}
