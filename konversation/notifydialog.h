/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  notifydialog.h  -  description
  begin:     Sam Jul 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef NOTIFYDIALOG_H
#define NOTIFYDIALOG_H

#include <qlineedit.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qspinbox.h>

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class NotifyDialog : public KDialogBase
{
  Q_OBJECT

  public:
    NotifyDialog(QStringList newNotifyList,QSize newSize,bool use,int delay);
    ~NotifyDialog();

  protected:
    QStringList getNotifyList();

  signals:
    void applyClicked(QStringList newList,bool use,int delay);
    void cancelClicked(QSize newSize);

  protected slots:
    void slotOk();
    void slotApply();
    void slotCancel();
    void newNotify();
    void removeNotify();

  protected:
    KListView* notifyListView;
    QLineEdit* notifyInput;
    QPushButton* newButton;
    QPushButton* removeButton;
    QCheckBox* useNotifyCheck;
    QSpinBox* notifyDelaySpin;
};

#endif
