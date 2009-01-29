/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Wed Sep 1 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

#ifndef EDITNOTIFYDIALOG_H
#define EDITNOTIFYDIALOG_H

#include <kdialog.h>

/**
  The EditNotifyDialog implements the dialog for user to add or edit a
  notify.  A notify consists of a server network name and a nickname.
  User must pick the server network name from existing network names
  in the server list.

  @author Gary Cramblitt <garycramblitt@comcast.net>
*/

class KLineEdit;
class KComboBox;

class EditNotifyDialog : public KDialog
{
    Q_OBJECT

    public:
        explicit EditNotifyDialog(QWidget* parent=0,const QString& network=QString(),
            const QString& nickname=QString());
        ~EditNotifyDialog();

        signals:
        void notifyChanged(const QString& network,
            const QString& nickname);
    protected slots:
        void slotOk();

    protected:
        KComboBox* m_networkNameCombo;
        KLineEdit* m_nicknameInput;
};
#endif
