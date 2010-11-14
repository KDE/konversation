/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This is the class that shows the channel nick list
  begin:     Fre Jun 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "nicklistview.h"
#include "nick.h"
#include "application.h"
#include "images.h"
#include "irccontextmenus.h"

#include <QHeaderView>
#include <QDropEvent>
#include <QToolTip>
#include <QStyledItemDelegate>


class NickItemDelegate : public QStyledItemDelegate
{
    public:
        NickItemDelegate(QObject *parent = 0);

        virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
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
    channel=chan;

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

    connect(selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(syncSelectionToCurrent(const QModelIndex&)));
}

NickListView::~NickListView()
{
}

void NickListView::syncSelectionToCurrent(const QModelIndex& index)
{
    if (index.isValid())
        selectionModel()->select(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
}

int NickListView::getMinimumRowHeight()
{
    return s_minimumRowHeight;
}

void NickListView::updateMinimumRowHeight()
{
    Images* images = Application::instance()->images();
    s_minimumRowHeight = images->getNickIcon(Images::Normal, false).height() + 2;
}

bool NickListView::event(QEvent *event)
{
    if(( event->type() == QEvent::ToolTip ) )
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>( event );

        QTreeWidgetItem *item = itemAt( helpEvent->pos() );
        if( item )
        {
            Nick *nick = dynamic_cast<Nick*>( item );
            if( nick )
            {
               QString text =  Konversation::removeIrcMarkup(nick->getChannelNick()->tooltip());
               if( !text.isEmpty() )
                       QToolTip::showText( helpEvent->globalPos(), text );
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

void NickListView::setWhatsThis()
{
    Images* images = Application::instance()->images();

    if(images->getNickIcon( Images::Normal, false).isNull())
    {
        QTreeWidget::setWhatsThis(i18n("<qt><p>This shows all the people in the channel.  The nick for each person is shown.<br />Usually an icon is shown showing the status of each person, but you do not seem to have any icon theme installed.  See the Konversation settings - <i>Configure Konversation</i> under the <i>Settings</i> menu.  Then view the page for <i>Themes</i> under <i>Appearance</i>.</p></qt>"));
    }
    else
    {
        QTreeWidget::setWhatsThis(i18n("<qt><p>This shows all the people in the channel.  The nick for each person is shown, with a picture showing their status.</p>"
            "<table>"

            "<tr><th><img src=\"admin\"/></th><td>This person has administrator privileges.</td></tr>"
            "<tr><th><img src=\"owner\"/></th><td>This person is a channel owner.</td></tr>"
            "<tr><th><img src=\"op\"/></th><td>This person is a channel operator.</td></tr>"
            "<tr><th><img src=\"halfop\"/></th><td>This person is a channel half-operator.</td></tr>"
            "<tr><th><img src=\"voice\"/></th><td>This person has voice, and can therefore talk in a moderated channel.</td></tr>"
            "<tr><th><img src=\"normal\"/></th><td>This person does not have any special privileges.</td></tr>"
            "<tr><th><img src=\"normalaway\"/></th><td>This indicates that this person is currently away.</td></tr>"
            "</table><p>"
            "The meaning of admin, owner and halfop varies between different IRC servers.</p><p>"
            "Hovering over any nick shows their current status"
                    //", as well as any information in the addressbook for this person"
                    ". See the Konversation Handbook for more information."
            "</p></qt>"
            )
            .replace("'admin'", images->getNickIconPath(Images::Admin))
            .replace("'owner'", images->getNickIconPath(Images::Owner))
            .replace("'op'", images->getNickIconPath(Images::Op))
            .replace("'halfop'", images->getNickIconPath(Images::HalfOp))
            .replace("'voice'", images->getNickIconPath(Images::Voice))
            .replace("'normal'", images->getNickIconPath(Images::Normal))
            .replace("'normalaway'", images->getNickIconPath(Images::Normal))
            );
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
                this, 0);
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
    if (selectedItems().count())
    {
        IrcContextMenus::nickMenu(ev->globalPos(), IrcContextMenus::ShowChannelActions,
            channel->getServer(), channel->getSelectedNickList(), channel->getName());
    }
}

QStringList NickListView::mimeTypes () const
{
    return KUrl::List::mimeDataTypes();
}

bool NickListView::canDecodeMime(QDropEvent const *event) const {
    // Verify if the URL is not irc://
    if (KUrl::List::canDecode(event->mimeData()))
    {
        const KUrl::List uris = KUrl::List::fromMimeData(event->mimeData());
        if (!uris.isEmpty())
        {
            const KUrl first = uris.first();

            if (first.protocol() == QLatin1String("irc") ||
                first.protocol() == QLatin1String("ircs") ||
                channel->getNickList().containsNick(first.url()))
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

    if (!indexAt(event->pos()).isValid())
    {
        event->ignore();
        return;
    }
}

bool NickListView::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
    Q_UNUSED(index);
    Q_UNUSED(action);
    Nick* nick = dynamic_cast<Nick*>(parent);
    if (nick) {
        const KUrl::List uris = KUrl::List::fromMimeData(data);
        channel->getServer()->sendURIs(uris, nick->getChannelNick()->getNickname());
        return true;
    }
    return false;
}

#include "nicklistview.moc"
