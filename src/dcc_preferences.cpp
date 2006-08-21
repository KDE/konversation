/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#include <qcombobox.h>
#include <qlineedit.h>
#include <klocale.h>

#include "dcc_preferences.h"

DCC_Config::DCC_Config(QWidget *parent, const char* name) :
  DCC_ConfigUI(parent,name)
{
    languageChange();
    connect(kcfg_DccMethodToGetOwnIp, SIGNAL(activated(int)), this, SLOT(dccMethodChanged(int)));          dccMethodChanged(kcfg_DccMethodToGetOwnIp->currentItem()); 


}

void DCC_Config::show()
{
    QWidget::show();

    kcfg_DccSpecificOwnIp->setEnabled(kcfg_DccMethodToGetOwnIp->currentItem() == 2);
}

void DCC_Config::dccMethodChanged(int index)
{
    kcfg_DccSpecificOwnIp->setEnabled( index == 2 ); 
}

void DCC_Config::languageChange()
{
    kcfg_DccMethodToGetOwnIp->clear();
    kcfg_DccMethodToGetOwnIp->insertItem(i18n("Network Interface"));
    kcfg_DccMethodToGetOwnIp->insertItem(i18n("Reply From IRC Server"));
    kcfg_DccMethodToGetOwnIp->insertItem(i18n("Specify Manually"));

}

DCC_Config::~DCC_Config()
{
}
#include "dcc_preferences.moc"

