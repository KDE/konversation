/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#include "awaylabel.h"

#include <QAction>

#include <QInputDialog>
#include <KLocalizedString>

AwayLabel::AwayLabel(QWidget *parent)
    : QLabel(i18n("(away)"), parent)
{
    this->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto *action = new QAction(i18n("&Unaway"),this);
    connect(action, &QAction::triggered, this, &AwayLabel::unaway);
    this->addAction(action);
    action = new QAction(i18n("&Change away message..."),this);
    connect(action, &QAction::triggered, this, &AwayLabel::changeAwayMessage);
    this->addAction(action);
}

AwayLabel::~AwayLabel()
{
}

void AwayLabel::changeAwayMessage()
{
    QString awayMessage = QInputDialog::getText(this, i18n("Change away message"),i18n("Enter new away message:"));
    if (!awayMessage.isEmpty())
        Q_EMIT awayMessageChanged(awayMessage);
}

#include "moc_awaylabel.cpp"
