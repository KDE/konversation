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
  Copyright (C) 2010 Eike Hein <hein@kde.org>
*/

#include "urlcatcher.h"
#include "application.h"

#include <QClipboard>
#include <QTreeView>

#include <KBookmarkDialog>
#include <KBookmarkManager>
#include <QFileDialog>
#include <KFilterProxySearchLine>
#include <KIO/CopyJob>
#include <QIcon>
#include <KLocalizedString>
#include <QMenu>
#include <KMessageBox>
#include <KToolBar>

UrlDateItem::UrlDateItem(const QDateTime& dateTime)
{
    setData(dateTime);
}

UrlDateItem::~UrlDateItem()
{
}

QVariant UrlDateItem::data(int role) const
{
    if (role == Qt::DisplayRole)
        return QLocale().toString(QStandardItem::data().toDateTime(), QLocale::ShortFormat);

    return QStandardItem::data(role);
}

UrlSortFilterProxyModel::UrlSortFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
}

UrlSortFilterProxyModel::~UrlSortFilterProxyModel()
{
}

Qt::ItemFlags UrlSortFilterProxyModel::flags(const QModelIndex& index) const
{
    return QSortFilterProxyModel::flags(index) & ~Qt::ItemIsEditable;
}

bool UrlSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    if (sortColumn() == 2)
    {
        QVariant leftData = sourceModel()->data(left, Qt::UserRole + 1);
        QVariant rightData = sourceModel()->data(right, Qt::UserRole + 1);

        return leftData.toDateTime() < rightData.toDateTime();
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

UrlCatcher::UrlCatcher(QWidget* parent) : ChatWindow(parent)
{
    setName(i18n("URL Catcher"));
    setType(ChatWindow::UrlCatcher);

    setSpacing(0);

    setupActions();
    setupUrlTree();
}

UrlCatcher::~UrlCatcher()
{
    Preferences::saveColumnState(m_urlTree, QStringLiteral("UrlCatcher ViewSettings"));
}

void UrlCatcher::setupActions()
{
    m_toolBar = new KToolBar(this, true, true);
    m_contextMenu = new QMenu(this);

    QAction* action;

    action = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("window-new")), i18nc("open url", "&Open"), this, SLOT(openSelectedUrls()));
    m_itemActions.append(action);
    m_contextMenu->addAction(action);
    action->setStatusTip(i18n("Open URLs in external browser."));
    action->setWhatsThis(i18n("<p>Select one or several <b>URLs</b> below, then click this button to launch the application associated with the mimetype of the URL.</p>-<p>In the <b>Settings</b>, under <b>Behavior</b> | <b>General</b>, you can specify a custom web browser for web URLs.</p>"));
    action->setEnabled(false);

    action = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("&Save..."), this, SLOT(saveSelectedUrls()));
    m_itemActions.append(action);
    m_contextMenu->addAction(action);
    action->setStatusTip(i18n("Save selected URLs to the disk."));
    action->setEnabled(false);

    action = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add Bookmark..."), this, SLOT (bookmarkSelectedUrls()));
    m_itemActions.append(action);
    m_contextMenu->addAction(action);
    action->setEnabled(false);

    m_toolBar->addSeparator();
    m_contextMenu->addSeparator();

    action = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18nc("copy url","&Copy"), this, SLOT(copySelectedUrls()));
    m_itemActions.append(action);
    m_contextMenu->addAction(action);
    action->setStatusTip(i18n("Copy URLs to the clipboard."));
    action->setWhatsThis(i18n("Select one or several <b>URLs</b> above, then click this button to copy them to the clipboard."));
    action->setEnabled(false);

    action = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("delete url","&Delete"), this, SLOT(deleteSelectedUrls()));
    m_itemActions.append(action);
    m_contextMenu->addAction(action);
    action->setWhatsThis(i18n("Select one or several <b>URLs</b> above, then click this button to delete them from the list."));
    action->setStatusTip(i18n("Delete selected link."));
    action->setEnabled(false);

    m_toolBar->addSeparator();
    m_contextMenu->addSeparator();

    action = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18nc("save url list", "&Save List..."), this, SLOT(saveUrlModel()));
    m_listActions.append(action);
    action->setStatusTip(i18n("Save list."));
    action->setWhatsThis(i18n("Click to save the entire list to a file."));
    action->setEnabled(false);

    action = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("edit-clear-list")), i18nc("clear url list","&Clear List"), this, SLOT(clearUrlModel()));
    m_listActions.append(action);
    action->setStatusTip(i18n("Clear list."));
    action->setWhatsThis(i18n("Click to erase the entire list."));
    action->setEnabled(false);

    updateListActionStates();
}

