/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
*/

#include "nicklistbehavior_config.h"
#include "preferences.h"


NicklistBehavior_Config::NicklistBehavior_Config(QWidget *parent, const char *name)
 : QWidget(parent)
{
  setObjectName(QString::fromLatin1(name));
  setupUi(this);

  // get page widget and populate listview
  loadSettings();
}

NicklistBehavior_Config::~NicklistBehavior_Config()
{
}

void NicklistBehavior_Config::restorePageToDefaults()
{
  setNickList(Preferences::defaultNicknameSortingOrder());
}

void NicklistBehavior_Config::loadSettings()
{
  // get sorting order string from preferences
  setNickList(Preferences::self()->sortOrder());
  m_oldSortingOrder=currentSortingOrder();
}

void NicklistBehavior_Config::setNickList(const QString &sortingOrder)
{
  sortOrder->clear();
  for (QChar mode : sortingOrder) {
    QTreeWidgetItem *item = nullptr;
    // find appropriate description
    if (mode == QLatin1Char('-')) item = new QTreeWidgetItem(sortOrder, QStringList { mode, i18n("Normal Users") });
    if (mode == QLatin1Char('v')) item = new QTreeWidgetItem(sortOrder, QStringList { mode, i18n("Voice (+v)") });
    if (mode == QLatin1Char('h')) item = new QTreeWidgetItem(sortOrder, QStringList { mode, i18n("Halfops (+h)") });
    if (mode == QLatin1Char('o')) item = new QTreeWidgetItem(sortOrder, QStringList { mode, i18n("Operators (+o)") });
    if (mode == QLatin1Char('p')) item = new QTreeWidgetItem(sortOrder, QStringList { mode, i18n("Channel Admins (+p)") });
    if (mode == QLatin1Char('q')) item = new QTreeWidgetItem(sortOrder, QStringList { mode, i18n("Channel Owners (+q)") });
    item->setFlags(item->flags() &~ Qt::ItemIsDropEnabled);
  }
}

QString NicklistBehavior_Config::currentSortingOrder() const
{
  // get the uppermost entry of the sorting list
  QTreeWidgetItem* item=sortOrder->topLevelItem(0);
  // prepare the new sorting order string
  QString currentSortingOrder;
  // iterate through all items of the listview
  while(item)
  {
    // add mode char to the sorting order string
    currentSortingOrder+=item->text(0);
    // go to next item in the listview
    item=sortOrder->itemBelow(item);
  } // while

  return currentSortingOrder;
}

// save settings permanently
void NicklistBehavior_Config::saveSettings()
{
  // get the current sorting order
  QString newSortingOrder=currentSortingOrder();

  // update sorting order on in-memory preferences
  Preferences::self()->setSortOrder(newSortingOrder);

  // save current sorting order as a reference to hasChanged()
  m_oldSortingOrder=currentSortingOrder();
}

bool NicklistBehavior_Config::hasChanged()
{
  return(m_oldSortingOrder!=currentSortingOrder());
}

#include "moc_nicklistbehavior_config.cpp"
