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

UrlCatcherModel::UrlCatcherModel(QObject* parent) : QAbstractListModel(parent)
{
}

bool operator==(const UrlItem& item, const UrlItem& item2)
{
    return (item.nick == item2.nick && item.url == item2.url);
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
    QModelIndex bottomRight = parent.sibling(last,1);

    bool success = true;
    for(int i=row; i <= last; i++)
    {
        UrlItem item;
        item.nick = parent.sibling(i,0).data().toString();
        item.url = parent.sibling(i,1).data().toString();
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
    return 2;
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
        default:
            return QVariant();
    }
}

UrlCatcher::UrlCatcher(QWidget* parent) : ChatWindow(parent)
{
    setName(i18n("URL Catcher"));
    setType(ChatWindow::UrlCatcher);

    setupUi(this);

    m_urlListModel = new UrlCatcherModel(this);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_urlListModel);
    m_urlListView->setModel(m_proxyModel);

    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setFilterKeyColumn(-1);

    m_filterTimer = new QTimer(this);
    m_filterTimer->setSingleShot(true);

    connect(m_urlListView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(urlSelected(const QItemSelection&)));
    connect(m_urlListView, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(openUrl(const QModelIndex&)) );
    connect(m_urlListView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(contextMenu(const QPoint&)) );

    connect(m_openBtn, SIGNAL(clicked()), this, SLOT(openUrlClicked()));
    connect(m_copyBtn, SIGNAL(clicked()), this, SLOT(copyUrlClicked()));
    connect(m_deleteBtn, SIGNAL(clicked()), this, SLOT(deleteUrlClicked()));
    connect(m_saveBtn, SIGNAL(clicked()), this, SLOT(saveListClicked()));
    connect(m_clearBtn, SIGNAL(clicked()), this, SLOT(clearListClicked()));

    //TODO remove this by turning the klineedit into a kfilterproxysearchline when we req. 4.2
    connect(m_filterLine, SIGNAL(textChanged(const QString&)), this, SLOT(filterChanged()));
    connect(m_filterTimer, SIGNAL(timeout()), this, SLOT(updateFilter()));
}


UrlCatcher::~UrlCatcher()
{
}

void UrlCatcher::filterChanged()
{
    m_filterTimer->start(300);
}

void UrlCatcher::updateFilter()
{
    m_proxyModel->setFilterWildcard(m_filterLine->text());
    if(!m_proxyModel->rowCount())
    {
        m_clearBtn->setEnabled(false);
        m_saveBtn->setEnabled(false);
    }
    else
    {
        m_clearBtn->setEnabled(true);
        m_saveBtn->setEnabled(true);
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
            urlList.append(item);
        }

        m_urlListModel->setUrlList(urlList);

        m_clearBtn->setEnabled(true);
        m_saveBtn->setEnabled(true);
    }

    m_proxyModel->setSourceModel(m_urlListModel);

}

void UrlCatcher::addUrl(const QString& who,const QString& url)
{
    UrlItem item;
    item.nick = who;
    item.url = url;
    m_urlListModel->append(item);

    m_clearBtn->setEnabled(true);
    m_saveBtn->setEnabled(true);
}

void UrlCatcher::urlSelected(const QItemSelection& selected)
{
    if(!selected.isEmpty())
    {
        m_openBtn->setEnabled(true);
        m_copyBtn->setEnabled(true);
        m_deleteBtn->setEnabled(true);
    }
    else
    {
        m_openBtn->setEnabled(false);
        m_copyBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
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
            QItemSelection selection(indexAbove, indexAbove.sibling(indexAbove.row(),1));
            if(!selection.isEmpty())
                m_urlListView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
        }
        else if (indexBelow.isValid())
        {
            QItemSelection selection(index, index.sibling(index.row(),1));
            if(!selection.isEmpty())
                m_urlListView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
        }
        else
            urlSelected(QItemSelection());

        UrlItem item;
        item.nick = index.sibling(index.row(), 0).data().toString();
        item.url = index.sibling(index.row(), 1).data().toString();

        m_urlListModel->removeRows(index.row(), 0, index);
        if(!m_urlListModel->rowCount())
        {
            urlSelected(QItemSelection());
            m_clearBtn->setEnabled(false);
            m_saveBtn->setEnabled(false);
        }

        emit deleteUrl(item.nick, item.url);
    }
}

void UrlCatcher::clearListClicked()
{
    m_urlListModel = new UrlCatcherModel(this);
    m_proxyModel->setSourceModel(m_urlListModel);

    m_saveBtn->setEnabled(false);
    m_clearBtn->setEnabled(false);
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

    menu->addAction(KIcon("edit-copy"), i18n("Copy Link Address"), this, SLOT (copyUrlClicked()));
    menu->addAction(KIcon("bookmark-new"), i18n("Add to Bookmarks"), this, SLOT (bookmarkUrl()));
    menu->addAction(KIcon("document-save"), i18n("Save Link As..."), this, SLOT(saveLinkAs()));
    //TODO maybe a delete action?

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
