/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#include "alias_config.h"
#include "preferences.h"

Alias_Config::Alias_Config(QWidget* parent, const char* name)
 : QWidget(parent)
{
  setObjectName(QString::fromLatin1(name));
  setupUi(this);

  // reset flag to defined state (used to block signals when just selecting a new item)
  m_newItemSelected = false;

  // populate listview
  loadSettings();

  connect(aliasListView, &QTreeWidget::currentItemChanged, this, &Alias_Config::entrySelected);

  connect(aliasInput, &KLineEdit::textChanged, this, &Alias_Config::nameChanged);
  connect(replacementInput, &KLineEdit::textChanged, this, &Alias_Config::actionChanged);

  connect(newButton, &QPushButton::clicked, this, &Alias_Config::addEntry);
  connect(removeButton, &QPushButton::clicked, this, &Alias_Config::removeEntry);
}

Alias_Config::~Alias_Config()
{
}

void Alias_Config::loadSettings()
{
    setAliasListView(Preferences::self()->aliasList());
}

void Alias_Config::saveSettings()
{
    QStringList newList=currentAliasList();
    Preferences::self()->setAliasList(newList);

    // saved list is now old list, to check for changes
    m_oldAliasList=newList;
}

void Alias_Config::restorePageToDefaults()
{
    aliasListView->clear();
    setAliasListView(Preferences::defaultAliasList());
}

bool Alias_Config::hasChanged()
{
    return (currentAliasList() != m_oldAliasList);
}

void Alias_Config::setAliasListView(const QStringList& aliasList)
{
    aliasListView->clear();

    // Insert alias items backwards to get them sorted properly
    for(int index=aliasList.count(); index!=0; index--)
    {
        QString item=aliasList[index-1];
        auto *treeWidgetItem = new QTreeWidgetItem(aliasListView, QStringList {
            item.section(QLatin1Char(' '), 0, 0),
            item.section(QLatin1Char(' '), 1)
        });
        treeWidgetItem->setFlags(treeWidgetItem->flags() &~ Qt::ItemIsDropEnabled);
    }

    aliasListView->setCurrentItem(aliasListView->topLevelItem(0));
    // remember alias list
    m_oldAliasList=aliasList;
}

QStringList Alias_Config::currentAliasList() const
{
    QStringList newList;

    QTreeWidgetItem* item=aliasListView->topLevelItem(0);
    while(item)
        {
        newList.append(item->text(0) + QLatin1Char(' ') + item->text(1));
        item=aliasListView->itemBelow(item);
        }
    return newList;
}

// what to do when the user selects an item
void Alias_Config::entrySelected(QTreeWidgetItem* aliasEntry)
{
    // play it safe, assume disabling all widgets first
    bool enabled = false;

    // check if there really was an item selected
    if (aliasEntry)
    {
        // remember to enable the editing widgets
        enabled = true;
        // tell the editing widgets not to emit modified() on signals now
        m_newItemSelected = true;
        // update editing widget contents
        aliasInput->setText(aliasEntry->text(0));
        replacementInput->setText(aliasEntry->text(1));
        // re-enable modified() signal on text changes in edit widgets
        m_newItemSelected = false;
    }
    // enable or disable editing widgets
    removeButton->setEnabled(enabled);
    aliasLabel->setEnabled(enabled);
    aliasInput->setEnabled(enabled);
    replacementLabel->setEnabled(enabled);
    replacementInput->setEnabled(enabled);
}

// what to do when the user change the name of a quick button
void Alias_Config::nameChanged(const QString& newName)
{
    // get possible first selected item
    QTreeWidgetItem* item = aliasListView->currentItem();

    // sanity check
    if (item)
    {
        // rename item
        item->setText(0,newName);
        // tell the config system that something has changed
        if (!m_newItemSelected) Q_EMIT modified();
    }
}

// what to do when the user change the action definition of a quick button
void Alias_Config::actionChanged(const QString& newAction)
{
    // get possible first selected item
    QTreeWidgetItem* item = aliasListView->currentItem();

    // sanity check
    if (item)
    {
        // rename item
        item->setText(1,newAction);
        // tell the config system that something has changed
        if(!m_newItemSelected) Q_EMIT modified();
    }
}

// add button pressed
void Alias_Config::addEntry()
{
    // add new item at the bottom of list view
    auto* newItem = new QTreeWidgetItem(aliasListView, aliasListView->topLevelItemCount());

    if (newItem)
    {
        newItem->setFlags(newItem->flags() &~ Qt::ItemIsDropEnabled);
        newItem->setText(0, i18n("New"));

        // select new item and make it the current one
        aliasListView->setCurrentItem(newItem);
        // set input focus on item name edit
        aliasInput->setFocus();
        // select all text to make overwriting easier
        aliasInput->selectAll();
        // tell the config system that something has changed
        Q_EMIT modified();
    }
}

// remove button pressed
void Alias_Config::removeEntry()
{
    // get possible first selected item
    QTreeWidgetItem* item = aliasListView->currentItem();

    // sanity check
    if (item)
    {
        // get item below the current one
        QTreeWidgetItem* nextItem = aliasListView->itemBelow(item);
        // if there was none, get the one above
        if(!nextItem) nextItem = aliasListView->itemAbove(item);

        // remove the item from the list
        delete item;

        // check if we found the next item
        if (nextItem)
        {
            // select the item and make it the current item
            aliasListView->setCurrentItem(nextItem);
        }
        else
        {
            // no next item found, this means the list is empty
            entrySelected(nullptr);
        }
        // tell the config system that somethig has changed
        Q_EMIT modified();
    }
}

#include "moc_alias_config.cpp"
