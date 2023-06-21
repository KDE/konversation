/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2017 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "highlighttreewidget.h"

#include <QDropEvent>

HighlightTreeWidget::HighlightTreeWidget(QWidget *parent) :
    QTreeWidget(parent)
{
}

void HighlightTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidget::dropEvent(event);

    if (event->isAccepted())
        Q_EMIT itemDropped();
}

#include "moc_highlighttreewidget.cpp"
