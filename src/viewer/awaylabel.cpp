/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#include "awaylabel.h"

#include <QAction>

#include <QInputDialog>
#include <KLocalizedString>
#include <KMessageBox>

AwayLabel::AwayLabel(QWidget *parent)
    : QLabel(i18n("(away)"), parent)
{
    this->setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *action = new QAction(i18n("&Unaway"),this);
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
        emit awayMessageChanged(awayMessage);
}


