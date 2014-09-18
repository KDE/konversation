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

#include "quickbutton.h"


QuickButton::QuickButton(const QString &label,const QString &newDefinition,QWidget* parent) :
QPushButton(label,parent)
{
    setDefinition(newDefinition);
    connect(this,SIGNAL (clicked()),this,SLOT (wasClicked()) );
}

QuickButton::~QuickButton()
{
}

void QuickButton::wasClicked()
{
    emit clicked(definition);
}

void QuickButton::setDefinition(const QString &newDefinition)
{
    definition=newDefinition;
}


