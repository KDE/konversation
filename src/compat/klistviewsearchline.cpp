/* This file is part of the KDE libraries
   Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "klistviewsearchline.h"

#include <klistview.h>
#include <kdebug.h>
#include <klocale.h>

#include <qtimer.h>
#include <qpopupmenu.h>

#define KLISTVIEWSEARCHLINE_ALLCOLUMNS_ID 2004

class KListViewSearchLine::KListViewSearchLinePrivate
{
public:
    KListViewSearchLinePrivate() :
        listView(0),
        caseSensitive(false),
        activeSearch(false),
        keepParentsVisible(true),
        queuedSearches(0) {}

    KListView *listView;
    bool caseSensitive;
    bool activeSearch;
    bool keepParentsVisible;
    QString search;
    int queuedSearches;
    QValueList<int> searchColumns;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

KListViewSearchLine::KListViewSearchLine(QWidget *parent, KListView *listView, const char *name) :
    KLineEdit(parent, name)
{
    d = new KListViewSearchLinePrivate;

    d->listView = listView;

    connect(this, SIGNAL(textChanged(const QString &)),
            this, SLOT(queueSearch(const QString &)));

    if(listView) {
        connect(listView, SIGNAL(destroyed()),
                this, SLOT(listViewDeleted()));

        connect(listView, SIGNAL(itemAdded(QListViewItem *)),
                this, SLOT(itemAdded(QListViewItem *)));
    }
    else
        setEnabled(false);
}

KListViewSearchLine::KListViewSearchLine(QWidget *parent, const char *name) :
    KLineEdit(parent, name)
{
    d = new KListViewSearchLinePrivate;

    d->listView = 0L;

    connect(this, SIGNAL(textChanged(const QString &)),
            this, SLOT(queueSearch(const QString &)));

    setEnabled(false);
}

KListViewSearchLine::~KListViewSearchLine()
{
    delete d;
}

bool KListViewSearchLine::caseSensitive() const
{
    return d->caseSensitive;
}

QValueList<int> KListViewSearchLine::searchColumns() const
{
    return d->searchColumns;
}

bool KListViewSearchLine::keepParentsVisible() const
{
    return d->keepParentsVisible;
}

KListView *KListViewSearchLine::listView() const
{
    return d->listView;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void KListViewSearchLine::updateSearch(const QString &s)
{
    if(!d->listView)
        return;

    d->search = s.isNull() ? text() : s;

    // If there's a selected item that is visible, make sure that it's visible
    // when the search changes too (assuming that it still matches).

    QListViewItem *currentItem = 0;

    switch(d->listView->selectionMode())
    {
    case KListView::NoSelection:
        break;
    case KListView::Single:
        currentItem = d->listView->selectedItem();
        break;
    default:
    {
        int flags = QListViewItemIterator::Selected | QListViewItemIterator::Visible;
        for(QListViewItemIterator it(d->listView, flags);
            it.current() && !currentItem;
            ++it)
        {
            if(d->listView->itemRect(it.current()).isValid())
                currentItem = it.current();
        }
    }
    }

    if(d->keepParentsVisible)
        checkItemParentsVisible(d->listView->firstChild());
    else
        checkItemParentsNotVisible();

    if(currentItem)
        d->listView->ensureItemVisible(currentItem);
}

void KListViewSearchLine::setCaseSensitive(bool cs)
{
    d->caseSensitive = cs;
}

void KListViewSearchLine::setKeepParentsVisible(bool v)
{
    d->keepParentsVisible = v;
}

void KListViewSearchLine::setSearchColumns(const QValueList<int> &columns)
{
    d->searchColumns = columns;
}

void KListViewSearchLine::setListView(KListView *lv)
{
    if(d->listView) {
        disconnect(d->listView, SIGNAL(destroyed()),
                   this, SLOT(listViewDeleted()));

        disconnect(d->listView, SIGNAL(itemAdded(QListViewItem *)),
                   this, SLOT(itemAdded(QListViewItem *)));
    }

    d->listView = lv;

    if(lv) {
        connect(d->listView, SIGNAL(destroyed()),
                this, SLOT(listViewDeleted()));

        connect(d->listView, SIGNAL(itemAdded(QListViewItem *)),
                this, SLOT(itemAdded(QListViewItem *)));
    }

    setEnabled(bool(lv));
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

bool KListViewSearchLine::itemMatches(const QListViewItem *item, const QString &s) const
{
    if(s.isEmpty())
        return true;

    // If the search column list is populated, search just the columns
    // specifified.  If it is empty default to searching all of the columns.

    if(!d->searchColumns.isEmpty()) {
        QValueList<int>::ConstIterator it = d->searchColumns.begin();
        for(; it != d->searchColumns.end(); ++it) {
            if(*it < item->listView()->columns() &&
               item->text(*it).find(s, 0, d->caseSensitive) >= 0)
                return true;
        }
    }
    else {
        for(int i = 0; i < item->listView()->columns(); i++) {
            if(item->text(i).find(s, 0, d->caseSensitive) >= 0)
                return true;
        }
    }

    return false;
}

QPopupMenu *KListViewSearchLine::createPopupMenu()
{
    QPopupMenu *popup = KLineEdit::createPopupMenu();

    QPopupMenu *subMenu = new QPopupMenu( popup );
    connect( subMenu, SIGNAL( activated(int) ), this, SLOT( searchColumnsMenuActivated(int) ) );

    popup->insertSeparator();
    popup->insertItem( i18n("Search Columns"), subMenu );
    
    subMenu->insertItem(i18n("All Columns"), KLISTVIEWSEARCHLINE_ALLCOLUMNS_ID);
    subMenu->insertSeparator();    
    
    bool allColumnsAreSearchColumns = true;
    for(int i = 0; i < d->listView->columns(); i++) {
        subMenu->insertItem(d->listView->columnText(i), i);
        if (d->searchColumns.isEmpty() || d->searchColumns.find(i) != d->searchColumns.end())
            subMenu->setItemChecked(i, true);
        else
            allColumnsAreSearchColumns = false;
    }
    subMenu->setItemChecked(KLISTVIEWSEARCHLINE_ALLCOLUMNS_ID, allColumnsAreSearchColumns);
    
    // searchColumnsMenuActivated() relies on one possible "all" representation
    if (allColumnsAreSearchColumns && !d->searchColumns.isEmpty())
      d->searchColumns.clear();
    
    return popup;   
}    

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void KListViewSearchLine::queueSearch(const QString &search)
{
    d->queuedSearches++;
    d->search = search;
    QTimer::singleShot(200, this, SLOT(activateSearch()));
}

void KListViewSearchLine::activateSearch()
{
    d->queuedSearches--;

    if(d->queuedSearches == 0)
        updateSearch(d->search);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void KListViewSearchLine::itemAdded(QListViewItem *item) const
{
    item->setVisible(itemMatches(item, text()));
}

void KListViewSearchLine::listViewDeleted()
{
    d->listView = 0;
    setEnabled(false);
}

void KListViewSearchLine::searchColumnsMenuActivated(int id)
{
    if (id==KLISTVIEWSEARCHLINE_ALLCOLUMNS_ID) {
      if (d->searchColumns.isEmpty())
          d->searchColumns.append(0);
      else
          d->searchColumns.clear();
    }
    else {
      if (d->searchColumns.find(id) != d->searchColumns.end())
          d->searchColumns.remove(id);
      else {
          if (d->searchColumns.isEmpty())
             for(int i = 0; i < d->listView->columns(); i++) {
                 if (i!=id)
                     d->searchColumns.append(i);
             }
          else
              d->searchColumns.append(id);
      }
    }
    updateSearch();
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void KListViewSearchLine::checkItemParentsNotVisible()
{
    QListViewItemIterator it(d->listView);
    for(; it.current(); ++it)
    {
        QListViewItem *item = it.current();
        if(itemMatches(item, d->search))
            item->setVisible(true);
        else
            item->setVisible(false);
    }
}

bool KListViewSearchLine::checkItemParentsVisible(QListViewItem *item)
{
    bool visible = false;
    for(; item; item = item->nextSibling()) {
        if((item->firstChild() && checkItemParentsVisible(item->firstChild())) ||
           itemMatches(item, d->search))
        {
            item->setVisible( true );
            visible = true;
        }
        else
            item->setVisible(false);
    }
    return visible;
}

#include "klistviewsearchline.moc"
