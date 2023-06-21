/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2011 Bernd Buschinski <b.buschinski@web.de>
*/

#include "dccfiledialog.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>

#include <KLocalizedString>
#include <KFileWidget>

#include <preferences.h>

DccFileDialog::DccFileDialog(QWidget* parent)
    : QDialog(parent)
{
    auto* mainLayout = new QGridLayout(this);

    m_fileWidget = new KFileWidget(QUrl(), this);
    mainLayout->addWidget(m_fileWidget, 0, 0);

    m_fileWidget->okButton()->show();
    connect(m_fileWidget->okButton(), &QPushButton::clicked, m_fileWidget, &KFileWidget::slotOk);
    connect(m_fileWidget, &KFileWidget::accepted, m_fileWidget, &KFileWidget::accept);
    connect(m_fileWidget, &KFileWidget::accepted, this, &QDialog::accept);
    m_fileWidget->cancelButton()->show();
    connect(m_fileWidget->cancelButton(), &QPushButton::clicked, this, &QDialog::reject);

    m_checkBox = new QCheckBox(i18nc("passive dcc send", "Passive Send"));
    m_fileWidget->setCustomWidget(m_checkBox);

    m_checkBox->setCheckState(Preferences::self()->dccPassiveSend() ? Qt::Checked : Qt::Unchecked);
}

QList<QUrl> DccFileDialog::getOpenUrls(const QUrl &startDir, const QString& caption)
{
    KFileWidget::setStartDir(startDir);

    m_fileWidget->setOperationMode( KFileWidget::Opening );

    m_fileWidget->setMode( KFile::Files | KFile::ExistingOnly );

    setWindowTitle(caption.isEmpty() ? i18n("Open") : caption);

    exec();
    return m_fileWidget->selectedUrls();
}

bool DccFileDialog::passiveSend() const
{
    return m_checkBox->isChecked();
}

QSize DccFileDialog::sizeHint() const
{
    return m_fileWidget->dialogSizeHint();
}

#include "moc_dccfiledialog.cpp"
