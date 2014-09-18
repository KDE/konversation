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
#include <QPainter>

// FIXME KF5 port, ViewTree port: Not DPI-aware.
#define LED_ICON_SIZE 14
#define MARGIN 2

ViewTreeDelegate::ViewTreeDelegate(QObject* parent) : QStyledItemDelegate(parent)
{
    m_view = qobject_cast<ViewTree*>(parent);
}

ViewTreeDelegate::~ViewTreeDelegate()
{
}

QSize ViewTreeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    const QRect& textRect = m_view->fontMetrics().boundingRect(index.data(Qt::DisplayRole).toString());

    size.setHeight(MARGIN + qMax(LED_ICON_SIZE, textRect.height()) + MARGIN);

    return size;
}

void ViewTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    bool selected = (option.state & QStyle::State_Selected);

    const QColor& bgColor  = m_view->palette().color(m_view->backgroundRole());
    const QColor& selColor = m_view->palette().color(QPalette::Highlight);
    const QColor& midColor = mixColor(bgColor, selColor);
    const QColor& background = selected ? selColor : m_view->palette().color(QPalette::Base);

    int y = option.rect.y();
    int height = option.rect.y() + option.rect.height();
    int width = option.rect.width();

    painter->fillRect(option.rect, background);

    if (selected)
    {
        bool isFirst = (index.row() == 0);

        painter->setPen(bgColor);

        if (!isFirst)
        {
            painter->drawPoint(0, y);
            painter->drawPoint(1, y);
            painter->drawPoint(0, y + 1);
        }

        painter->drawPoint(0, height - 1);
        painter->drawPoint(1, height - 1);
        painter->drawPoint(0, height - 2);

        painter->setPen(midColor);

        if (!isFirst)
        {
            painter->drawPoint(2, y);
            painter->drawPoint(0, y + 2);
        }

        painter->drawPoint(2, height - 1);
        painter->drawPoint(0, height - 3);
    }

    const QAbstractItemModel* model = index.model();

    if (model) {
        const QModelIndex idxBefore = model->index(index.row() - 1, 0);

        if (idxBefore.isValid() && m_view->selectionModel()->isSelected(idxBefore)) {
            painter->setPen(selColor);
            painter->drawPoint(width - 1, y);
            painter->drawPoint(width - 2, y);
            painter->drawPoint(width - 1, y + 1);
            painter->setPen(midColor);
            painter->drawPoint(width - 3, y);
            painter->drawPoint(width - 1, y + 2);
        }

        const QModelIndex idxAfter = model->index(index.row() + 1, 0);

        if (idxAfter.isValid() && m_view->selectionModel()->isSelected(idxAfter)) {
            painter->setPen(selColor);
            painter->drawPoint(width - 1, height - 1);
            painter->drawPoint(width - 2, height - 1);
            painter->drawPoint(width - 1, height - 2);
            painter->setPen(midColor);
            painter->drawPoint(width - 3, height - 1);
            painter->drawPoint(width - 1, height - 3);
        }

        bool isLast = (index.row() == model->rowCount() - 1);

        if (isLast && selected) {
            painter->setPen(selColor);
            painter->drawPoint(width - 1, height);
            painter->drawPoint(width - 2, height);
            painter->drawPoint(width - 1, height + 1);
            painter->setPen(midColor);
            painter->drawPoint(width - 3, height);
            painter->drawPoint(width - 1, height + 2);
        }
    }

    painter->restore();

    QStyleOptionViewItem _option = option;
    _option.state = QStyle::State_None;
    _option.palette.setColor(QPalette::Text, index.data(ViewContainer::ColorRole).value<QColor>());

    QStyledItemDelegate::paint(painter, _option, index);
}

QColor ViewTreeDelegate::mixColor(const QColor& color1, const QColor& color2) const
{
    QColor mixedColor;
    mixedColor.setRgb( (color1.red()   + color2.red())   / 2,
                       (color1.green() + color2.green()) / 2,
                       (color1.blue()  + color2.blue())  / 2 );
    return mixedColor;
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

    setBackgroundRole(QPalette::Base);

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
        // Only override the active color to keep around the disabled text color
        // for the disconnect label greyout.
        palette.setColor(QPalette::Active, QPalette::Text, Preferences::self()->color(Preferences::ChannelMessage));
        palette.setColor(QPalette::Base, Preferences::self()->color(Preferences::TextViewBackground));
    }

    setPalette(palette);
}

void ViewTree::resizeEvent(QResizeEvent* event)
{
    QListView::resizeEvent(event);

    emit sizeChanged();
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
    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
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



