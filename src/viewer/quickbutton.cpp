/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "quickbutton.h"


QuickButton::QuickButton(const QString &label,const QString &newDefinition,QWidget* parent) :
QPushButton(label,parent)
{
    setDefinition(newDefinition);
    connect(this, &QAbstractButton::clicked, this, &QuickButton::wasClicked);
}

QuickButton::~QuickButton()
{
}

void QuickButton::wasClicked()
{
    Q_EMIT clicked(definition);
}

void QuickButton::setDefinition(const QString &newDefinition)
{
    definition=newDefinition;
}

#include "moc_quickbutton.cpp"
