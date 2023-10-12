/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#include "topichistoryview.h"
#include "application.h"
#include "irccontextmenus.h"
#include "topichistorymodel.h"

#include <KCategoryDrawer>


constexpr int MARGIN = 2;


TopicHistorySortfilterProxyModel::TopicHistorySortfilterProxyModel(QObject* parent) : KCategorizedSortFilterProxyModel(parent)
{
    setCategorizedModel(true);
}

TopicHistorySortfilterProxyModel::~TopicHistorySortfilterProxyModel()
{
}

QVariant TopicHistorySortfilterProxyModel::data(const QModelIndex& index, int role) const
{
    if (role == KCategorizedSortFilterProxyModel::CategoryDisplayRole)
    {
        const QModelIndex& sourceIndex = mapToSource(index);

        const QString& author = sourceModel()->data(sourceIndex.sibling(sourceIndex.row(), 1)).toString();
        const QString& timestamp = sourceModel()->data(sourceIndex.sibling(sourceIndex.row(), 2)).toString();

        return i18nc("%1 is a timestamp, %2 is the author's name", "On %1 by %2", timestamp, author);
    }
    else if (role == KCategorizedSortFilterProxyModel::CategorySortRole)
    {
        const QModelIndex& sourceIndex = mapToSource(index);

        return sourceModel()->data(sourceIndex.sibling(sourceIndex.row(), 2)).toDateTime().toSecsSinceEpoch();
    }

    return KCategorizedSortFilterProxyModel::data(index, role);
}

void TopicHistorySortfilterProxyModel::setSourceModel(QAbstractItemModel* model)
{
    if (sourceModel())
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged, this, &TopicHistorySortfilterProxyModel::sourceDataChanged);

    KCategorizedSortFilterProxyModel::setSourceModel(model);

    connect(model, &QAbstractItemModel::dataChanged, this, &TopicHistorySortfilterProxyModel::sourceDataChanged);
}

bool TopicHistorySortfilterProxyModel::filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const
{
    Q_UNUSED(source_parent)

    return (source_column == 0);
}

void TopicHistorySortfilterProxyModel::sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)

    Q_EMIT layoutAboutToBeChanged();
    Q_EMIT layoutChanged();
}

TopicHistoryLabel::TopicHistoryLabel(QWidget* parent) : KTextEdit(parent)
{
    viewport()->setAutoFillBackground(false);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);

    document()->setDocumentMargin(2);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    setAcceptRichText(false);

    setReadOnly(true);
    setTextSelectable(false);
}

TopicHistoryLabel::~TopicHistoryLabel()
{
}

void TopicHistoryLabel::setTextSelectable(bool selectable)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, !selectable);
    setTextInteractionFlags(selectable ? Qt::TextBrowserInteraction : Qt::NoTextInteraction);

    if (!selectable)
        setTextCursor(QTextCursor());
}

TopicHistoryItemDelegate::TopicHistoryItemDelegate(QAbstractItemView* itemView, QObject* parent) : KWidgetItemDelegate(itemView, parent)
{
    m_hiddenLabel = new TopicHistoryLabel(itemView);
    m_hiddenLabel->setFixedHeight(0);
    m_hiddenLabel->lower();

    m_shownBefore = false;

    itemView->installEventFilter(this);
}

TopicHistoryItemDelegate::~TopicHistoryItemDelegate()
{
}

bool TopicHistoryItemDelegate::eventFilter(QObject* watched, QEvent* event)
{
    // NOTE: QTextEdit needs to have been shown at least once (and while its
    // parents are shown, too) before it starts to calculate the document sizes
    // we need in sizeHint().

    if (!m_shownBefore && event->type() == QEvent::Show && !event->spontaneous())
    {
        m_hiddenLabel->show();
        m_hiddenLabel->hide();
    }

    return KWidgetItemDelegate::eventFilter(watched, event);
}

QList<QWidget*> TopicHistoryItemDelegate::createItemWidgets(const QModelIndex& index) const
{
    Q_UNUSED(index)

    QList<QWidget*> widgets;

    auto* label = new TopicHistoryLabel();
    connect(qobject_cast<TopicHistoryView*>(itemView()), &TopicHistoryView::textSelectableChanged,
        label, &TopicHistoryLabel::setTextSelectable);
    widgets << label;

    return widgets;
}

