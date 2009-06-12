/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#include "tabs_config.h"


Tabs_Config::Tabs_Config(QWidget *parent, const char *name)
 : QWidget(parent)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    connect(kcfg_TabPlacement, SIGNAL(activated(int)), this, SLOT(toggleCheckBoxes(int)));
}

Tabs_Config::~Tabs_Config()
{
}

void Tabs_Config::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (kcfg_TabPlacement->currentIndex() == 0 || kcfg_TabPlacement->currentIndex() == 1)
    {
        kcfg_ShowTabBarCloseButton->setEnabled(true);
        kcfg_UseMaxSizedTabs->setEnabled(true);
    }
    else
    {
        kcfg_ShowTabBarCloseButton->setEnabled(false);
        kcfg_UseMaxSizedTabs->setEnabled(false);
    }
}

void Tabs_Config::toggleCheckBoxes(int activated)
{
    if (activated == 0 || activated == 1)
    {
        kcfg_ShowTabBarCloseButton->setEnabled(true);
        kcfg_UseMaxSizedTabs->setEnabled(true);
    }
    else
    {
        kcfg_ShowTabBarCloseButton->setEnabled(false);
        kcfg_UseMaxSizedTabs->setEnabled(false);
    }
}

#include "tabs_config.moc"
