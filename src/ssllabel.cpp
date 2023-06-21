/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 İsmail Dönmez <ismail.donmez@boun.edu.tr>
*/

#include "ssllabel.h"


SSLLabel::SSLLabel(QWidget* parent)
: QLabel(parent)
{
}

void SSLLabel::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e)

    Q_EMIT clicked();
}

#include "moc_ssllabel.cpp"
