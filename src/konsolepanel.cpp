/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Mickael Marchand <marchand@kde.org>
*/

#include "konsolepanel.h"

#include "common.h"
#include "viewcontainer.h"
#include "konversation_log.h"

#include <KLocalizedString>
#include <KParts/PartLoader>
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

    TerminalInterface *terminal = nullptr;
    KPluginMetaData part(QStringLiteral("kf6/parts/konsolepart"));
    int valid = part.isValid();

    setContentsMargins(0, 0, 0, 0);

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    // TODO determine if instantiation can actually fail if the metadata was retrieved
    if (valid)
    {
        k_part = KParts::PartLoader::instantiatePart<KParts::ReadOnlyPart>(part, m_headerSplitter).plugin;
        if (k_part)
        {
            terminal = qobject_cast<TerminalInterface *>(k_part);
            if (!terminal) // so we got a kpart with the right name but its not a TerminalInterface? How likely is this?
                valid = -2;
        }
        else
            valid = -1;
    }

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

    // TODO rather than abusing the button icon and label, lets have proper messages
    if (valid <1)
    {
        m_profileButton->setIconSize(QSize(64,64));
        m_profileButton->setEnabled(false);
        m_konsoleLabel->setNum(valid);
        m_profileButton->setIcon(QIcon::fromTheme(QStringLiteral("error")));
        return;
    }

    m_headerSplitter->addWidget(k_part->widget());
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(k_part->widget()), 1);
    k_part->widget()->setFocusPolicy(Qt::WheelFocus);
    setFocusProxy(k_part->widget());
    k_part->widget()->setFocus();

    connect(k_part, &KParts::ReadOnlyPart::setWindowCaption, m_konsoleLabel, &QLabel::setText);

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
