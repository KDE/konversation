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
#include <QGuiApplication>
#include <QPainter>
#include <QItemSelectionModel>
#include <QStyleHints>
#include <QToolTip>

// FIXME KF5 Port: Not DPI-aware.
#define LED_ICON_SIZE 14
#define MARGIN 2
#define RADIUS 4

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
    bool highlighted = index.data(ViewContainer::HighlightRole).toBool();

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    const QColor &selColor = m_view->palette().color(QPalette::Highlight);

    if (selected || highlighted) {
        QColor bgColor = selColor;

        if (highlighted && !selected) {
            bgColor = Preferences::self()->inputFieldsBackgroundColor()
                ? Preferences::self()->color(Preferences::AlternateBackground)
                : m_view->palette().color(QPalette::AlternateBase);
        }

        painter->setBrush(bgColor);

        QRect baseRect = option.rect;

        painter->drawRoundedRect(baseRect, RADIUS - 1, RADIUS - 1);

        baseRect.setLeft(baseRect.left() + (baseRect.width() - RADIUS));
        painter->drawRect(baseRect);


    }

    const QModelIndex idxAbove = m_view->indexAbove(index);

    if (idxAbove.isValid() && m_view->selectionModel()->isSelected(idxAbove)) {
        QPainterPath bottomWing;
        const QPoint &startPos = option.rect.topRight() + QPoint(1, 0);
        bottomWing.moveTo(startPos);
        bottomWing.lineTo(bottomWing.currentPosition().x() - RADIUS, bottomWing.currentPosition().y());
        bottomWing.moveTo(startPos);
        bottomWing.lineTo(bottomWing.currentPosition().x(), bottomWing.currentPosition().y() + RADIUS);
        bottomWing.quadTo(startPos, QPoint(bottomWing.currentPosition().x() - RADIUS,
            bottomWing.currentPosition().y() - RADIUS));
        painter->fillPath(bottomWing, selColor);
    }

    const QModelIndex idxBelow = m_view->indexBelow(index);

    if (idxBelow.isValid() && m_view->selectionModel()->isSelected(idxBelow)) {
        QPainterPath topWing;
        const QPoint &startPos = option.rect.bottomRight() + QPoint(1, 1);
        topWing.moveTo(startPos);
        topWing.lineTo(topWing.currentPosition().x() - RADIUS, topWing.currentPosition().y());
        topWing.moveTo(startPos);
        topWing.lineTo(topWing.currentPosition().x(), topWing.currentPosition().y() - RADIUS);
        topWing.quadTo(startPos, QPoint(topWing.currentPosition().x() - RADIUS,
            topWing.currentPosition().y() + RADIUS));
        painter->fillPath(topWing, selColor);
    }

    painter->restore();

    QStyleOptionViewItem _option = option;
    _option.state = QStyle::State_None;
    _option.palette.setColor(QPalette::AlternateBase, Qt::transparent);

    if (index.data(ViewContainer::DisabledRole).toBool()) {
        _option.palette.setColor(QPalette::Text, QGuiApplication::palette().color(QPalette::Disabled, QPalette::Text));
    } else {
        const QColor &textColor = index.data(ViewContainer::ColorRole).value<QColor>();

        if (textColor.isValid() && !selected) {
            _option.palette.setColor(QPalette::Text, textColor);
        } else {
            _option.palette.setColor(QPalette::Text, selected
                ? m_view->palette().color(QPalette::HighlightedText)
                : m_view->palette().color(QPalette::Text));
        }
    }

    QStyledItemDelegate::paint(painter, _option, index);
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

    setDragEnabled(true); // We initiate drags ourselves below.
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setDropIndicatorShown(true);

    setBackgroundRole(QPalette::Base);

    setItemDelegateForColumn(0, new ViewTreeDelegate(this));

    updateAppearance();
}

ViewTree::~ViewTree()
{
}

void ViewTree::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    expandAll();

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ViewTree::selectionChanged);
}

