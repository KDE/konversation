/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "modebutton.h"

ModeButton::ModeButton(const QString& label,QWidget* parent,int newId) :
QToolButton(parent)
{
    id=newId;
    on=false;
    setText(label);
    setCheckable(true);
    connect(this, &ModeButton::clicked, this, &ModeButton::wasClicked);
}

ModeButton::~ModeButton()
{
}

void ModeButton::setOn(bool state)
{
    on=state;
    QToolButton::setChecked(state);
}

void ModeButton::wasClicked()
{
    Q_EMIT modeClicked(id, !on);
    // Keep button in old state, since we don't know if mode change will
    // eventually work. If we aren't channel operator, it won't.
    setOn(on);
}

#include "moc_modebutton.cpp"
