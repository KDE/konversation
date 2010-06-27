/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Shows all URLs found by the client

  Copyright (C) 2003 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2009 Travis McHenry <wordsizzle@gmail.com>
*/

#include "urlcatcher.h"
#include "channel.h"
#include "server.h"
#include "application.h"
#include "viewcontainer.h"

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QClipboard>

#include <KMenu>
#include <KBookmarkManager>
#include <kbookmarkdialog.h>
#include <KUrl>
#include <KFileDialog>
#include <KIO/CopyJob>
#include <KToolBar>

UrlCatcherModel::UrlCatcherModel(QObject* parent) : QAbstractListModel(parent)
{
}

bool operator==(const UrlItem& item, const UrlItem& item2)
{
    return (item.nick == item2.nick && item.url == item2.url && item.datetime == item2.datetime);
}

void UrlCatcherModel::append(const UrlItem& item)
{
    if(!m_urlList.contains(item))
        m_urlList.append(item);
    reset();
}

bool UrlCatcherModel::removeRows(int row, int count, const QModelIndex& parent)
{
    int last = row+count;
    beginRemoveRows(parent, row, last);

    QModelIndex topLeft = parent.sibling(row,0);
    QModelIndex bottomRight = parent.sibling(last,2);

    bool success = true;
    for(int i=row; i <= last; i++)
    {
        UrlItem item;
        item.nick = parent.sibling(i,0).data().toString();
        item.url = parent.sibling(i,1).data().toString();
        item.datetime = parent.sibling(i,2).data().toDateTime();
        if (success)
            success = m_urlList.removeOne(item);
    }

    emit dataChanged(topLeft, bottomRight);
    endRemoveRows();

    return success;
}

void UrlCatcherModel::setUrlList(const QList<UrlItem>& list)
{
    m_urlList = list;
    reset();
}

int UrlCatcherModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 3;
}

int UrlCatcherModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_urlList.count();
}

QVariant UrlCatcherModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_urlList.count ())
        return QVariant();

    const UrlItem& item = m_urlList[index.row()];

    if (role == Qt::DisplayRole)
    {
        switch(index.column())
        {
            case 0:
                return item.nick;
            case 1:
                return item.url;
            case 2:
                return KGlobal::locale()->formatDateTime(item.datetime, KLocale::ShortDate, true);
            default:
                return QVariant();
        }
    }
    if (role == Qt::UserRole)
    {
        switch(index.column())
        {
            case 0:
                return item.nick;
            case 1:
                return item.url;
            case 2:
                return item.datetime;
            default:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant UrlCatcherModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case 0:
            return i18n("From");
        case 1:
            return i18n("URL");
        case 2:
            return i18n("Date");
        default:
            return QVariant();
    }
}

UrlCatcher::UrlCatcher(QWidget* parent) : ChatWindow(parent)
{
    setName(i18n("URL Catcher"));
    setType(ChatWindow::UrlCatcher);

    setSpacing(0);
    m_toolBar = new KToolBar(this, true, true);
    m_toolBar->setObjectName("urlcatcher_toolbar");
    m_open = m_toolBar->addAction(KIcon("window-new"), i18nc("open url", "&Open"), this, SLOT(openUrlClicked()));
    m_open->setStatusTip(i18n("Open link in external browser."));
    m_open->setWhatsThis(i18n("<p>Select a <b>URL</b> above, then click this button to launch the application associated with the mimetype of the URL.</p>-<p>In the <b>Settings</b>, under <b>Behavior</b> | <b>General</b>, you can specify a custom web browser for web URLs.</p>"));
    m_open->setEnabled(false);
    m_saveLink = m_toolBar->addAction(KIcon("document-save"), i18n("&Save..."), this, SLOT(saveLinkAs()));
    m_saveLink->setStatusTip(i18n("Save selected link to the disk."));
    m_saveLink->setEnabled(false);
    m_toolBar->addSeparator();
    m_copy = m_toolBar->addAction(KIcon("edit-copy"), i18nc("copy url","&Copy"), this, SLOT(copyUrlClicked()));
    m_copy->setStatusTip(i18n("Copy link address to the clipboard."));
    m_copy->setWhatsThis(i18n("Select a <b>URL</b> above, then click this button to copy the URL to the clipboard."));
    m_copy->setEnabled(false);
    m_delete = m_toolBar->addAction(KIcon("edit-delete"), i18nc("delete url","&Delete"), this, SLOT(deleteUrlClicked()));
    m_delete->setWhatsThis(i18n("Select a <b>URL</b> above, then click this button to delete the URL from the list."));
    m_delete->setStatusTip(i18n("Delete selected link."));
    m_delete->setEnabled(false);
    m_toolBar->addSeparator();
    m_save = m_toolBar->addAction(KIcon("document-save"), i18nc("save url list", "&Save List..."), this, SLOT(saveListClicked()));
    m_save->setStatusTip(i18n("Save list."));
    m_save->setWhatsThis(i18n("Click to save the entire list to a file."));
    m_save->setEnabled(false);
    m_clear = m_toolBar->addAction(KIcon("edit-clear-list"), i18nc("clear url list","&Clear List"), this, SLOT(clearListClicked()));
    m_clear->setStatusTip(i18n("Clear list."));
    m_clear->setWhatsThis(i18n("Click to erase the entire list."));
    m_clear->setEnabled(false);
    m_toolBar->addSeparator();
    m_bookmarkLink = m_toolBar->addAction(KIcon("bookmark-new"), i18n("Add Bookmark..."), this, SLOT (bookmarkUrl()));
    m_bookmarkLink->setEnabled(false);

    setupUi(this);

    m_urlListModel = new UrlCatcherModel(this);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSortRole(Qt::UserRole);
    m_proxyModel->setSourceModel(m_urlListModel);
    m_urlListView->setModel(m_proxyModel);

    m_proxyModel->setDynamicSortFilter(true);

    Preferences::restoreColumnState(m_urlListView, "UrlCatcher ViewSettings");

    connect(m_urlListView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(urlSelected(const QItemSelection&)));
    connect(m_urlListView, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(openUrl(const QModelIndex&)) );
    connect(m_urlListView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(contextMenu(const QPoint&)) );

    m_filterLine->setProxy(m_proxyModel);
    connect(m_proxyModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(updateButtons()));
    connect(m_proxyModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(updateButtons()));
}


