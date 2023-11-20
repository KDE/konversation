/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "nicklistview.h"
#include "nick.h"
#include "application.h"
#include "irccontextmenus.h"

#include <QHeaderView>
#include <QDropEvent>
#include <QMimeData>
#include <QToolTip>
#include <QStyledItemDelegate>
#include <QBuffer>

#include <KUrlMimeData>

class NickItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        NickItemDelegate(QObject *parent = nullptr);

        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

NickItemDelegate::NickItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize NickItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QSize& size = QStyledItemDelegate::sizeHint(option, index);
    return QSize(size.width(), qMax(NickListView::getMinimumRowHeight(), size.height()));
}

int NickListView::s_minimumRowHeight = 0;


NickListView::NickListView(QWidget* parent, Channel *chan) : QTreeWidget(parent)
{
    setWhatsThis();
    m_channel = chan;

    // Enable Drag & Drop
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(false);
    setDragDropMode(QAbstractItemView::DropOnly);

    // Make the minimum height of nicklist items the height of their
    // icons plus two pixels.
    setUniformRowHeights(true);
    QAbstractItemDelegate *prevDelegate = itemDelegate();
    setItemDelegate(new NickItemDelegate(this));
    delete prevDelegate;
    updateMinimumRowHeight();

    // General layout
    setRootIsDecorated(false); // single level view
    setColumnCount(2);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAllColumnsShowFocus(true);

    // These two below must be called after setColumnCount().
    header()->setSortIndicator(Nick::NicknameColumn, Qt::AscendingOrder);
    setSortingEnabled(true);

    header()->hide();
    header()->setStretchLastSection(false);
}

NickListView::~NickListView()
{
}

int NickListView::getMinimumRowHeight()
{
    return s_minimumRowHeight;
}

void NickListView::updateMinimumRowHeight()
{
    Images* images = Application::instance()->images();
    s_minimumRowHeight = images->getNickIconSize() + 2;
}

bool NickListView::event(QEvent *event)
{
    if(( event->type() == QEvent::ToolTip ) )
    {
        auto* helpEvent = static_cast<QHelpEvent*>(event);

        QTreeWidgetItem *item = itemAt(viewport()->mapFromParent(helpEvent->pos()));
        if( item )
        {
            Nick *nick = dynamic_cast<Nick*>( item );
            if( nick )
            {
               QString text =  Konversation::removeIrcMarkup(nick->getChannelNick()->tooltip());
               if( !text.isEmpty() )
                       QToolTip::showText( helpEvent->globalPos(), text, this );
               else
                       QToolTip::hideText();
            }

        }
        else
                QToolTip::hideText();
    }
    return QTreeWidget::event( event );
}

// Make this public
void NickListView::executeDelayedItemsLayout()
{
    QTreeWidget::executeDelayedItemsLayout();
}

static
QString iconImgTag(const QIcon& icon)
{
    const QPixmap pixmap = icon.pixmap(16, 16);
    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG", 100);

    const QString imgTag = QStringLiteral("<img width='16' height='16' src='data:image/png;base64, %1'/>")
           .arg(QString::fromLatin1(pngBytes.toBase64()));
    return imgTag;
}

void NickListView::setWhatsThis()
{
    Images* images = Application::instance()->images();

    if(!images->getNickIcon( Images::Normal, false).isNull())
    {
        QTreeWidget::setWhatsThis(i18n("<qt><p>This shows all the people in the channel.  The nick for each person is shown, with a picture showing their status.<br /></p>"
            "<table>"
            "<tr><th>%1</th><td>This person has administrator privileges.</td></tr>"
            "<tr><th>%2</th><td>This person is a channel owner.</td></tr>"
            "<tr><th>%3</th><td>This person is a channel operator.</td></tr>"
            "<tr><th>%4</th><td>This person is a channel half-operator.</td></tr>"
            "<tr><th>%5</th><td>This person has voice, and can therefore talk in a moderated channel.</td></tr>"
            "<tr><th>%6</th><td>This person does not have any special privileges.</td></tr>"
            "<tr><th>%7</th><td>This, overlaid on any of the above, indicates that this person is currently away.</td></tr>"
            "</table><p>"
            "The meaning of admin, owner and halfop varies between different IRC servers.</p><p>"
            "Hovering over any nick shows their current status. See the Konversation Handbook for more information."
            "</p></qt>",
            iconImgTag(images->getNickIcon(Images::Admin)),
            iconImgTag(images->getNickIcon(Images::Owner)),
            iconImgTag(images->getNickIcon(Images::Op)),
            iconImgTag(images->getNickIcon(Images::HalfOp)),
            iconImgTag(images->getNickIcon(Images::Voice)),
            iconImgTag(images->getNickIcon(Images::Normal)),
            iconImgTag(images->getNickIconAwayOverlay())));
    }

}

