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
    bool selected = (option.state & QStyle::State_Selected);

    QStyleOptionViewItem _option = option;
    _option.state = QStyle::State_None;

    if (index.data(ViewContainer::DisabledRole).toBool()) {
        _option.palette.setColor(QPalette::Text, QGuiApplication::palette().color(QPalette::Disabled, QPalette::Text));
    } else {
        const QColor &textColor = index.data(ViewContainer::ColorRole).value<QColor>();

        if (textColor.isValid()) {
            _option.palette.setColor(QPalette::Text, textColor);
        } else {
            _option.palette.setColor(QPalette::Text, selected
                ? m_view->palette().color(QPalette::HighlightedText)
                : m_view->palette().color(QPalette::Text));
        }
    }

    QStyledItemDelegate::paint(painter, _option, index);
}

QColor ViewTreeDelegate::mixColor(const QColor& color1, const QColor& color2)
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

    expandAll();

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ViewTree::selectionChanged);
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
        palette.setColor(QPalette::Text, Preferences::self()->color(Preferences::ChannelMessage));
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

void ViewTree::paintEvent(QPaintEvent* event)
{
    const QModelIndex &idx = selectionModel()->currentIndex();

    if (idx.isValid()) {
        int radius = 4;
        bool drawTopWing = false;

        QRect baseRect = visualRect(idx);

        QRect boundingRect = baseRect;

        if ((idx.row() > 0 || idx.parent().isValid()) && boundingRect.top() > radius) {
            drawTopWing = true;
            boundingRect.setTop(boundingRect.top() - radius);
        }

        boundingRect.setBottom(boundingRect.bottom() + radius);

        if (boundingRect.intersects(event->rect())) {
            QPainter painter(viewport());
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(palette().color(QPalette::Highlight));

            painter.drawRoundedRect(baseRect, radius - 1, radius - 1);

            baseRect.setLeft(baseRect.left() + (baseRect.width() - radius));
            painter.drawRect(baseRect);

            if (drawTopWing) {
                QPainterPath topWing;
                const QPoint &startPos = baseRect.topRight() + QPoint(1, 0);
                topWing.moveTo(startPos);
                topWing.lineTo(topWing.currentPosition().x() - radius, topWing.currentPosition().y());
                topWing.moveTo(startPos);
                topWing.lineTo(topWing.currentPosition().x(), topWing.currentPosition().y() - radius);
                topWing.quadTo(startPos, QPoint(topWing.currentPosition().x() - radius,
                    topWing.currentPosition().y() + radius));
                painter.fillPath(topWing, painter.brush());
            }

            QPainterPath bottomWing;
            const QPoint &startPos = baseRect.bottomRight() + QPoint(1, 1);
            bottomWing.moveTo(startPos);
            bottomWing.lineTo(bottomWing.currentPosition().x() - radius, bottomWing.currentPosition().y());
            bottomWing.moveTo(startPos);
            bottomWing.lineTo(bottomWing.currentPosition().x(), bottomWing.currentPosition().y() + radius);
            bottomWing.quadTo(startPos, QPoint(bottomWing.currentPosition().x() - radius,
                bottomWing.currentPosition().y() - radius));
            painter.fillPath(bottomWing, painter.brush());
        }
    }

    QTreeView::paintEvent(event);
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
    Q_UNUSED(deselected)

    const QModelIndexList& idxList = selected.indexes();

    if (idxList.count()) {
        const QModelIndex& idx = idxList.at(0);

        ChatWindow* view = static_cast<ChatWindow*>(idx.internalPointer());

        if (view) {
            emit showView(view);
        }
    }

    viewport()->update();
}



