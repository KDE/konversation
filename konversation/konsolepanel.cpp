
// (C)2003 Mickael Marchand <marchand@kde.org>

#include "konsolepanel.h"
#include <klibloader.h>

KonsolePanel::KonsolePanel(QWidget *p) : ChatWindow( p ) {
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
	if ( k_part )
		delete k_part;
}

void KonsolePanel::adjustFocus() {
}

void KonsolePanel::partDestroyed()
{
  k_part = 0;
}

#include "konsolepanel.moc"
