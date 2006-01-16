
// (C)2003 Mickael Marchand <marchand@kde.org>

#include <kdebug.h>
#include <klibloader.h>
#include <klocale.h>

#include "konsolepanel.h"
#include "common.h"

KonsolePanel::KonsolePanel(QWidget *p) : ChatWindow( p )
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
    kdDebug() << "KonsolePanel::~KonsolePanel()" << endl;
    if ( k_part )
    {
        // make sure to prevent partDestroyed() signals from being sent
        disconnect(k_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
        delete k_part;
    }
}

void KonsolePanel::childAdjustFocus()
{
}

void KonsolePanel::partDestroyed()
{
    k_part = 0;
    // tell the main window to delete us
    emit deleted(this);
}

void KonsolePanel::konsoleChanged(const QString& /* data */)
{
  activateTabNotification(Konversation::tnfNormal);
}

#include "konsolepanel.moc"