void NickListView::refresh()
{
    updateMinimumRowHeight();

    QTreeWidgetItemIterator it(this);

    while (*it)
    {
        static_cast<Nick*>(*it)->refresh();
        ++it;
    }

    setWhatsThis();
}

void NickListView::setSortingEnabled(bool enable)
{
    QTreeWidget::setSortingEnabled(enable);
    // We want to decouple header and this object with regard to sorting
    // (for performance reasons). By default there is no way to reenable
    // sorting without full resort (which is necessary for us) since both
    // header indicator change and setSortingEnabled(false/true) trigger
    // full resort of QTreeView. However, after disconnect, it is possible
    // to use header()->setSortIndicator() for this.
    // This could probably be done in a better way if nick list was model
    // based. NOTE: QTreeView::sortByColumn() won't work after this change
    // if sorting is enabled.
    if (enable && header()) {
        disconnect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
                this, nullptr);
    }
}

void NickListView::fastSetSortingEnabled(bool value)
{
    if (value)
    {
        int sortCol = header()->sortIndicatorSection();
        if (sortCol > -1)
        {
            header()->setSortIndicator(-1, header()->sortIndicatorOrder());
            // since indicator section is -1, this basically sets the flag only
            // while setSortIndicator() is decoupled from triggerring resort.
            setSortingEnabled(true);
            // NOTE:: ResizeToContents mode on sortCol would be performance killer
            header()->setSortIndicator(sortCol, header()->sortIndicatorOrder());
        }
    }
    else
    {
        setSortingEnabled(false);
    }
}

void NickListView::sortByColumn(int column, Qt::SortOrder order)
{
    if (isSortingEnabled())
    {
        // original implementation relies on sortIndicatorChanged signal
        model()->sort(column, order);
    }
    else
        QTreeWidget::sortByColumn(column, order);
}

void NickListView::resort()
{
    sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
}

int NickListView::findLowerBound(const QTreeWidgetItem& item) const
{
    int start = 0, end = topLevelItemCount();
    int mid;

    while (start < end) {
        mid = start + (end-start)/2;
        if ((*topLevelItem(mid)) < item)
            start = mid + 1;
        else
            end = mid;
    }
    return start;
}

void NickListView::contextMenuEvent(QContextMenuEvent* ev)
{
    if (!selectedItems().isEmpty()) {
        IrcContextMenus::nickMenu(ev->globalPos(), IrcContextMenus::ShowChannelActions,
            m_channel->getServer(), m_channel->getSelectedNickList(), m_channel->getName());
    }
}

QStringList NickListView::mimeTypes () const
{
    return  KUrlMimeData::mimeDataTypes();
}

bool NickListView::canDecodeMime(QDropEvent const *event) const {

    // Verify if the URL is not irc://
    if (event->mimeData()->hasUrls())
    {
        const QList<QUrl> uris = KUrlMimeData::urlsFromMimeData(event->mimeData());

        if (!uris.isEmpty())
        {
            const QUrl first = uris.first();

            if (first.scheme() == QLatin1String("irc") ||
                first.scheme() == QLatin1String("ircs") ||
                m_channel->getNickList().containsNick(first.url()))
                {
                    return false;
                }
        }
        return true;
    }

    return false;
}

void NickListView::dragEnterEvent(QDragEnterEvent *event)
{
    if (canDecodeMime(event)) {
        QTreeWidget::dragEnterEvent(event);
        return;
    }
    else
    {
        event->ignore();
    }
}

void NickListView::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    if (!indexAt(event->position().toPoint()).isValid())
    {
        event->ignore();
        return;
    }
}

bool NickListView::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
    Q_UNUSED(index)
    Q_UNUSED(action)
    Nick* nick = dynamic_cast<Nick*>(parent);
    if (nick) {
        const QList<QUrl> uris = KUrlMimeData::urlsFromMimeData(data);
        m_channel->getServer()->sendURIs(uris, nick->getChannelNick()->getNickname());
        return true;
    }
    return false;
}

#include "nicklistview.moc"
#include "moc_nicklistview.cpp"
