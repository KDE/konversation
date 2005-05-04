
// (C)2003 Mickael Marchand <marchand@kde.org>

#include "konsolepanel.h"
#include <kdebug.h>
#include <klibloader.h>

#ifdef USE_MDI
KonsolePanel::KonsolePanel(const QString &caption) : ChatWindow(caption)
#else
KonsolePanel::KonsolePanel(QWidget *p) : ChatWindow( p )
#endif
{
  setType(ChatWindow::Konsole);
  KLibFactory *fact = KLibLoader::self()->factory("libkonsolepart");
  if (!fact) return;

  k_part = (KParts::ReadOnlyPart *) fact->create(this);
  if (!k_part) return;

  k_part->widget()->setFocusPolicy(QWidget::WheelFocus);
  setFocusProxy(k_part->widget());
  k_part->widget()->setFocus();

	connect(k_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
}

KonsolePanel::~KonsolePanel() {
        kdDebug() << "KonsolePanel::~KonsolePanel()" << endl;
        if ( k_part ) {
		// make sure to prevent partDestroyed() signals from being sent
		disconnect(k_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
		delete k_part;
	}
}

void KonsolePanel::childAdjustFocus() {
}

void KonsolePanel::partDestroyed()
{
  k_part = 0;
#ifdef USE_MDI
  emit chatWindowCloseRequest(this);
#else
  // tell the main window to delete us
  emit deleted(this);
#endif
}

#ifdef USE_MDI
void KonsolePanel::closeYourself(ChatWindow*)
{
  emit chatWindowCloseRequest(this);
}
#endif

#include "konsolepanel.moc"
