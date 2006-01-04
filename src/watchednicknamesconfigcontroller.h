//
// C++ Interface: watchednicknamesconfigcontroller
//
// Description: 
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef WATCHEDNICKNAMESCONFIGCONTROLLER_H
#define WATCHEDNICKNAMESCONFIGCONTROLLER_H

#include <qobject.h>

class WatchedNicknames_Config;
class QListViewItem;

/**
  @author Dario Abatianni
*/

class WatchedNicknamesConfigController : public QObject
{
  Q_OBJECT

  public:
    WatchedNicknamesConfigController(WatchedNicknames_Config* watchedNicknamesPage,QObject *parent = 0, const char *name = 0);
    ~WatchedNicknamesConfigController();

    void saveSettings();

  signals:
    void modified();

  protected slots:
    void checkIfEmptyListview(bool state);
    void newNotify();
    void removeNotify();
    void entrySelected(QListViewItem* notifyEntry);
    void networkChanged(const QString& newNetwork);
    void nicknameChanged(const QString& newNickname);

  protected:
    void populateWatchedNicksList();
    WatchedNicknames_Config* m_watchedNicknamesPage;
};

#endif
