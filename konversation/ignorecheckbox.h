/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ignorecheckbox.h  -  description
  begin:     Die Jun 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef IGNORECHECKBOX_H
#define IGNORECHECKBOX_H

#include <qcheckbox.h>

/*
  @author Dario Abatianni
*/

class IgnoreCheckBox : public QCheckBox
{
  Q_OBJECT

  public:
    IgnoreCheckBox(QString name,QWidget* parent,int newId);
    ~IgnoreCheckBox();

  signals:
    void flagChanged(int flag,bool active);

  protected slots:
    void wasClicked();

  protected:
    int id;
};

#endif
