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

#include <KParts/Part>
#include <KParts/ReadOnlyPart>

class QSplitter;
class QToolButton;
class QLabel;

class KonsolePanel : public ChatWindow
{
    Q_OBJECT

    public:
        explicit KonsolePanel(QWidget *p);
        ~KonsolePanel();

        void setName(const QString& newName) Q_DECL_OVERRIDE { ChatWindow::setName(newName); }

        QWidget* getWidget();

    Q_SIGNALS:
        void closeView(ChatWindow* view);

    public Q_SLOTS:
        void partDestroyed();
        void manageKonsoleProfiles();

        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() Q_DECL_OVERRIDE;

    protected Q_SLOTS:
        void konsoleChanged(const QString& data);

    private:
        QSplitter* m_headerSplitter;
        QToolButton* m_profileButton;
        QLabel* m_konsoleLabel;
        KParts::ReadOnlyPart *k_part;
};
#endif                                            /* KONSOLE_PANEL_H */
