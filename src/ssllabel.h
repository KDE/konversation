/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 İsmail Dönmez <ismail.donmez@boun.edu.tr>
*/

#ifndef SSLLABEL_H
#define SSLLABEL_H

#include <QLabel>

class SSLLabel : public QLabel
{
    Q_OBJECT

    public:
        explicit SSLLabel(QWidget* parent);

    protected:
        void mouseReleaseEvent(QMouseEvent *e) override;

        Q_SIGNALS:
        void clicked();
};
#endif
