
// (C)2003 Mickael Marchand <marchand@kde.org>

#ifndef KONSOLE_PANEL_H
#define KONSOLE_PANEL_H

#include "chatwindow.h"
#include <kparts/part.h>

class KonsolePanel : public ChatWindow {
	Q_OBJECT

	public:
		KonsolePanel(QWidget *p);
		~KonsolePanel();

	public slots:
		void adjustFocus();
		void partDestroyed();

	private:
		KParts::ReadOnlyPart *k_part;
};

#endif /* KONSOLE_PANEL_H */
