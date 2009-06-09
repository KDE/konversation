/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#ifndef DCC_CONFIG_H
#define DCC_CONFIG_H

#include "ui_dcc_configui.h"

class DCC_Config : public QWidget, private Ui::DCC_ConfigUI
{
    Q_OBJECT

    public:
        DCC_Config(QWidget* parent, const char* name);
        ~DCC_Config();

    protected slots:
        virtual void languageChange();
        void dccMethodChanged(int index);
        void dccUPnPChanged(int state);

    protected:
        virtual void showEvent(QShowEvent *event);
};

#endif
