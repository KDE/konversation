/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  quickbuttonsdialog.h  -  description
  begin:     Mon Jun 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef QUICKBUTTONSDIALOG_H
#define QUICKBUTTONSDIALOG_H

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class QuickButtonsDialog : public KDialogBase
{
  Q_OBJECT

  public:
    QuickButtonsDialog(QStringList& buttonList,QSize& size);
    ~QuickButtonsDialog();

  protected:
    QStringList getButtonList();

  signals:
    void applyClicked(QStringList newList);
    void cancelClicked(QSize newButtonsSize);

  protected slots:
    void slotOk();
    void slotApply();
    void slotCancel();

  protected:
    KListView* buttonListView;
};

#endif
