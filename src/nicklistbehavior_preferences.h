//
// C++ Interface: nicklistbehavior_Config
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef NICKLISTBEHAVIOR_Config_H
#define NICKLISTBEHAVIOR_Config_H

#include <qobject.h>
#include "konvisettingspage.h"
#include "nicklistbehavior_preferencesui.h"

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
 */

class NicklistBehavior_Config : public NicklistBehavior_ConfigUI, public KonviSettingsPage
{
  Q_OBJECT

  public:
    NicklistBehavior_Config(QWidget *parent = 0, const char *name = 0);
    ~NicklistBehavior_Config();

    virtual void saveSettings();
    virtual void loadSettings();
    virtual void restorePageToDefaults();

    virtual bool hasChanged();

  private:
    void setNickList(const QString &sortingOrder);
    QString currentSortingOrder();

    QString m_oldSortingOrder;

  signals:
    void modified();

};

#endif
