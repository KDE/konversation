/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2003 Mickael Marchand <marchand@kde.org>
*/

#include "konsolepanel.h"
#include "common.h"
#include "viewcontainer.h"

#include <kdebug.h>
#include <klibloader.h>
#include <klocale.h>


KonsolePanel::KonsolePanel(QWidget *p) : ChatWindow( p ), k_part (0)
{
    setName(i18n("Konsole"));
    setType(ChatWindow::Konsole);
    KLibFactory *fact = KLibLoader::self()->factory("libkonsolepart");
    if (!fact) return;

    k_part = (KParts::ReadOnlyPart *) fact->create(this);
    if (!k_part) return;

    k_part->widget()->setFocusPolicy(QWidget::WheelFocus);
    setFocusProxy(k_part->widget());
    k_part->widget()->setFocus();

    connect(k_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
    connect(k_part, SIGNAL(receivedData(const QString&)), this, SLOT(konsoleChanged(const QString&)));
}

KonsolePanel::~KonsolePanel()
{
    kDebug() << "KonsolePanel::~KonsolePanel()" << endl;
    if ( k_part )
    {
        // make sure to prevent partDestroyed() signals from being sent
        disconnect(k_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
        delete k_part;
    }
}

QWidget* KonsolePanel::getWidget()
{
    if (k_part)
        return k_part->widget();
    else
        return 0;
}

void KonsolePanel::childAdjustFocus()
{
    if (k_part) k_part->widget()->setFocus();
}

void KonsolePanel::partDestroyed()
{
    k_part = 0;

    emit closeView(this);
}

void KonsolePanel::konsoleChanged(const QString& /* data */)
{
  activateTabNotification(Konversation::tnfSystem);
}

// #include "./konsolepanel.moc"
