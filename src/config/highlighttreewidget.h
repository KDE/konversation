/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2017 Peter Simonsson <peter.simonsson@gmail.com>
*/

#ifndef HIGHLIGHTTREEWIDGET_H
#define HIGHLIGHTTREEWIDGET_H

#include <QTreeWidget>

class HighlightTreeWidget : public QTreeWidget
{
    Q_OBJECT

    public:
        HighlightTreeWidget(QWidget *parent = nullptr);

    Q_SIGNALS:
        void itemDropped();

    protected:
        void dropEvent(QDropEvent *event) override;
};

#endif // HIGHLIGHTTREEWIDGET_H
