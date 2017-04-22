/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Peter Simonsson <peter.simonsson@gmail.com>
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
        emit itemDropped();
}
