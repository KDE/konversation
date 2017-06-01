/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Peter Simonsson <peter.simonsson@gmail.com>
*/

#ifndef HIGHLIGHTTREEWIDGET_H
#define HIGHLIGHTTREEWIDGET_H

#include <QTreeWidget>

class HighlightTreeWidget : public QTreeWidget
{
    Q_OBJECT

    public:
        HighlightTreeWidget(QWidget *parent = nullptr);

    protected:
        void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

    signals:
        void itemDropped();
};

#endif // HIGHLIGHTTREEWIDGET_H
