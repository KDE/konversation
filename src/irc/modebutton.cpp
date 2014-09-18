/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Wed Feb 6 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "modebutton.h"


ModeButton::ModeButton(const QString& label,QWidget* parent,int newId) :
QPushButton(label,parent)
{
    id=newId;
    on=false;
    setCheckable(true);
    connect(this, &ModeButton::clicked, this, &ModeButton::wasClicked);
}

ModeButton::~ModeButton()
{
}

void ModeButton::setOn(bool state)
{
    on=state;
    QPushButton::setChecked(state);
}

void ModeButton::wasClicked()
{
    emit clicked(id,!on);
    // Keep button in old state, since we don't know if mode change will
    // eventually work. If we aren't channel operator, it won't.
    setOn(on);
}