UrlCatcher::~UrlCatcher()
{
    Preferences::saveColumnState(m_urlListView, "UrlCatcher ViewSettings");
}

void UrlCatcher::updateButtons()
{
    if(!m_proxyModel->rowCount())
    {
        m_clear->setEnabled(false);
        m_save->setEnabled(false);
    }
    else
    {
        m_clear->setEnabled(true);
        m_save->setEnabled(true);
    }

}

void UrlCatcher::setUrlList(const QStringList& list)
{
    m_urlListModel = new UrlCatcherModel(this);

    if(!list.isEmpty())
    {

        QList<UrlItem> urlList;
        for (int i=0; i < list.count(); i++)
        {
            UrlItem item;
            item.nick = list.at(i).section(' ',0,0);
            item.url = list.at(i).section(' ',1,1);
            item.datetime = QDateTime::fromString(list.at(i).section(' ',2));
            urlList.append(item);
        }

        m_urlListModel->setUrlList(urlList);

        m_clear->setEnabled(true);
        m_save->setEnabled(true);
    }

    m_proxyModel->setSourceModel(m_urlListModel);

}

void UrlCatcher::addUrl(const QString& who,const QString& url, const QDateTime &datetime)
{
    UrlItem item;
    item.nick = who;
    item.url = url;
    item.datetime = datetime;
    m_urlListModel->append(item);

    m_clear->setEnabled(true);
    m_save->setEnabled(true);
}

void UrlCatcher::urlSelected(const QItemSelection& selected)
{
    if(!selected.isEmpty())
    {
        m_open->setEnabled(true);
        m_saveLink->setEnabled(true);
        m_copy->setEnabled(true);
        m_delete->setEnabled(true);
        m_bookmarkLink->setEnabled(true);
    }
    else
    {
        m_open->setEnabled(false);
        m_saveLink->setEnabled(false);
        m_copy->setEnabled(false);
        m_delete->setEnabled(false);
        m_bookmarkLink->setEnabled(false);
    }
}

void UrlCatcher::openUrl(const QModelIndex& index)
{
    QString url = index.sibling(index.row(), 1).data().toString();

    Application::openUrl(url);
}

void UrlCatcher::openUrlClicked()
{
    QModelIndex index = m_urlListView->selectionModel()->selectedIndexes().first();
    if (index.isValid()) openUrl(index);
}

void UrlCatcher::copyUrlClicked()
{
    QModelIndex index = m_urlListView->selectionModel()->selectedIndexes().first();
    if (index.isValid())
    {
        QString url = index.sibling(index.row(), 1).data().toString();
        QClipboard *cb = qApp->clipboard();
        cb->setText(url, QClipboard::Selection);
        cb->setText(url, QClipboard::Clipboard);
    }
}

