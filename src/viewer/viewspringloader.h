/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#ifndef VIEWSPRINGLOADER_H
#define VIEWSPRINGLOADER_H

#include "viewcontainer.h"


class ViewSpringLoader : public QObject
{
    Q_OBJECT

    public:
        explicit ViewSpringLoader(ViewContainer* viewContainer);
        ~ViewSpringLoader() override;

        void addWidget(QWidget* widget);

        bool eventFilter(QObject* watched, QEvent* event) override;


    private Q_SLOTS:
        void springLoad();


    private:
        ChatWindow* viewForPos(QObject* widget, const QPoint& pos);

        QPointer<QWidget> m_hoveredWidget;
        QPointer<ChatWindow> m_hoveredView;

        QTimer m_hoverTimer;

        ViewContainer* m_viewContainer;

        Q_DISABLE_COPY(ViewSpringLoader)
};

#endif
