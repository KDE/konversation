/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  quickbutton.h  -  description
  begin:     Wed Feb 6 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef QUICKBUTTON_H
#define QUICKBUTTON_H

#include <qpushbutton.h>

/*
  @author Dario Abatianni
*/

class QuickButton : public QPushButton
{
  Q_OBJECT

  public:
    QuickButton(QString label,QString newDefinition,QWidget* parent);
    ~QuickButton();

    void setDefinition(QString newDefinition);
    
  signals:
    void clicked(int);
    void clicked(const QString &definition);

  public slots:
    void wasClicked();

  protected:
    int id;

    QString definition;
};

#endif
