/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006-2009 Eike Hein <hein@kde.org>
*/

#include "viewtree.h"
#include "viewcontainer.h"
#include "preferences.h"
#include "chatwindow.h"
#include "konsolepanel.h"
#include "ircview.h"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QFontDatabase>

ViewTreeDelegate::ViewTreeDelegate(QObject* parent) : QStyledItemDelegate(parent)
{
}

ViewTreeDelegate::~ViewTreeDelegate()
{
}

void ViewTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem _option = option;

    _option.palette.setColor(QPalette::Text, index.data(ViewContainer::ColorRole).value<QColor>());

    QStyledItemDelegate::paint(painter, _option, index);
}


ViewTree::ViewTree(QWidget* parent) : QListView(parent)
{
    setUniformItemSizes(true);
    setResizeMode(QListView::Adjust);

    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);

    setDragEnabled(false);
    setDropIndicatorShown(false);

    setItemDelegate(new ViewTreeDelegate(this));

    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged(QItemSelection,QItemSelection)));

    hide();
}

ViewTree::~ViewTree()
{
}

void ViewTree::updateAppearance()
{
    if (Preferences::self()->customTabFont())
        setFont(Preferences::self()->tabFont());
    else
        setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    QPalette palette;

    if (Preferences::self()->inputFieldsBackgroundColor())
    {
        palette.setColor(QPalette::Base, Preferences::self()->color(Preferences::TextViewBackground));
    }

    setPalette(palette);
}

void ViewTree::contextMenuEvent(QContextMenuEvent* event)
{
    const QModelIndex& idx = indexAt(event->pos());

    if (idx.isValid()) {
        QWidget* widget = static_cast<QWidget*>(idx.internalPointer());

        if (widget) {
            event->accept();

            emit showViewContextMenu(widget, event->globalPos());
        }
    }

    event->ignore();
}

void ViewTree::wheelEvent(QWheelEvent* event)
{
    event->accept();

    bool up = (event->angleDelta().y() > 0);

    QModelIndex idx = moveCursor(up ? QAbstractItemView::MoveUp : QAbstractItemView::MoveDown, Qt::NoModifier);

    if (idx == currentIndex()) {
        const QAbstractItemModel* model = idx.model();

        if (!model) {
            return;
        }

        if (up) {
            idx = model->index(model->rowCount() - 1, 0);
        } else {
            idx = model->index(0, 0);
        }
    }

    selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
    selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
}

void ViewTree::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        QListView::keyPressEvent(event);
    } else {
        const QModelIndex& idx = currentIndex();

        if (idx.isValid()) {
            ChatWindow* view = static_cast<ChatWindow*>(idx.internalPointer());

            if (view) {
                if (view->getInputBar())
                    QCoreApplication::sendEvent(view->getTextView(), event);
                else if (view->isInsertSupported())
                    view->appendInputText(event->text(), true);
                else if (view->getType() == ChatWindow::Konsole)
                {
                    KonsolePanel* panel = static_cast<KonsolePanel*>(view);
                    QCoreApplication::sendEvent(panel->getWidget(), event);
                }

                view->adjustFocus();
            }
        }
    }
}

void ViewTree::selectView(const QModelIndex& index)
{
    selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
    selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
}

void ViewTree::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)

    const QModelIndexList& idxList = selected.indexes();

    if (idxList.count()) {
        const QModelIndex& idx = idxList.at(0);

        ChatWindow* view = static_cast<ChatWindow*>(idx.internalPointer());

        if (view) {
            emit showView(view);
        }
    }
}


#include "viewtree.moc"
