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
#include <QItemSelectionModel>
#include <QToolTip>

// FIXME KF5 Port: Not DPI-aware.
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
    size.setWidth(1);

    return size;
}

QSize ViewTreeDelegate::preferredSizeHint(const QModelIndex& index) const
{
    QStyleOptionViewItem option;

    initStyleOption(&option, index);

    return QStyledItemDelegate::sizeHint(option, index);
}

void ViewTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    bool selected = (option.state & QStyle::State_Selected);

    const QColor& bgColor  = m_view->palette().color(m_view->backgroundRole());
    const QColor& selColor = m_view->palette().color(QPalette::Highlight);
    const QColor& midColor = mixColor(bgColor, selColor);

    QColor background;

    if (!selected && index.data(ViewContainer::HighlightRole).toBool()) {
        background = Preferences::self()->inputFieldsBackgroundColor()
            ? Preferences::self()->color(Preferences::AlternateBackground) : m_view->palette().color(QPalette::AlternateBase);
    } else {
         background = selected ? selColor : m_view->palette().color(QPalette::Base);
    }

    int y = option.rect.y();
    int height = option.rect.y() + option.rect.height();
    int width = option.rect.width();
    int left = option.rect.left();
    int right = option.rect.left() + width;

    painter->fillRect(option.rect, background);

    if (selected)
    {
        bool isFirst = (index.row() == 0 && !index.parent().isValid());

        painter->setPen(bgColor);

        if (!isFirst)
        {
            painter->drawPoint(left, y);
            painter->drawPoint(left + 1, y);
            painter->drawPoint(left, y + 1);
        }

        painter->drawPoint(left, height - 1);
        painter->drawPoint(left + 1, height - 1);
        painter->drawPoint(left, height - 2);

        painter->setPen(midColor);

        if (!isFirst)
        {
            painter->drawPoint(left + 2, y);
            painter->drawPoint(left, y + 2);
        }

        painter->drawPoint(left + 2, height - 1);
        painter->drawPoint(left, height - 3);
    }

    const QModelIndex idxAbove = m_view->indexAbove(index);

    if (idxAbove.isValid() && m_view->selectionModel()->isSelected(idxAbove)) {
        painter->setPen(selColor);
        painter->drawPoint(right - 1, y);
        painter->drawPoint(right - 2, y);
        painter->drawPoint(right - 1, y + 1);
        painter->setPen(midColor);
        painter->drawPoint(right - 3, y);
        painter->drawPoint(right - 1, y + 2);
    }

    const QModelIndex idxBelow = m_view->indexBelow(index);

    if (idxBelow.isValid() && m_view->selectionModel()->isSelected(idxBelow)) {
        painter->setPen(selColor);
        painter->drawPoint(right - 1, height - 1);
        painter->drawPoint(right - 2, height - 1);
        painter->drawPoint(right - 1, height - 2);
        painter->setPen(midColor);
        painter->drawPoint(right - 3, height - 1);
        painter->drawPoint(right - 1, height - 3);
    }

    if (!idxBelow.isValid() && selected) {
        painter->setPen(selColor);
        painter->drawPoint(right - 1, height);
        painter->drawPoint(right - 2, height);
        painter->drawPoint(right - 1, height + 1);
        painter->setPen(midColor);
        painter->drawPoint(right - 3, height);
        painter->drawPoint(right - 1, height + 2);
    }

    painter->restore();

    QStyleOptionViewItem _option = option;
    _option.state = QStyle::State_None;

    const QColor &textColor = index.data(ViewContainer::ColorRole).value<QColor>();

    if (textColor.isValid()) {
        _option.palette.setColor(QPalette::Text, textColor);
    } else {
        _option.palette.setColor(QPalette::Text, selected
            ? m_view->palette().color(QPalette::HighlightedText)
            : m_view->palette().color(QPalette::Text));
    }

    QStyledItemDelegate::paint(painter, _option, index);

    if (selected && idxAbove.isValid()) {
        m_view->update(idxAbove);
    }

    if (selected && idxBelow.isValid()) {
        m_view->update(idxBelow);
    }
}

QColor ViewTreeDelegate::mixColor(const QColor& color1, const QColor& color2) const
{
    QColor mixedColor;
    mixedColor.setRgb( (color1.red()   + color2.red())   / 2,
                       (color1.green() + color2.green()) / 2,
                       (color1.blue()  + color2.blue())  / 2 );
    return mixedColor;
}

ViewTree::ViewTree(QWidget* parent) : QTreeView(parent)
{
    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setHeaderHidden(true);
    setSortingEnabled(false);

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

    setItemDelegateForColumn(0, new ViewTreeDelegate(this));

    updateAppearance();
}

ViewTree::~ViewTree()
{
}

void ViewTree::setModel (QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ViewTree::selectionChanged);
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

bool ViewTree::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        event->accept();

        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

        const QModelIndex& idx = indexAt(helpEvent->pos());

        if (idx.isValid()) {
            const QString &text = idx.model()->data(idx, Qt::DisplayRole).toString();
            const QSize& preferredSize = static_cast<ViewTreeDelegate*>(itemDelegate())->preferredSizeHint(idx);
            const QRect& itemRect = visualRect(idx);

            if (preferredSize.width() > itemRect.width()) {
                event->accept();

                QToolTip::showText(helpEvent->globalPos(), text, this);

                return true;
            }
        }

        QToolTip::hideText();

        return true;
    }

    return QTreeView::event(event);
}

void ViewTree::resizeEvent(QResizeEvent* event)
{
    setColumnWidth(0, event->size().width());

    QTreeView::resizeEvent(event);

    emit sizeChanged();
}

void ViewTree::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        event->ignore();

        return;
    } else if (event->button() == Qt::MiddleButton && Preferences::self()->middleClickClose()) {
        const QModelIndex& idx = indexAt(event->pos());

        if (idx.isValid()) {
            m_pressedView = static_cast<ChatWindow*>(idx.internalPointer());

            event->ignore();

            return;
        }
    }

    QTreeView::mousePressEvent(event);
}

void ViewTree::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton && Preferences::self()->middleClickClose()) {
        const QModelIndex& idx = indexAt(event->pos());

        if (idx.isValid()) {
            if (m_pressedView != 0 && m_pressedView == static_cast<ChatWindow*>(idx.internalPointer())) {
                emit closeView(m_pressedView.data());
            }
        }
    }

    m_pressedView = 0;

    QTreeView::mouseReleaseEvent(event);
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

            int count = model->rowCount(idx);

            if (count) {
                idx = idx.child(count - 1, 0);
            }
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
        QTreeView::keyPressEvent(event);
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
    if (!index.isValid()) {
        return;
    }

    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
}

void ViewTree::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (!deselected.indexes().isEmpty()) {
        const QModelIndex& old = deselected.indexes().at(0);
        update(old);
        update(indexAbove(old));
        update(indexBelow(old));
    }

    const QModelIndexList& idxList = selected.indexes();

    if (idxList.count()) {
        const QModelIndex& idx = idxList.at(0);

        ChatWindow* view = static_cast<ChatWindow*>(idx.internalPointer());

        if (view) {
            emit showView(view);
        }
    }
}



