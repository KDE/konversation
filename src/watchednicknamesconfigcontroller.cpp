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
  connect(m_watchedNicknamesPage->removeButton,SIGNAL (clicked()),this,SLOT (removeNotify()) );
  connect(m_watchedNicknamesPage->notifyListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(m_watchedNicknamesPage->notifyListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );

  connect(m_watchedNicknamesPage->networkDropdown,SIGNAL (activated(const QString&)),this,SLOT (networkChanged(const QString&)) );
  connect(m_watchedNicknamesPage->nicknameInput,SIGNAL (textChanged(const QString&)),this,SLOT (nicknameChanged(const QString&)) );
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

void WatchedNicknamesConfigController::removeNotify()
{
}

void WatchedNicknamesConfigController::entrySelected(QListViewItem* notifyEntry)
{
  bool enabled=false;

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

  m_watchedNicknamesPage->removeButton->setEnabled(enabled);
  m_watchedNicknamesPage->networkLabel->setEnabled(enabled);
  m_watchedNicknamesPage->networkDropdown->setEnabled(enabled);
  m_watchedNicknamesPage->nicknameLabel->setEnabled(enabled);
  m_watchedNicknamesPage->nicknameInput->setEnabled(enabled);
}

void WatchedNicknamesConfigController::networkChanged(const QString& newNetwork)
{
  KListView* listView=m_watchedNicknamesPage->notifyListView;
  QListViewItem* item=listView->selectedItem();

  if(item)
  {
    QListViewItem* group=item->parent();
    if(group && group->text(0)!=newNetwork)
    {
      QListViewItem* lookGroup=listView->firstChild();
      while(lookGroup && (lookGroup->text(0)!=newNetwork)) lookGroup=lookGroup->nextSibling();
      if(lookGroup)
      {
        item->setSelected(false);
        group->takeItem(item);
        lookGroup->insertItem(item);
        item->setSelected(true);
        listView->setCurrentItem(item);
      }
    }
  }
}

void WatchedNicknamesConfigController::nicknameChanged(const QString& newNickname)
{
  kdDebug() << newNickname << endl;
  KListView* listView=m_watchedNicknamesPage->notifyListView;
  QListViewItem* item=listView->selectedItem();

  if(item) item->setText(0,newNickname);
}

#include "watchednicknamesconfigcontroller.moc"
