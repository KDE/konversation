/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#include "quickbuttons_config.h"
#include "preferences.h"

#include <QPushButton>
#include <KSharedConfig>


QuickButtons_Config::QuickButtons_Config(QWidget* parent, const char* name)
 : QWidget(parent)
{
  setObjectName(QString::fromLatin1(name));
  setupUi(this);

  // reset flag to defined state (used to block signals when just selecting a new item)
  m_newItemSelected=false;

  // populate listview
  loadSettings();

  connect(buttonListView, &QTreeWidget::currentItemChanged, this, &QuickButtons_Config::entrySelected);

  connect(nameInput, &KLineEdit::textChanged, this, &QuickButtons_Config::nameChanged);
  connect(actionInput, &KLineEdit::textChanged, this, &QuickButtons_Config::actionChanged);

  connect(newButton, &QPushButton::clicked, this, &QuickButtons_Config::addEntry);
  connect(removeButton, &QPushButton::clicked, this, &QuickButtons_Config::removeEntry);
}

QuickButtons_Config::~QuickButtons_Config()
{
}

void QuickButtons_Config::loadSettings()
{
  setButtonsListView(Preferences::quickButtonList());

  // remember button list for hasChanged()
  m_oldButtonList=Preferences::quickButtonList();
}

// fill listview with button definitions
void QuickButtons_Config::setButtonsListView(const QStringList &buttonList)
{
    buttonListView->clear();

    for (const QString& definition : buttonList) {
        auto *item = new QTreeWidgetItem(buttonListView, QStringList {
            definition.section(QLatin1Char(','), 0, 0),
            definition.section(QLatin1Char(','), 1)
        });

        item->setFlags(item->flags() &~ Qt::ItemIsDropEnabled);
    }

    buttonListView->setCurrentItem(buttonListView->topLevelItem(0));
}

// save quick buttons to configuration
void QuickButtons_Config::saveSettings()
{
  // get configuration object
  KSharedConfigPtr config=KSharedConfig::openConfig();

  // delete all buttons
  config->deleteGroup(QStringLiteral("Button List"));
  // create new empty button group
  KConfigGroup grp = config->group(QStringLiteral("Button List"));

  // create empty list
  QStringList newList=currentButtonList();

  // check if there are any quick buttons in the list view
  if (!newList.isEmpty()) {
    // go through all buttons and save them into the configuration
    for(int index=0;index<newList.count();index++)
    {
      // write the current button's name and definition
      grp.writeEntry(QStringLiteral("Button%1").arg(index),newList[index]);
    } // for
  }
  // if there were no buttons at all, write a dummy entry to prevent KConfigXT from "optimizing"
  // the group out, which would in turn make konvi restore the default buttons
  else
    grp.writeEntry("Empty List",QString());

  // set internal button list
  Preferences::setQuickButtonList(newList);

  // remember button list for hasChanged()
  m_oldButtonList=newList;
}

void QuickButtons_Config::restorePageToDefaults()
{
  setButtonsListView(Preferences::defaultQuickButtonList());
}

QStringList QuickButtons_Config::currentButtonList()
{
  QStringList newList;

  const int buttonCount = buttonListView->topLevelItemCount();
  newList.reserve(buttonCount);
  QTreeWidgetItem* item = nullptr;

  for (int index = 0; index < buttonCount; ++index) {
      item = buttonListView->topLevelItem(index);

      newList.append(item->text(0) + QLatin1Char(',') + item->text(1));
  }

  return newList;
}

bool QuickButtons_Config::hasChanged()
{
  return(m_oldButtonList!=currentButtonList());
}

// slots

// what to do when the user selects an item
void QuickButtons_Config::entrySelected(QTreeWidgetItem* quickButtonEntry)
{
  // play it safe, assume disabling all widgets first
  bool enabled=false;

  // check if there really was an item selected
  if(quickButtonEntry)
  {
    // remember to enable the editing widgets
    enabled=true;

    // tell the editing widgets not to emit modified() on signals now
    m_newItemSelected=true;
    // update editing widget contents
    nameInput->setText(quickButtonEntry->text(0));
    actionInput->setText(quickButtonEntry->text(1));
    // re-enable modified() signal on text changes in edit widgets
    m_newItemSelected=false;
  }
  // enable or disable editing widgets
  removeButton->setEnabled(enabled);
  nameLabel->setEnabled(enabled);
  nameInput->setEnabled(enabled);
  actionLabel->setEnabled(enabled);
  actionInput->setEnabled(enabled);
}

// what to do when the user change the name of a quick button
void QuickButtons_Config::nameChanged(const QString& newName)
{
  // get possible first selected item
  QTreeWidgetItem* item=buttonListView->currentItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(0,newName);
    // tell the config system that something has changed
    if(!m_newItemSelected) Q_EMIT modified();
  }
}

// what to do when the user change the action definition of a quick button
void QuickButtons_Config::actionChanged(const QString& newAction)
{
  // get possible first selected item
  QTreeWidgetItem* item=buttonListView->currentItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(1,newAction);
    // tell the config system that something has changed
    if(!m_newItemSelected) Q_EMIT modified();
  }
}

// add button pressed
void QuickButtons_Config::addEntry()
{
  // add new item at the bottom of list view
  auto* newItem = new QTreeWidgetItem(buttonListView, buttonListView->topLevelItemCount());

  if (newItem)
  {
    newItem->setFlags(newItem->flags() &~ Qt::ItemIsDropEnabled);
    newItem->setText(0, i18n("New"));

    // select new item and make it the current one
    buttonListView->setCurrentItem(newItem);
    // set input focus on item name edit
    nameInput->setFocus();
    // select all text to make overwriting easier
    nameInput->selectAll();
    // tell the config system that something has changed
    Q_EMIT modified();
  }
}

// remove button pressed
void QuickButtons_Config::removeEntry()
{
  // get possible first selected item
  QTreeWidgetItem* item=buttonListView->currentItem();

  // sanity check
  if(item)
  {
    // get item below the current one
    QTreeWidgetItem* nextItem=buttonListView->itemBelow(item);
    // if there was none, get the one above
    if(!nextItem) nextItem=buttonListView->itemAbove(item);

    // remove the item from the list
    delete item;

    // check if we found the next item
    if(nextItem)
    {
      // select the item and make it the current item
      buttonListView->setCurrentItem(nextItem);
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

#include "moc_quickbuttons_config.cpp"
