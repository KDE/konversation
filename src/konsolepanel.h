/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2003 Mickael Marchand <marchand@kde.org>
*/

#ifndef KONSOLE_PANEL_H
#define KONSOLE_PANEL_H

#include "chatwindow.h"

#include <kparts/part.h>


class KonsolePanel : public ChatWindow
{
    Q_OBJECT

    public:
        explicit KonsolePanel(QWidget *p);
        ~KonsolePanel();

        virtual void setName(const QString& newName) { ChatWindow::setName(newName); }

        QWidget* getWidget();

    signals:
        void closeView(ChatWindow* view);

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
