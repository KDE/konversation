
// (C)2003 Mickael Marchand <marchand@kde.org>

#ifndef KONSOLE_PANEL_H
#define KONSOLE_PANEL_H

#include "chatwindow.h"
#include <kparts/part.h>

class KonsolePanel : public ChatWindow {
	Q_OBJECT

	public:
#ifdef USE_MDI
		KonsolePanel(QString caption);
#else
		KonsolePanel(QWidget *p);
#endif
		~KonsolePanel();

	signals:
		void deleted(ChatWindow* myself);

        public slots:
		void adjustFocus();
		void partDestroyed();

#ifdef USE_MDI
        protected:
                virtual void closeYourself(ChatWindow*);
#endif
	private:
		KParts::ReadOnlyPart *k_part;
};

#endif /* KONSOLE_PANEL_H */