bool ViewTree::dropIndicatorOnItem() const
{
    return (dropIndicatorPosition() == OnItem);
}
void ViewTree::updateAppearance()
{
    if (Preferences::self()->customTabFont())
        setFont(Preferences::self()->tabFont());
    else
        setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    setAlternatingRowColors(Preferences::self()->inputFieldsBackgroundColor());

    QPalette palette;

    if (Preferences::self()->inputFieldsBackgroundColor())
    {
        palette.setColor(QPalette::Text, Preferences::self()->color(Preferences::ChannelMessage));
        palette.setColor(QPalette::Base, Preferences::self()->color(Preferences::TextViewBackground));
        palette.setColor(QPalette::AlternateBase, Preferences::self()->color(Preferences::AlternateBackground));
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
    QTreeView::paintEvent(event);

    const QModelIndex &currentRow = selectionModel()->currentIndex();

    if (!currentRow.isValid()) {
        return;
    }

    const QAbstractItemModel* model = currentRow.model();

    QModelIndex lastRow = model->index(model->rowCount() - 1, 0);

    int count = model->rowCount(lastRow);

    if (count) {
        lastRow = lastRow.child(count - 1, 0);
    }

    if (lastRow.isValid() && lastRow == currentRow) {
        const QRect &baseRect = visualRect(lastRow);

        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette().color(QPalette::Highlight));

        QPainterPath bottomWing;
        const QPoint &startPos = baseRect.bottomRight() + QPoint(1, 1);
        bottomWing.moveTo(startPos);
        bottomWing.lineTo(bottomWing.currentPosition().x() - RADIUS, bottomWing.currentPosition().y());
        bottomWing.moveTo(startPos);
        bottomWing.lineTo(bottomWing.currentPosition().x(), bottomWing.currentPosition().y() + RADIUS);
        bottomWing.quadTo(startPos, QPoint(bottomWing.currentPosition().x() - RADIUS,
            bottomWing.currentPosition().y() - RADIUS));
        painter.fillPath(bottomWing, painter.brush());
    }
}

void ViewTree::drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // Avoid style engine's row pre-fill.
    QStyleOptionViewItem _option = option;
    _option.palette.setColor(QPalette::Highlight, Qt::transparent);

    QTreeView::drawRow(painter, _option, index);
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
    } else if (event->button() == Qt::MiddleButton) {
        m_pressPos = event->pos();

        const QModelIndex& idx = indexAt(event->pos());

        if (idx.isValid()) {
            m_pressedView = static_cast<ChatWindow*>(idx.internalPointer());
        }
    }

    QTreeView::mousePressEvent(event);
}

void ViewTree::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_pressPos = QPoint();

        if (Preferences::self()->middleClickClose()) {
            const QModelIndex& idx = indexAt(event->pos());

            if (idx.isValid()) {
                if (m_pressedView != 0 && m_pressedView == static_cast<ChatWindow*>(idx.internalPointer())) {
                    emit closeView(m_pressedView.data());
                }
            }
        }
    }

    m_pressedView = 0;

    QTreeView::mouseReleaseEvent(event);
}

void ViewTree::mouseMoveEvent(QMouseEvent* event)
{
    if (m_pressedView && (m_pressPos - event->pos()).manhattanLength() >= QGuiApplication::styleHints()->startDragDistance()) {
        selectionModel()->select(indexAt(event->pos()), QItemSelectionModel::ClearAndSelect);
        startDrag(Qt::MoveAction);
        m_pressPos = QPoint();
        m_pressedView = 0;
    }

    QTreeView::mouseMoveEvent(event);
}

void ViewTree::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->source() == this && event->possibleActions() & Qt::MoveAction) {
        event->accept();
        setState(DraggingState);
    } else {
        event->ignore();
    }
}

void ViewTree::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);

    // Work around Qt being idiotic and not hiding the drop indicator when we want it to.
    setDropIndicatorShown(event->isAccepted() && !dropIndicatorOnItem());
    viewport()->update();
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



