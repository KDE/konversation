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

#include <qlabel.h>
#include <qcombobox.h>

#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <kdebug.h>

#include "config/preferences.h"

#include "watchednicknames_preferences.h"
#include "watchednicknamesconfigcontroller.h"

WatchedNicknamesConfigController::WatchedNicknamesConfigController(WatchedNicknames_Config* watchedNicknamesPage,QObject *parent, const char *name)
 : QObject(parent, name)
{
  m_watchedNicknamesPage=watchedNicknamesPage;
  populateWatchedNicksList();

  connect(m_watchedNicknamesPage->newButton,SIGNAL (clicked()),this,SLOT (newNotify()) );
  connect(m_watchedNicknamesPage->notifyListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(m_watchedNicknamesPage->notifyListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
}

WatchedNicknamesConfigController::~WatchedNicknamesConfigController()
{
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
    m_watchedNicknamesPage->networkDropdown->insertItem(groupIt.key(),-1);
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
    while(nick)
    {
      nicks+=nick->text(0)+" ";
      nick=nick->nextSibling();
    }
    config->writeEntry(group->text(0),nicks.stripWhiteSpace());
    group=group->nextSibling();
  } // while
}

// slots

void WatchedNicknamesConfigController::newNotify()
{
}

void WatchedNicknamesConfigController::entrySelected(QListViewItem* notifyEntry)
{
  bool enabled=false;

  KListView* listView=m_watchedNicknamesPage->notifyListView;

  if(notifyEntry)
  {
    QListViewItem* group=notifyEntry->parent();
    if(group)
    {
      enabled=true;
      m_watchedNicknamesPage->nicknameInput->setText(notifyEntry->text(0));
      m_watchedNicknamesPage->networkDropdown->setCurrentText(group->text(0));
    }
  }

  m_watchedNicknamesPage->networkLabel->setEnabled(enabled);
  m_watchedNicknamesPage->networkDropdown->setEnabled(enabled);
  m_watchedNicknamesPage->nicknameLabel->setEnabled(enabled);
  m_watchedNicknamesPage->nicknameInput->setEnabled(enabled);
}

#include "watchednicknamesconfigcontroller.moc"
