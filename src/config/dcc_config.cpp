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

#include "dcc_config.h"

#include <qcombobox.h>
#include <qlineedit.h>
#include <klocale.h>


DCC_Config::DCC_Config(QWidget *parent, const char* name) :
  QWidget(parent)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    kcfg_DccPath->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);

    languageChange();
    connect(kcfg_DccMethodToGetOwnIp, SIGNAL(activated(int)), this, SLOT(dccMethodChanged(int)));
    dccMethodChanged(kcfg_DccMethodToGetOwnIp->currentIndex()); 


}

void DCC_Config::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    kcfg_DccSpecificOwnIp->setEnabled(kcfg_DccMethodToGetOwnIp->currentIndex() == 2);
}

void DCC_Config::dccMethodChanged(int index)
{
    kcfg_DccSpecificOwnIp->setEnabled( index == 2 ); 
}

void DCC_Config::languageChange()
{
    kcfg_DccMethodToGetOwnIp->clear();
    kcfg_DccMethodToGetOwnIp->addItem(i18n("Network Interface"));
    kcfg_DccMethodToGetOwnIp->addItem(i18n("Reply From IRC Server"));
    kcfg_DccMethodToGetOwnIp->addItem(i18n("Specify Manually"));

}

DCC_Config::~DCC_Config()
{
}

#include "dcc_config.moc"