void UrlCatcher::setupUrlTree()
{
    KFilterProxySearchLine* searchLine = new KFilterProxySearchLine(this);

    m_urlTree = new QTreeView(this);
    m_urlTree->setWhatsThis(i18n("List of Uniform Resource Locators mentioned in any of the Konversation windows during this session."));
    m_urlTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_urlTree->setSortingEnabled(true);
    m_urlTree->header()->setSectionsMovable(false);
    m_urlTree->header()->setSortIndicatorShown(true);
    m_urlTree->setAllColumnsShowFocus(true);
    m_urlTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_urlTree->setRootIsDecorated(false);
    connect(m_urlTree, &QTreeView::customContextMenuRequested, this, &UrlCatcher::openContextMenu);
    connect(m_urlTree, &QTreeView::doubleClicked, this, &UrlCatcher::openUrl);

    Application* konvApp = Application::instance();
    QStandardItemModel* urlModel = konvApp->getUrlModel();
    QStandardItem* item = new QStandardItem(i18n("From"));
    urlModel->setHorizontalHeaderItem(0, item);
    item = new QStandardItem(i18n("URL"));
    urlModel->setHorizontalHeaderItem(1, item);
    item = new QStandardItem(i18n("Date"));
    urlModel->setHorizontalHeaderItem(2, item);
    connect(urlModel, &QStandardItemModel::rowsInserted, this, &UrlCatcher::updateListActionStates);
    connect(urlModel, &QStandardItemModel::rowsRemoved, this, &UrlCatcher::updateListActionStates);

    UrlSortFilterProxyModel* proxyModel = new UrlSortFilterProxyModel(this);
    proxyModel->setSourceModel(urlModel);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(-1);

    m_urlTree->setModel(proxyModel);
    connect(m_urlTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(updateItemActionStates()));

    searchLine->setProxy(proxyModel);

    Preferences::restoreColumnState(m_urlTree, QStringLiteral("UrlCatcher ViewSettings"), 2, Qt::DescendingOrder);
}

void UrlCatcher::updateItemActionStates()
{
    bool enable = m_urlTree->selectionModel()->hasSelection();

    foreach(QAction* action, m_itemActions) action->setEnabled(enable);
}

void UrlCatcher::updateListActionStates()
{
    Application* konvApp = Application::instance();
    bool enable = konvApp->getUrlModel()->rowCount();

    foreach(QAction* action, m_listActions) action->setEnabled(enable);
}

void UrlCatcher::openContextMenu(const QPoint& p)
{
    QModelIndex index = m_urlTree->indexAt(p);
    if (!index.isValid()) return;

    m_contextMenu->exec(QCursor::pos());
}

void UrlCatcher::openUrl(const QModelIndex& index)
{
    Application::openUrl(index.sibling(index.row(), 1).data().toString());
}

void UrlCatcher::openSelectedUrls()
{
    QModelIndexList selectedIndexes = m_urlTree->selectionModel()->selectedRows(1);

    if (selectedIndexes.count() > 1)
    {
        int ret = KMessageBox::warningContinueCancel(this,
            i18n("You have selected more than one URL. Do you really want to open several URLs at once?"),
            i18n("Open URLs"),
            KStandardGuiItem::cont(),
            KStandardGuiItem::cancel(),
            QString(),
            KMessageBox::Notify | KMessageBox::Dangerous);

        if (ret != KMessageBox::Continue) return;
    }

    foreach(const QModelIndex& index, selectedIndexes)
        if (index.isValid()) Application::openUrl(index.data().toString());
}

