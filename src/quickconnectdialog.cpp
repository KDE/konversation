/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Dialog for quick connection to an IRC network without adding a server in the Server List.
  begin:     Sat June 05 2004
  copyright: (C) 2004 by Michael Goettsche
  email:     mail@tuxipuxi.de
*/

#include "quickconnectdialog.h"
//Added by qt3to4:
#include <Q3GridLayout>
#include "konversationapplication.h"

#include <qlayout.h>
#include <q3whatsthis.h>
#include <qlabel.h>
#include <qcheckbox.h>

#include <klineedit.h>
#include <kpassdlg.h>
#include <klocale.h>


QuickConnectDialog::QuickConnectDialog(QWidget *parent)
:KDialogBase(parent, "quickconnect", true, i18n("Quick Connect"),
KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, true)
{
    QWidget* page = new QWidget(this);
    setMainWidget(page);

    Q3GridLayout* layout = new Q3GridLayout(page, 2, 4);
    layout->setSpacing(spacingHint());
    layout->setColStretch(1, 10);

    QLabel* hostNameLabel = new QLabel(i18n("&Server host:"), page);
    QString hostNameWT = i18n("Enter the host of the network here.");
    Q3WhatsThis::add(hostNameLabel, hostNameWT);
    hostNameInput = new KLineEdit(page);
    Q3WhatsThis::add(hostNameInput, hostNameWT);
    hostNameLabel->setBuddy(hostNameInput);

    QLabel* portLabel = new QLabel(i18n("&Port:"), page);
    QString portWT = i18n("The port that the IRC server is using.");
    Q3WhatsThis::add(portLabel, portWT);
    portInput = new KLineEdit("6667", page );
    Q3WhatsThis::add(portInput, portWT);
    portLabel->setBuddy(portInput);

    QLabel* nickLabel = new QLabel(i18n("&Nick:"), page);
    QString nickWT = i18n("The nick you want to use.");
    Q3WhatsThis::add(nickLabel, nickWT);
    nickInput = new KLineEdit(Preferences::nickname(0), page);
    Q3WhatsThis::add(nickInput, nickWT);
    nickLabel->setBuddy(nickInput);

    QLabel* passwordLabel = new QLabel(i18n("P&assword:"), page);
    QString passwordWT = i18n("If the IRC server requires a password, enter it here (most servers do not require a password.)");
    Q3WhatsThis::add(passwordLabel, passwordWT);
    passwordInput = new KPasswordEdit(page);
    Q3WhatsThis::add(passwordInput, passwordWT);
    passwordLabel->setBuddy(passwordInput);

    sslCheckBox = new QCheckBox(page, "sslCheckBox");
    sslCheckBox->setText(i18n("&Use SSL"));

    layout->addWidget(hostNameLabel, 0, 0);
    layout->addWidget(hostNameInput, 0, 1);
    layout->addWidget(portLabel, 0, 2);
    layout->addWidget(portInput, 0, 3);

    layout->addWidget(nickLabel, 1, 0);
    layout->addWidget(nickInput, 1, 1);
    layout->addWidget(passwordLabel, 1, 2);
    layout->addWidget(passwordInput, 1, 3);

    layout->addWidget(sslCheckBox, 2, 0);

    hostNameInput->setFocus();

    setButtonOK(KGuiItem(i18n("C&onnect"),"connect_creating",i18n("Connect to the server")));
}

QuickConnectDialog::~QuickConnectDialog()
{
}

void QuickConnectDialog::slotOk()
{
    if (!hostNameInput->text().isEmpty() &&
        !portInput->text().isEmpty() &&
        !nickInput->text().isEmpty())
    {

        emit connectClicked(Konversation::PromptToReuseConnection,
                            hostNameInput->text().trimmed(),
                            portInput->text(),
                            passwordInput->text(),
                            nickInput->text(),
                            "",
                            sslCheckBox->isChecked());
        delayedDestruct();
    }
}

#include "quickconnectdialog.moc"
