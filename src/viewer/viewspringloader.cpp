/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2012 Eike Hein <hein@kde.org>
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
        if (!dynamic_cast<QDragEnterEvent*>(event)->mimeData()->hasFormat(QStringLiteral("application/x-konversation-chatwindow")))
        {
            m_hoveredWidget = qobject_cast<QWidget*>(watched);

            event->accept();

            return true;
        }
    }
    else if (event->type() == QEvent::DragMove)
    {
        QDragMoveEvent* dragMoveEvent = dynamic_cast<QDragMoveEvent*>(event);

        if (!dragMoveEvent->mimeData()->hasFormat(QStringLiteral("application/x-konversation-chatwindow")))
        {
            ChatWindow* hoveredView = viewForPos(watched, dragMoveEvent->pos());

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
    QTabBar* tabBar = qobject_cast<QTabBar*>(widget);

    if (tabBar)
       return m_viewContainer->getViewAt(tabBar->tabAt(pos));
    else
    {
        ViewTree* viewTree = qobject_cast<ViewTree*>(widget->parent());

        if (viewTree)
        {
            const QModelIndex& idx = viewTree->indexAt(QPoint(0, pos.y()));

            if (idx.isValid()) {
                ChatWindow* view = static_cast<ChatWindow*>(idx.internalPointer());

                return view;
            }
        }
    }

    return nullptr;
}