void UrlCatcher::deleteUrlClicked()
{
    QModelIndex index = m_urlListView->selectionModel()->selectedIndexes().first();
    if (index.isValid())
    {
        QModelIndex indexAbove;
        QModelIndex indexBelow;
        if(index.row()-1 >= 0)
            indexAbove = index.sibling(index.row()-1, 0);
        if(index.row()+1 < m_proxyModel->rowCount())
            indexBelow = index.sibling(index.row()+1, 0);

        if (indexAbove.isValid())
        {
            QItemSelection selection(indexAbove, indexAbove.sibling(indexAbove.row(),2));
            if(!selection.isEmpty())
                m_urlListView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
        }
        else if (indexBelow.isValid())
        {
            QItemSelection selection(index, index.sibling(index.row(),2));
            if(!selection.isEmpty())
                m_urlListView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
        }
        else
            urlSelected(QItemSelection());

        UrlItem item;
        item.nick = index.sibling(index.row(), 0).data().toString();
        item.url = index.sibling(index.row(), 1).data().toString();
        item.datetime = index.sibling(index.row(), 2).data().toDateTime();

        m_urlListModel->removeRows(index.row(), 0, index);
        if(!m_urlListModel->rowCount())
        {
            urlSelected(QItemSelection());
            m_clear->setEnabled(false);
            m_save->setEnabled(false);
        }

        emit deleteUrl(item.nick, item.url, item.datetime);
    }
}

void UrlCatcher::clearListClicked()
{
    m_urlListModel = new UrlCatcherModel(this);
    m_proxyModel->setSourceModel(m_urlListModel);

    m_save->setEnabled(false);
    m_clear->setEnabled(false);
    urlSelected(QItemSelection());

    emit clearUrlList();
}

void UrlCatcher::saveListClicked()
{
    // Ask user for file name
    QString fileName=KFileDialog::getSaveFileName(
        QString(),
        QString(),
        this,
        i18n("Save URL List"));

    if (!fileName.isEmpty())
    {
        int maxNickWidth=0;

        int rows = m_proxyModel->rowCount();
        QModelIndex index = m_proxyModel->index(0,0,QModelIndex());
        for (int r = 0; r < rows; r++)
        {
            QString nick = index.sibling(r,0).data().toString();

            if (nick.length()>maxNickWidth)
            {
                maxNickWidth = nick.length();
            }
        }

        // now save the list to disk
        QFile listFile(fileName);
        listFile.open(QIODevice::WriteOnly);
        // wrap the file into a stream
        QTextStream stream(&listFile);

        QString header(i18n("Konversation URL List: %1\n\n",
                            QDateTime::currentDateTime().toString()));
        stream << header;

        for (int r = 0; r < rows; r++)
        {
            QString nick = index.sibling(r,0).data().toString();
            QString url = index.sibling(r,1).data().toString();

            QString nickPad;
            nickPad.fill(' ', maxNickWidth);
            nickPad.replace(0, nick.length(), nick);

            QString line(nickPad+' '+url+'\n');
            stream << line;
        }

        listFile.close();
    }
}

void UrlCatcher::contextMenu(const QPoint& p)
{
    QModelIndex item = m_urlListView->indexAt(p);
    if (!item.isValid()) return;

    KMenu* menu = new KMenu(this);

    menu->insertAction(0, m_open);
    menu->insertAction(0, m_saveLink);
    menu->addSeparator();
    menu->insertAction(0, m_copy);
    menu->insertAction(0, m_delete);
    menu->addSeparator();
    menu->insertAction(0, m_save);
    menu->insertAction(0, m_clear);
    menu->addSeparator();
    menu->insertAction(0, m_bookmarkLink);
    menu->exec(QCursor::pos());

    delete menu;
}

void UrlCatcher::bookmarkUrl()
{
    QModelIndex index = m_urlListView->selectionModel()->selectedIndexes().first();
    if (!index.isValid()) return;

    QString url = index.sibling(index.row(), 1).data().toString();

    KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
    KBookmarkDialog* dialog = new KBookmarkDialog(bm, this);
    dialog->addBookmark(url, url);
    delete dialog;
}

void UrlCatcher::saveLinkAs()
{
    QModelIndex index = m_urlListView->selectionModel()->selectedIndexes().first();
    if (!index.isValid()) return;

    QString url = index.sibling(index.row(), 1).data().toString();

    KUrl srcUrl (url);
    KUrl saveUrl = KFileDialog::getSaveUrl(srcUrl.fileName(KUrl::ObeyTrailingSlash), QString(), this, i18n("Save link as"));

    if (saveUrl.isEmpty() || !saveUrl.isValid())
        return;

    KIO::copy(srcUrl, saveUrl);
}

void UrlCatcher::childAdjustFocus()
{
}
#include "urlcatcher.moc"
