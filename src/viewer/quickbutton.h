/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef QUICKBUTTON_H
#define QUICKBUTTON_H

#include <QPushButton>


class QuickButton : public QPushButton
{
    Q_OBJECT

        public:
        QuickButton(const QString &label,const QString &newDefinition,QWidget* parent);
        ~QuickButton() override;

        void setDefinition(const QString &newDefinition);

        Q_SIGNALS:
        void clicked(const QString &definition);

    private Q_SLOTS:
        void wasClicked();

    private:
        QString definition;

        Q_DISABLE_COPY(QuickButton)
};

#endif
