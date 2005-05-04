/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Proivides an interface to the notify list
  begin:     Fre Jun 13 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGENOTIFY_H
#define PREFSPAGENOTIFY_H

#include "watchednicknames_preferences.h"

class Preferences;

class PrefsPageNotify : public WatchedNicknames_Config
{
  Q_OBJECT

  public:
    PrefsPageNotify(QWidget* newParent,Preferences* newPreferences);
    ~PrefsPageNotify();

    QMap<QString, QStringList> getNotifyList();

  public slots:
    void applyPreferences();

  private slots:
    void newNotify();
    void createNotify(const QString& networkName, const QString& nickname);
    void removeNotify();
    void notifyCheckChanged(bool enable);
    void slotNotifyListView_SelectionChanged();

  private:
    Preferences* preferences;
    QListViewItem* findBranch(QString name, bool generate);
    QListViewItem* findItemChild(const QListViewItem* parent, const QString& name);
};

#endif