void TopicHistoryItemDelegate::updateItemWidgets(const QList<QWidget *> &widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    if (widgets.isEmpty()) return;

    auto* historyView = static_cast<TopicHistoryView*>(itemView());
    auto* label = static_cast<TopicHistoryLabel*>(widgets[0]);

    QPalette::ColorRole colorRole = !historyView->textSelectable() &&
        historyView->selectionModel()->isRowSelected(index.row(), index.parent())
            ? QPalette::HighlightedText : QPalette::Text;
    QPalette::ColorGroup colorGroup = historyView->hasFocus() ? QPalette::Active
        : QPalette::Inactive;
    const QColor& color = historyView->palette().color(colorGroup, colorRole);
    label->setTextColor(color);

    label->setPlainText(index.model()->data(index).toString());

    label->setGeometry(QRect(0, 0, option.rect.width(), option.rect.height() - (2 * MARGIN)));
}

void TopicHistoryItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index)

    if (!static_cast<TopicHistoryView*>(itemView())->textSelectable()) {
        auto* hack = const_cast<QStyleOptionViewItem*>(&option);

        hack->rect.setHeight(hack->rect.height() - (2 * MARGIN) - 1);

        itemView()->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, hack, painter, nullptr);
    }
}

QSize TopicHistoryItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)

    m_hiddenLabel->setPlainText(index.model()->data(index).toString());
    m_hiddenLabel->setFixedWidth(itemView()->viewport()->width()
        - (2 * static_cast<TopicHistoryView*>(itemView())->categorySpacing())
        - (2 * MARGIN));
    int documentHeight = m_hiddenLabel->document()->size().toSize().height();

    return QSize(itemView()->viewport()->width(), documentHeight + (2 * MARGIN));
}

TopicHistoryView::TopicHistoryView(QWidget* parent): KCategorizedView(parent)
{
    m_proxyModel = new TopicHistorySortfilterProxyModel(this);

    m_textSelectable = false;

    setCategoryDrawer(new KCategoryDrawer(this));

    setModelColumn(0);

    setItemDelegateForColumn(0, new TopicHistoryItemDelegate(this, this));

    setVerticalScrollMode(QListView::ScrollPerPixel);

    setWhatsThis(i18n("This is a list of all topics that have been set for this channel "
                      "while its tab was open.\n\n"
                      "If the same topic is set multiple times consecutively by someone, "
                      "only the final occurrence is shown.\n\n"
                      "When you select a topic in the list, the edit field below it will "
                      "receive its text. Once you start modifying the contents of the field, "
                      "however, the list will switch from the regular entry selection mode to "
                      "allowing you to perform text selection on the entries in case you may "
                      "wish to incorporate some of their text into the new topic. To return to "
                      "entry selection mode and a synchronized edit field, undo back to the "
                      "original text or close and reopen the dialog."));

    connect(Application::instance(), &Application::appearanceChanged, this, &TopicHistoryView::updateSelectedItemWidgets);
}

TopicHistoryView::~TopicHistoryView()
{
}

bool TopicHistoryView::textSelectable() const
{
    return m_textSelectable;
}

void TopicHistoryView::setTextSelectable(bool selectable)
{
    if (selectable != m_textSelectable)
    {
        m_textSelectable = selectable;

        updateSelectedItemWidgets();

        Q_EMIT textSelectableChanged(selectable);
    }
}

void TopicHistoryView::setModel(QAbstractItemModel* model)
{
    m_proxyModel->setSourceModel(model);
    KCategorizedView::setModel(m_proxyModel);
}

void TopicHistoryView::resizeEvent(QResizeEvent* event)
{
    KCategorizedView::resizeEvent(event);

    const QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();

    if (!selectedIndexes.isEmpty())
        scrollTo(selectedIndexes.first(), QAbstractItemView::EnsureVisible);
}

void TopicHistoryView::contextMenuEvent(QContextMenuEvent* event)
{
    const QModelIndex& sourceIndex = m_proxyModel->mapToSource(indexAt(event->pos()));
    QAbstractItemModel* sourceModel = m_proxyModel->sourceModel();

    const QString& text = sourceModel->data(sourceModel->index(sourceIndex.row(), 0)).toString();
    QString author = sourceModel->data(sourceModel->index(sourceIndex.row(), 1)).toString();

    if (author == TopicHistoryModel::authorPlaceholder())
        author.clear();

    IrcContextMenus::topicHistoryMenu(event->globalPos(), m_server, text, author);
}

void TopicHistoryView::updateSelectedItemWidgets()
{
    // KWidgetItemDelegate::updateItemWidgets() is documented to run in response to
    // data changes, so in order to update our widgets outside of data changes we
    // fake a data change.

    const QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();

    if (!selectedIndexes.isEmpty())
        Q_EMIT m_proxyModel->dataChanged(selectedIndexes.first(), selectedIndexes.first());
}

void TopicHistoryView::updateGeometries()
{
    KCategorizedView::updateGeometries();

    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

#include "moc_topichistoryview.cpp"
