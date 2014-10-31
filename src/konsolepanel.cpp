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

#include <QSplitter>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>

#include <QDebug>
#include <QIcon>
#include <KLocalizedString>
#include <KService>
#include <kde_terminal_interface.h>


KonsolePanel::KonsolePanel(QWidget *p) : ChatWindow( p ), k_part (0)
{
    setName(i18n("Konsole"));
    setType(ChatWindow::Konsole);

    setMargin(0);

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    QWidget* headerWidget = new QWidget(m_headerSplitter);
    QHBoxLayout* headerWidgetLayout = new QHBoxLayout(headerWidget);
    headerWidgetLayout->setMargin(0);
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(headerWidget), 0);

    m_profileButton = new QToolButton(headerWidget);
    headerWidgetLayout->addWidget(m_profileButton);
    m_profileButton->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    m_profileButton->setToolTip(i18n("Manage Konsole Profiles"));
    m_profileButton->setAutoRaise(true);
    m_profileButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    connect(m_profileButton, &QToolButton::clicked, this, &KonsolePanel::manageKonsoleProfiles);

    m_konsoleLabel = new QLabel(headerWidget);
    headerWidgetLayout->addWidget(m_konsoleLabel);
    m_konsoleLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));

    KPluginFactory* fact = 0;
    KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("konsolepart"));
    if( service )
    {
        fact = KPluginLoader(service->library()).factory();
    }

    if (!fact) return;

    k_part = fact->create<KParts::ReadOnlyPart>(m_headerSplitter);
    if (!k_part) return;

    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(k_part->widget()), 1);
    k_part->widget()->setFocusPolicy(Qt::WheelFocus);
    setFocusProxy(k_part->widget());
    k_part->widget()->setFocus();

    connect(k_part, &KParts::ReadOnlyPart::setWindowCaption, m_konsoleLabel, &QLabel::setText);

    TerminalInterface *terminal = qobject_cast<TerminalInterface *>(k_part);
    if (!terminal) return;
    terminal->showShellInDir(QDir::homePath());

    connect(k_part, &KParts::ReadOnlyPart::destroyed, this, &KonsolePanel::partDestroyed);
#if 0
// TODO find the correct signal
    connect(k_part, &KParts::ReadOnlyPart::receivedData, this, &KonsolePanel::konsoleChanged);
#endif
}

KonsolePanel::~KonsolePanel()
{
    qDebug();
    if ( k_part )
    {
        // make sure to prevent partDestroyed() signals from being sent
        disconnect(k_part, &KParts::ReadOnlyPart::destroyed, this, &KonsolePanel::partDestroyed);
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

void KonsolePanel::manageKonsoleProfiles()
{
    QMetaObject::invokeMethod(k_part, "showManageProfilesDialog",
        Qt::QueuedConnection, Q_ARG(QWidget*, QApplication::activeWindow()));
}

void KonsolePanel::konsoleChanged(const QString& /* data */)
{
  activateTabNotification(Konversation::tnfSystem);
}


