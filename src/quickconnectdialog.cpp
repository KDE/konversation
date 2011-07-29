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
#include "application.h"

#include <QCheckBox>

#include <KLineEdit>


QuickConnectDialog::QuickConnectDialog(QWidget *parent)
:KDialog(parent)
{
    showButtonSeparator( true );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    setCaption(  i18n("Quick Connect") );
    setModal( true );
    QWidget* page = mainWidget();

    QGridLayout* layout = new QGridLayout(mainWidget());
    layout->setSpacing(spacingHint());

    QLabel* hostNameLabel = new QLabel(i18n("&Server host:"), page);
    QString hostNameWT = i18n("Enter the host of the network here.");
    hostNameLabel->setWhatsThis(hostNameWT);
    hostNameInput = new KLineEdit(page);
    hostNameInput->setWhatsThis(hostNameWT);
    hostNameLabel->setBuddy(hostNameInput);
    hostNameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    QLabel* portLabel = new QLabel(i18n("&Port:"), page);
    QString portWT = i18n("The port that the IRC server is using.");
    portLabel->setWhatsThis(portWT);
    portInput = new KLineEdit("6667", page );
    portInput->setWhatsThis(portWT);
    portLabel->setBuddy(portInput);
    portLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    QLabel* nickLabel = new QLabel(i18n("&Nick:"), page);
    QString nickWT = i18n("The nick you want to use.");
    nickLabel->setWhatsThis(nickWT);
    nickInput = new KLineEdit(Preferences::identityById(0)->getNickname(0), page);
    nickInput->setWhatsThis(nickWT);
    nickLabel->setBuddy(nickInput);
    nickLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    QLabel* passwordLabel = new QLabel(i18n("P&assword:"), page);
    QString passwordWT = i18n("If the IRC server requires a password, enter it here (most servers do not require a password.)");
    passwordLabel->setWhatsThis(passwordWT);
    passwordInput = new KLineEdit(page);
    passwordInput->setPasswordMode(true);
    passwordInput->setWhatsThis(passwordWT);
    passwordLabel->setBuddy(passwordInput);
    passwordLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    sslCheckBox = new QCheckBox(page);
    sslCheckBox->setObjectName("sslCheckBox");
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

    setButtonGuiItem(KDialog::Ok, KGuiItem(i18n("C&onnect"),"network-connect",i18n("Connect to the server")));

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect( hostNameInput, SIGNAL(textChanged(QString)),this,SLOT(slotServerNameChanged(QString)) );
    slotServerNameChanged( hostNameInput->text() );
}

QuickConnectDialog::~QuickConnectDialog()
{
}

void QuickConnectDialog::slotServerNameChanged( const QString &text )
{
    enableButtonOk( !text.isEmpty() );
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
