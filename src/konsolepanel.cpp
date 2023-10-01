/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Mickael Marchand <marchand@kde.org>
*/

#include "konsolepanel.h"

#include "common.h"
#include "viewcontainer.h"
#include "konversation_log.h"

#include <KLocalizedString>
#include <KService>
#include <KPluginFactory>
#include <kde_terminal_interface.h>

#include <QSplitter>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QIcon>


KonsolePanel::KonsolePanel(QWidget *p) : ChatWindow( p ), k_part (nullptr)
{
    setName(i18n("Konsole"));
    setType(ChatWindow::Konsole);

    setContentsMargins(0, 0, 0, 0);

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    auto* headerWidget = new QWidget(m_headerSplitter);
    auto* headerWidgetLayout = new QHBoxLayout(headerWidget);
    headerWidgetLayout->setContentsMargins(0, 0, 0, 0);
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

    const QString konsolePart = QStringLiteral("kf6/parts/konsolepart");
    KPluginFactory *factory = KPluginFactory::loadFactory(konsolePart).plugin;
    if (!factory) {
        return;
    }

    k_part = factory->create<KParts::ReadOnlyPart>(m_headerSplitter);

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
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    if ( k_part )
    {
        // make sure to prevent partDestroyed() signals from being sent
        disconnect(k_part, &KParts::ReadOnlyPart::destroyed, this, &KonsolePanel::partDestroyed);
        delete k_part;
    }
}

QWidget* KonsolePanel::getWidget() const
{
    if (k_part)
        return k_part->widget();
    else
        return nullptr;
}

void KonsolePanel::childAdjustFocus()
{
    if (k_part) k_part->widget()->setFocus();
}

void KonsolePanel::partDestroyed()
{
    k_part = nullptr;

    Q_EMIT closeView(this);
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

#include "moc_konsolepanel.cpp"
