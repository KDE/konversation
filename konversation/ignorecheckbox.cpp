/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ignorecheckbox.cpp  -  description
  begin:     Die Jun 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include "ignorecheckbox.h"

IgnoreCheckBox::IgnoreCheckBox(QString name,QWidget* parent,int newId):
                QCheckBox(name,parent)
{
  id=newId;

  connect(this,SIGNAL(clicked()),
          this,SLOT(wasClicked()));
}

IgnoreCheckBox::~IgnoreCheckBox()
{
}

void IgnoreCheckBox::wasClicked()
{
  emit flagChanged(id,isChecked());
}
