/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ignoredialog.h  -  description
  begin:     Mon Jun 24 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IGNOREDIALOG_H
#define IGNOREDIALOG_H

#include <qlineedit.h>
#include <qcheckbox.h>

#include <kdialogbase.h>

#include "ignore.h"
#include "ignorelistviewitem.h"
#include "ignorecheckbox.h"

/*
  @author Dario Abatianni
*/

class IgnoreDialog : public KDialogBase
{
  Q_OBJECT

  public:
    IgnoreDialog(QPtrList<Ignore> newIgnoreList,QSize newSize);
    ~IgnoreDialog();

  protected:
    QPtrList<Ignore> getIgnoreList();

  signals:
    void applyClicked(QPtrList<Ignore> newList);
    void cancelClicked(QSize newSize);

  protected slots:
    void slotOk();
    void slotApply();
    void slotCancel();
    void newIgnore();
    void removeIgnore();
    void select(QListViewItem* item);
    void checked(int flag,bool active);

  protected:
    KListView* ignoreListView;
    QLineEdit* ignoreInput;
    QPushButton* newButton;
    QPushButton* removeButton;
    QPtrList<IgnoreCheckBox> checkList;
};

#endif
