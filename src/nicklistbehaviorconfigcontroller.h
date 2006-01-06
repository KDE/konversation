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

  protected slots:
//    void entrySelected(QListViewItem* quickButtonEntry);
//    void nameChanged(const QString& newName);
//    void actionChanged(const QString& newAction);

  protected:
    void populateQuickButtonsList();
    NicklistBehavior_Config* m_nicklistBehaviorPage;
//    bool newItemSelected;
};

#endif
