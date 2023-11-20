/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#include "viewspringloader.h"
#include "viewtree.h"

#include <QDragMoveEvent>
#include <QTabBar>
#include <QMimeData>

ViewSpringLoader::ViewSpringLoader(ViewContainer* viewContainer) : QObject(viewContainer)
{
    m_viewContainer = viewContainer;

    m_hoverTimer.setSingleShot(true);
    connect(&m_hoverTimer, &QTimer::timeout, this, &ViewSpringLoader::springLoad);
}

ViewSpringLoader::~ViewSpringLoader()
{
}

void ViewSpringLoader::addWidget(QWidget* widget)
{
    widget->installEventFilter(this);
}

bool ViewSpringLoader::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::DragEnter)
    {
        if (!static_cast<QDragEnterEvent*>(event)->mimeData()->hasFormat(QStringLiteral("application/x-konversation-chatwindow"))) {
            m_hoveredWidget = qobject_cast<QWidget*>(watched);

            event->accept();

            return true;
        }
    }
    else if (event->type() == QEvent::DragMove)
    {
        auto* dragMoveEvent = static_cast<QDragMoveEvent*>(event);

        if (!dragMoveEvent->mimeData()->hasFormat(QStringLiteral("application/x-konversation-chatwindow")))
        {
            ChatWindow* hoveredView = viewForPos(watched, dragMoveEvent->position().toPoint());

            if (hoveredView != m_hoveredView)
            {
                m_hoveredView = hoveredView;

                if (m_hoveredView)
                    m_hoverTimer.start(400);
            }

            event->ignore();

            return true;
        }
    }
    else if (event->type() == QEvent::Drop || event->type() == QEvent::DragLeave)
    {
        m_hoverTimer.stop();
        m_hoveredWidget = nullptr;
        m_hoveredView = nullptr;
    }

    return QObject::eventFilter(watched, event);
}

void ViewSpringLoader::springLoad()
{
    if (m_hoveredView && m_hoveredView == viewForPos(m_hoveredWidget,
        m_hoveredWidget->mapFromGlobal(QCursor::pos())))
    {
        m_viewContainer->showView(m_hoveredView);
        m_hoveredView = nullptr;
    }
}

ChatWindow* ViewSpringLoader::viewForPos(QObject* widget, const QPoint& pos)
{
    auto* tabBar = qobject_cast<QTabBar*>(widget);

    if (tabBar)
       return m_viewContainer->getViewAt(tabBar->tabAt(pos));
    else
    {
        auto* viewTree = qobject_cast<ViewTree*>(widget->parent());

        if (viewTree)
        {
            const QModelIndex& idx = viewTree->indexAt(QPoint(0, pos.y()));

            if (idx.isValid()) {
                auto* view = static_cast<ChatWindow*>(idx.internalPointer());

                return view;
            }
        }
    }

    return nullptr;
}

#include "moc_viewspringloader.cpp"
