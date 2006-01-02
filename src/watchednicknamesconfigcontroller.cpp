//
// C++ Implementation: watchednicknamesconfigcontroller
//
// Description: 
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>

#include "config/preferences.h"

#include "watchednicknames_preferences.h"
#include "watchednicknamesconfigcontroller.h"

WatchedNicknamesConfigController::WatchedNicknamesConfigController(WatchedNicknames_Config* watchedNicknamesPage,QObject *parent, const char *name)
 : QObject(parent, name)
{
  m_watchedNicknamesPage=watchedNicknamesPage;
  populateWatchedNicksList();
}

WatchedNicknamesConfigController::~WatchedNicknamesConfigController()
{
  KConfig* config=kapp->config();
}

void WatchedNicknamesConfigController::populateWatchedNicksList()
{
    QMap<QString, QStringList> notifyList = Preferences::notifyList();
    QMapConstIterator<QString, QStringList> groupItEnd = notifyList.constEnd();

    for (QMapConstIterator<QString, QStringList> groupIt = notifyList.constBegin();
        groupIt != groupItEnd; ++groupIt)
    {
        new KListViewItem(m_watchedNicknamesPage->notifyListView,groupIt.key());
      // groupIt.data().join(" "));
    }
}

void WatchedNicknamesConfigController::saveSettings()
{
    KConfig* config=kapp->config();

    config->setGroup("Notify Group Lists");
    QMap<QString, QStringList> notifyList = Preferences::notifyList();
    QMapConstIterator<QString, QStringList> groupItEnd = notifyList.constEnd();

    for (QMapConstIterator<QString, QStringList> groupIt = notifyList.constBegin();
        groupIt != groupItEnd; ++groupIt)
    {
        config->writeEntry(groupIt.key(), groupIt.data().join(" "));
    }
}

#include "watchednicknamesconfigcontroller.moc"
