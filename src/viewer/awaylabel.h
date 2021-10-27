/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONAWAYLABEL_H
#define KONVERSATIONAWAYLABEL_H

#include <QLabel>


class AwayLabel : public QLabel
{
    Q_OBJECT

    public:
        explicit AwayLabel(QWidget *parent = nullptr);
        ~AwayLabel() override;

    Q_SIGNALS:
        void awayMessageChanged(const QString&);
        void unaway();

    private Q_SLOTS:
        void changeAwayMessage();

    private:
        Q_DISABLE_COPY(AwayLabel)
};

#endif
