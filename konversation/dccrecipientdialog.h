/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dccrecipientdialog.h  -  lets the user choose a nick from the list
  begin:     Sam Dez 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef DCCRECIPIENTDIALOG_H
#define DCCRECIPIENTDIALOG_H

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class DccRecipientDialog : public KDialogBase
{
  Q_OBJECT

  public: 
    DccRecipientDialog(QWidget* parent,QStringList list,QSize size);
    ~DccRecipientDialog();

    static QString getNickname(QWidget* parent,QStringList list);

  protected slots:
    void newNicknameSelected(QListBoxItem* item);
    void newNicknameSelectedQuit(QListBoxItem* item);  // KDE double click

    void slotCancel();

  protected:
    QString getSelectedNickname();
    static QString selectedNickname;

};

#endif
