
// (C)2003 Mickael Marchand <marchand@kde.org>

#ifndef KONSOLE_PANEL_H
#define KONSOLE_PANEL_H

#include "chatwindow.h"
#include <kparts/part.h>

class KonsolePanel : public ChatWindow
{
    Q_OBJECT

    public:
        KonsolePanel(QWidget *p);
        ~KonsolePanel();

        virtual void setName(const QString& newName) { ChatWindow::setName(newName); }

        signals:
        void deleted(ChatWindow* myself);

    public slots:
        void partDestroyed();

        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

    protected slots:
        void konsoleChanged(const QString& data);

    private:
        KParts::ReadOnlyPart *k_part;
};
#endif                                            /* KONSOLE_PANEL_H */
