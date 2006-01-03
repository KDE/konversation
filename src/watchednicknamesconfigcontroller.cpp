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
#include <kdebug.h>

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
    QStringList nicks=groupIt.data();
    KListViewItem* groupItem=new KListViewItem(m_watchedNicknamesPage->notifyListView,groupIt.key());
    for(unsigned int index=0;index<nicks.count();index++)
    {
      new KListViewItem(groupItem,nicks[index]);
    } // for
    m_watchedNicknamesPage->notifyListView->setOpen(groupItem,true);
  } // for
}

void WatchedNicknamesConfigController::saveSettings()
{
  KConfig* config=kapp->config();
  config->deleteGroup("Notify Groups List");
  config->setGroup("Notify Group Lists");

  QMap<QString, QStringList> notifyList;

  KListView* listView=m_watchedNicknamesPage->notifyListView;
  QListViewItem* group=listView->firstChild();

  while(group)
  {
    QString nicks;
    QListViewItem* nick=group->firstChild();
    kdDebug() << group->text(0) << endl;
    while(nick)
    {
      kdDebug() << "  "+nick->text(0) << endl;
      nicks+=nick->text(0)+" ";
      nick=nick->nextSibling();
    }
    config->writeEntry(group->text(0),nicks.stripWhiteSpace());
    group=group->nextSibling();
  } // while
}

#include "watchednicknamesconfigcontroller.moc"
