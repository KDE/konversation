//
// C++ Interface: nicklistbehaviorconfigcontroller
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef NICKLISTBEHAVIORCONFIGCONTROLLER_H
#define NICKLISTBEHAVIORCONFIGCONTROLLER_H

#include <qobject.h>

class NicklistBehavior_Config;

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
 */

class NicklistBehaviorConfigController : public QObject
{
  Q_OBJECT

  public:
    NicklistBehaviorConfigController(NicklistBehavior_Config* nicklistBehaviorPage,QObject *parent = 0, const char *name = 0);
    ~NicklistBehaviorConfigController();

    void saveSettings();

  signals:
    void modified();

  protected:
    void populateSortingList();
    NicklistBehavior_Config* m_nicklistBehaviorPage;
};

#endif
