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
*/

#ifndef PREFSPAGEIDENTITY_H
#define PREFSPAGEIDENTITY_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class QLabel;
class QCheckBox;
class QComboBox;
class QPushButton;

class KLineEdit;
class KComboBox;

class Identity;

class PrefsPageIdentity : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageIdentity(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageIdentity();

  public slots:
    void applyPreferences();

  protected slots:
    void realNameChanged(const QString& newRealName);
    void loginChanged(const QString& newlogin);

    void encodingChanged(int newEncodingIndex);

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
    void updateAwayWidgets(bool state);

    void updateIdentity(int number);
//    void renameIdentity(const QString& newName);
    void renameIdentity();

    void addIdentity();
    void removeIdentity();
    
    void awayNickChanged(const QString& newNick);

  protected:
    QPtrList<Identity> identities;
    Identity* identity;

    KComboBox* identityCombo;

    QStringList encodings;

    QLabel* defaultText;

    KLineEdit* realNameInput;
    KLineEdit* loginInput;

    QComboBox* codecComboBox;

    KLineEdit* nick0;
    KLineEdit* nick1;
    KLineEdit* nick2;
    KLineEdit* nick3;

    KLineEdit* bot;
    KLineEdit* password;

    KLineEdit* partInput;
    KLineEdit* kickInput;

    QCheckBox* showAwayMessageCheck;
    QLabel* awayLabel;
    QLabel* unAwayLabel;
    KLineEdit* awayInput;
    KLineEdit* unAwayInput;
    
    KLineEdit* awayNickInput;

    QPushButton* removeIdentityButton;
};

#endif
