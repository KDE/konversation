/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageidentity.h  -  Provides a user interface to customize identity settings
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef PREFSPAGEIDENTITY_H
#define PREFSPAGEIDENTITY_H

#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <klineedit.h>
#include <kcombobox.h>

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class PrefsPageIdentity : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageIdentity(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageIdentity();

  protected slots:
    void realNameChanged(const QString& newRealName);
    void loginChanged(const QString& newlogin);

    void nick0Changed(const QString& newNick);
    void nick1Changed(const QString& newNick);
    void nick2Changed(const QString& newNick);
    void nick3Changed(const QString& newNick);

    void botChanged(const QString& newBot);
    void passwordChanged(const QString& newPassword);

    void partReasonChanged(const QString& newReason);
    void kickReasonChanged(const QString& newReason);

    void showAwayMessageChanged(int state);
    void awayMessageChanged(const QString& newMessage);
    void unAwayMessageChanged(const QString& newMessage);

    void updateIdentity(int number);
//    void renameIdentity(const QString& newName);
    void renameIdentity();

    void addIdentity();
    void removeIdentity();

  protected:
    QPtrList<Identity> identities;
    Identity* identity;

    KComboBox* identityCombo;

    QLabel* defaultText;

    KLineEdit* realNameInput;
    KLineEdit* loginInput;

    KLineEdit* nick0;
    KLineEdit* nick1;
    KLineEdit* nick2;
    KLineEdit* nick3;

    KLineEdit* bot;
    KLineEdit* password;

    KLineEdit* partInput;
    KLineEdit* kickInput;

    QCheckBox* showAwayMessageCheck;
    KLineEdit* awayInput;
    KLineEdit* unAwayInput;
    
    QPushButton* removeIdentityButton;
};

#endif
