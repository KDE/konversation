/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006-2009 Eike Hein <hein@kde.org>
*/

// FIXME KF5 Port: ViewTree TODO: Close buttons, DND, separator painting.

#ifndef VIEWTREE_H
#define VIEWTREE_H

#include <QStyledItemDelegate>
#include <QTreeView>
#include <QPointer>

class ChatWindow;
class ViewTree;

class ViewTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        explicit ViewTreeDelegate(ViewTree *parent = nullptr);
        ~ViewTreeDelegate() override;

        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize preferredSizeHint(const QModelIndex& index) const;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        ViewTree* m_view;

        Q_DISABLE_COPY(ViewTreeDelegate)
};

class ViewTree : public QTreeView
{
    Q_OBJECT

    public:
        explicit ViewTree(QWidget *parent);
        ~ViewTree() override;

        void setModel(QAbstractItemModel *model) override;

        bool dropIndicatorOnItem() const;

    public Q_SLOTS:
        void updateAppearance();
        void selectView(const QModelIndex &index);

    Q_SIGNALS:
        void sizeChanged();
        void showView(ChatWindow* view);
        void closeView(ChatWindow* view);
        void showViewContextMenu(QWidget* widget, const QPoint& point);

    protected:
        bool event(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private Q_SLOTS:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

    private:
        QPointer<ChatWindow> m_pressedView;

        QPoint m_pressPos;

        int m_accumulatedWheelDelta;
        bool m_lastWheelDeltaDirection;

        Q_DISABLE_COPY(ViewTree)
};

#endif
