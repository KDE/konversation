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

#ifndef MODEBUTTON_H
#define MODEBUTTON_H

#include <QToolButton>


class ModeButton : public QToolButton
{
    Q_OBJECT

        public:
        ModeButton(const QString& label,QWidget* parent,int id);
        ~ModeButton() override;

        void setOn(bool state);

    Q_SIGNALS:
        void modeClicked(int id,bool on);

    public Q_SLOTS:
        void wasClicked();

    protected:
        int id;
        bool on;
};
#endif
