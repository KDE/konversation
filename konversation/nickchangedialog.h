/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nickchangedialog.h  -  Shows a small dialog where the user can change their nickname
  begin:     Mon Jul 8 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef NICKCHANGEDIALOG_H
#define NICKCHANGEDIALOG_H

#include <qstringlist.h>
#include <qcombobox.h>

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class NickChangeDialog : public KDialogBase
{
  Q_OBJECT

  public: 
    NickChangeDialog(QWidget* parent,QString currentNick,const QStringList& nickList,QSize initialSize);
    ~NickChangeDialog();

  signals:
    void newNickname(QString newNick);
    void closeDialog(QSize newSize);

  protected slots:
    void slotOk();
    void slotCancel();
    void newNicknameEntered(const QString& newNick);

  protected:
    QComboBox* nicknameInput;
};

#endif