void UrlCatcher::saveSelectedUrls()
{
    QModelIndexList selectedIndexes = m_urlTree->selectionModel()->selectedRows(1);

    if (selectedIndexes.count() > 1)
    {
        int ret = KMessageBox::warningContinueCancel(this,
            i18n("You have selected more than one URL. A file dialog to set the destination will open for each. "
                "Do you really want to save several URLs at once?"),
            i18n("Open URLs"),
            KStandardGuiItem::cont(),
            KStandardGuiItem::cancel(),
            QString(),
            KMessageBox::Notify | KMessageBox::Dangerous);

        if (ret != KMessageBox::Continue) return;
    }

    foreach(const QModelIndex& index, selectedIndexes)
    {
        if (index.isValid())
        {
            QUrl url(index.data().toString());
            QUrl targetUrl = QFileDialog::getSaveFileUrl(this, i18n("Save link as"), QUrl::fromLocalFile(url.fileName()));

            if (targetUrl.isEmpty() || !targetUrl.isValid())
                continue;

            KIO::copy(url, targetUrl);
        }
    }
}

void UrlCatcher::bookmarkSelectedUrls()
{
    QModelIndexList selectedIndexes = m_urlTree->selectionModel()->selectedRows(1);

    KBookmarkManager* manager = KBookmarkManager::userBookmarksManager();
    KBookmarkDialog* dialog = new KBookmarkDialog(manager, this);

    if (selectedIndexes.count() > 1)
    {
        QList<KBookmarkOwner::FutureBookmark> bookmarks;

        foreach(const QModelIndex& index, selectedIndexes)
            bookmarks << KBookmarkOwner::FutureBookmark(index.data().toString(), QUrl(index.data().toString()), QString());

        dialog->addBookmarks(bookmarks, i18n("New"));
    }
    else
    {
        QString url = selectedIndexes.first().data().toString();

        dialog->addBookmark(url, QUrl(url), QString());
    }

    delete dialog;
}

void UrlCatcher::copySelectedUrls()
{
    QModelIndexList selectedIndexes = m_urlTree->selectionModel()->selectedRows(1);

    QStringList urls;

    foreach(const QModelIndex& index, selectedIndexes)
        if (index.isValid()) urls << index.data().toString();

    QClipboard* clipboard = qApp->clipboard();
    clipboard->setText(urls.join(QStringLiteral("\n")), QClipboard::Clipboard);
}

void UrlCatcher::deleteSelectedUrls()
{
    QList<QPersistentModelIndex> selectedIndices;

    foreach(const QPersistentModelIndex& index, m_urlTree->selectionModel()->selectedIndexes())
        selectedIndices << index;

    Application* konvApp = Application::instance();

    foreach(const QPersistentModelIndex& index, selectedIndices)
        if (index.isValid()) konvApp->getUrlModel()->removeRow(index.row());
}

void UrlCatcher::saveUrlModel()
{
    QString target = QFileDialog::getSaveFileName(this,
        i18n("Save URL List"));

    if (!target.isEmpty())
    {
        Application* konvApp = Application::instance();
        QStandardItemModel* urlModel = konvApp->getUrlModel();

        int nickColumnWidth = 0;

        QModelIndex index = urlModel->index(0, 0, QModelIndex());
        int rows = urlModel->rowCount();

        for (int r = 0; r < rows; r++)
        {
            int length = index.sibling(r, 0).data().toString().length();

            if (length > nickColumnWidth)
                nickColumnWidth = length;
        }

        ++nickColumnWidth;

        QFile file(target);
        file.open(QIODevice::WriteOnly);

        QTextStream stream(&file);

        stream << i18n("Konversation URL List: %1\n\n",
            QLocale().toString(QDateTime::currentDateTime(), QLocale::LongFormat));

        for (int r = 0; r < rows; r++)
        {
            QString line = index.sibling(r, 0).data().toString().leftJustified(nickColumnWidth, QLatin1Char(' '));
            line.append(index.sibling(r, 1).data().toString());
            line.append(QLatin1Char('\n'));

            stream << line;
        }

        file.close();
    }
}

void UrlCatcher::clearUrlModel()
{
    Application* konvApp = Application::instance();
    QStandardItemModel* urlModel = konvApp->getUrlModel();

    urlModel->removeRows(0, urlModel->rowCount());
}

void UrlCatcher::childAdjustFocus()
{
    m_urlTree->setFocus();
}

bool UrlCatcher::event(QEvent* event)
{
    if (event->type() == QEvent::LocaleChange) {
        Application* konvApp = Application::instance();
        QStandardItemModel* urlModel = konvApp->getUrlModel();

        m_urlTree->dataChanged(urlModel->index(0, 0), urlModel->index(urlModel->rowCount() - 1, 2));
    }

    return ChatWindow::event(event);
}
