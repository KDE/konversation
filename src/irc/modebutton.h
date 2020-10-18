/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef MODEBUTTON_H
#define MODEBUTTON_H

#include <QToolButton>


class ModeButton : public QToolButton
{
    Q_OBJECT

        public:
        ModeButton(const QString& label,QWidget* parent,int id);
        ~ModeButton() override;

        void setOn(bool state);

    Q_SIGNALS:
        void modeClicked(int id,bool on);

    private Q_SLOTS:
        void wasClicked();

    private:
        int id;
        bool on;

        Q_DISABLE_COPY(ModeButton)
};

#endif
