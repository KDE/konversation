/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONIDENTITYDIALOG_H
#define KONVERSATIONIDENTITYDIALOG_H

#include <kdialogbase.h>

#include "identity.h"

class KComboBox;
class KLineEdit;
class QCheckBox;
class QListBox;
class QGroupBox;
class QToolButton;

namespace Konversation {

class IdentityDialog : public KDialogBase
{
  Q_OBJECT
  public:
    IdentityDialog(QWidget *parent = 0, const char *name = 0);
    ~IdentityDialog();
    void setCurrentIdentity(int index);

  protected slots:
    void updateIdentity(int index);

    void addNickname();
    void editNickname();
    void deleteNickname();
    void moveNicknameUp();
    void moveNicknameDown();

    void refreshCurrentIdentity();

    void slotOk();

    void newIdentity();
    void renameIdentity();
    void deleteIdentity();
    void copyIdentity();

  private:
    KComboBox* m_identityCBox;
    KLineEdit* m_realNameEdit;
    KLineEdit* m_loginEdit;
    KComboBox* m_codecCBox;
    KLineEdit* m_botEdit;
    KLineEdit* m_passwordEdit;
    KLineEdit* m_partEdit;
    KLineEdit* m_kickEdit;
    KLineEdit* m_awayEdit;
    KLineEdit* m_unAwayEdit;
    KLineEdit* m_awayNickEdit;
    QCheckBox* m_insertRememberLineOnAwayChBox;
    QListBox* m_nicknameLBox;
    QGroupBox* m_awayMessageGBox;
    QCheckBox* m_showAwayMessage;
    QToolButton* m_editBtn;
    QToolButton* m_delBtn;

    QValueList<IdentityPtr> m_identityList;
    IdentityPtr m_currentIdentity;
};

}

#endif
