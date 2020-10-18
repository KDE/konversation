/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Gary Cramblitt <garycramblitt@comcast.net>
*/

#ifndef EDITNOTIFYDIALOG_H
#define EDITNOTIFYDIALOG_H

#include <QDialog>

/**
  The EditNotifyDialog implements the dialog for user to add or edit a
  notify.  A notify consists of a server network name and a nickname.
  User must pick the server network name from existing network names
  in the server list.

  @author Gary Cramblitt <garycramblitt@comcast.net>
*/

class KLineEdit;
class KComboBox;

class EditNotifyDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit EditNotifyDialog(QWidget* parent = nullptr, int serverGroupId = 0,
            const QString& nickname=QString());
        ~EditNotifyDialog() override;

        Q_SIGNALS:
        void notifyChanged(int serverGroupId,
            const QString& nickname);

    private Q_SLOTS:
        void slotOk();

    private:
        void delayedDestruct();

    private:
        KComboBox* m_networkNameCombo;
        KLineEdit* m_nicknameInput;

        Q_DISABLE_COPY(EditNotifyDialog)
};

#endif
