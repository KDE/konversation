/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagenotify.h  -  Proivides an interface to the notify list
  begin:     Fre Jun 13 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGENOTIFY_H
#define PREFSPAGENOTIFY_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class QLineEdit;
class QPushButton;
class QCheckBox;
class QLabel;
class QSpinBox;

class KListView;

class PrefsPageNotify : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageNotify(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageNotify();

    QStringList getNotifyList();

  public slots:
    void applyPreferences();

  protected slots:
    void newNotify();
    void removeNotify();
    void notifyCheckChanged(int state);

  protected:
    KListView* notifyListView;
    QLineEdit* notifyInput;
    QPushButton* newButton;
    QPushButton* removeButton;
    QCheckBox* useNotifyCheck;
    QLabel* notifyDelayLabel;
    QSpinBox* notifyDelaySpin;
};

#endif
