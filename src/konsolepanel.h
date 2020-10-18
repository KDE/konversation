/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Mickael Marchand <marchand@kde.org>
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
        ~KonsolePanel() override;

        void setName(const QString& newName) override { ChatWindow::setName(newName); }

        QWidget* getWidget() const;

    Q_SIGNALS:
        void closeView(ChatWindow* view);

    public Q_SLOTS:
        void partDestroyed();
        void manageKonsoleProfiles();

        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() override;

    private Q_SLOTS:
        void konsoleChanged(const QString& data);

    private:
        QSplitter* m_headerSplitter;
        QToolButton* m_profileButton;
        QLabel* m_konsoleLabel;
        KParts::ReadOnlyPart *k_part;

        Q_DISABLE_COPY(KonsolePanel)
};

#endif                                            /* KONSOLE_PANEL_H */
